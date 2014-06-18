/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_config.h"
#include "sctk_topology.h"
#include "sctk_launch.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk.h"
#include "sctk_kernel_thread.h"

#ifdef MPC_Message_Passing
#include "sctk_pmi.h"
#endif

#include <dirent.h>

#include <stdio.h>
#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#define SCTK_MAX_PROCESSOR_NAME 100
#define SCTK_MAX_NODE_NAME 512
#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

static char* sctk_xml_specific_path = NULL;
static int sctk_processor_number_on_node = 0;
static char sctk_node_name[SCTK_MAX_NODE_NAME];
static sctk_spinlock_t topology_lock = SCTK_SPINLOCK_INITIALIZER;
hwloc_bitmap_t pin_processor_bitmap;
#define MAX_PIN_PROCESSOR_LIST 1024
int pin_processor_list[MAX_PIN_PROCESSOR_LIST];
int pin_processor_current = 0;
int bind_processor_current = 0;

/* The topology of the machine + its cpuset */
static hwloc_topology_t topology;
static hwloc_bitmap_t topology_cpuset;
/* Describe the full topology of the machine.
 * Only used for binding*/
static hwloc_topology_t topology_full;
const struct hwloc_topology_support *support;

hwloc_topology_t sctk_get_topology_object(void)
{
	return topology ;
}

/* print a cpuset in a human readable way */
void
print_cpuset(hwloc_bitmap_t cpuset)
{
  char buff[256];
  hwloc_bitmap_list_snprintf(buff, 256, cpuset);
  fprintf(stdout, "cpuset: %s\n", buff);
  fflush(stdout);
}

static void
sctk_update_topology (
    const int processor_number,
    const int index_first_processor
    ) {
  sctk_processor_number_on_node = processor_number ;
  hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
  unsigned int i;
  int err;

  hwloc_bitmap_zero(cpuset);
  if (hwloc_bitmap_iszero(pin_processor_bitmap)) {
	  for( i=index_first_processor; i < index_first_processor+processor_number; ++i)
	  {
		  hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
		  hwloc_cpuset_t set = hwloc_bitmap_dup(pu->cpuset);
		  hwloc_bitmap_singlify(set);
		  hwloc_bitmap_or(cpuset, cpuset, set);
	  }

  } else {
    int sum = 0;
    sum = hwloc_get_nbobjs_inside_cpuset_by_type(topology, pin_processor_bitmap, HWLOC_OBJ_PU);
    if (sum != sctk_get_processor_nb ()) {
      sctk_error("MPC_PIN_PROCESSOR_LIST is set with a different number of processor available on the node: %d in the list, %d on the node", sum, sctk_get_processor_nb());
      sctk_abort();
    }

	  hwloc_bitmap_copy(cpuset, pin_processor_bitmap);
  }
  err = hwloc_topology_restrict(topology, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
  assume(!err);
  hwloc_bitmap_copy(topology_cpuset, cpuset);
  hwloc_bitmap_free(cpuset);
}

static void
sctk_expand_pin_processor_add_to_list(int id) {
  hwloc_cpuset_t previous = hwloc_bitmap_alloc();
  hwloc_bitmap_copy(previous, pin_processor_bitmap);
	if ( id > (sctk_processor_number_on_node - 1) ) sctk_error("Error in MPC_PIN_PROCESSOR_LIST: processor %d is more than the max number of cores %d", id, sctk_processor_number_on_node);
	hwloc_bitmap_set(pin_processor_bitmap, id);

  /* New entry added. We also add it in the list */
  if ( !hwloc_bitmap_isequal(pin_processor_bitmap, previous)) {
    assume(pin_processor_current < MAX_PIN_PROCESSOR_LIST);
    pin_processor_list[pin_processor_current] = id;
    pin_processor_current++;
  }
  hwloc_bitmap_free(previous);
}

  static void
sctk_expand_pin_processor_list(char *env) {
	char *c = env;
	char prev_number[5];
	char *prev_ptr=prev_number;
	int id;

	while(*c != '\0') {
		if (isdigit(*c)) {
			*prev_ptr=*c;
			prev_ptr++;
		} else if (*c == ',') {
			*prev_ptr='\0';
			id = atoi(prev_number);
			sctk_expand_pin_processor_add_to_list(id);
			prev_ptr = prev_number;
		} else if (*c == '-') {
			*prev_ptr='\0';
			int start_num = atoi(prev_number);
			prev_ptr = prev_number;
			++c;
			while( isdigit(*c)) {
				*prev_ptr=*c;
				prev_ptr++;
				++c;
			}
			--c;
			*prev_ptr='\0';
			int end_num = atoi(prev_number);
			int i;
			for (i=start_num; i <= end_num; ++i) {
				sctk_expand_pin_processor_add_to_list(i);
			}
			prev_ptr = prev_number;
		}else{
			sctk_error("Error in MPC_PIN_PROCESSOR_LIST: wrong character read: %c", *c);
			sctk_abort();
		}
		++c;
	}

	/*Terminates the string */
	if (prev_ptr != prev_number) {
		*prev_ptr='\0';
		id = atoi(prev_number);
		sctk_expand_pin_processor_add_to_list(id);
	}
}

  static void
sctk_restrict_topology ()
{
  int rank ;
  sctk_only_once();

 restart_restrict:
  if (sctk_enable_smt_capabilities)
  {
    int i;
    sctk_warning ("SMT capabilities ENABLED");

    sctk_processor_number_on_node = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    hwloc_bitmap_zero(topology_cpuset);

    for(i=0;i < sctk_processor_number_on_node; ++i)
    {
      hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
      hwloc_bitmap_or(topology_cpuset, topology_cpuset, core->cpuset);
    }

  }
  else
  {
    /* disable SMT capabilities */
    unsigned int i;
    int err;
    hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
    hwloc_cpuset_t set = hwloc_bitmap_alloc();
    hwloc_bitmap_zero(cpuset);
    const int core_number = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);

    for(i=0;i < core_number; ++i)
    {
      hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
      hwloc_bitmap_copy(set, core->cpuset);
      hwloc_bitmap_singlify(set);
      hwloc_bitmap_or(cpuset, cpuset, set);
    }
    /* restrict the topology to physical CPUs */
    err = hwloc_topology_restrict(topology, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
    if(err){
      hwloc_bitmap_free(cpuset);
      hwloc_bitmap_free(set);
      sctk_enable_smt_capabilities = 1;
      sctk_warning ("Topology reduction issue");
      goto restart_restrict;
    }
    hwloc_bitmap_copy(topology_cpuset, cpuset);

    hwloc_bitmap_free(cpuset);
    hwloc_bitmap_free(set);
    sctk_processor_number_on_node = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
  }

	pin_processor_bitmap = hwloc_bitmap_alloc();
  hwloc_bitmap_zero(pin_processor_bitmap);
  char* pinning_env = getenv("MPC_PIN_PROCESSOR_LIST");
  if (pinning_env != NULL ) {
	  sctk_expand_pin_processor_list(pinning_env);
  }

  /* Share nodes */
  sctk_share_node_capabilities = 1;
  if ((sctk_share_node_capabilities == 1) &&
      (sctk_process_number > 1))
  {
    int detected_on_this_host = 0;
    int detected = 0;

#ifdef MPC_Message_Passing
    /* Determine number of processes on this node */
    sctk_pmi_get_process_on_node_number(&detected_on_this_host);
    sctk_pmi_get_process_on_node_rank(&rank);
    detected = sctk_process_number;
    sctk_nodebug("Nb process on node %d",detected_on_this_host);
#else
    detected = 1;
#endif

    /* More than 1 process per node is not supported by process pinning */
    int nodes_number = sctk_get_node_nb();
    if ( (sctk_process_number != nodes_number) && !hwloc_bitmap_iszero(pin_processor_bitmap)) {
      sctk_error("MPC_PIN_PROCESSOR_LIST cannot be set if more than 1 process per node is set. process_number=%d node_number=%d", sctk_process_number, nodes_number);
      sctk_abort();
    }

    while (detected != sctk_get_process_nb ());
    sctk_nodebug ("%d/%d host detected %d share %s", detected,
        sctk_get_process_nb (), detected_on_this_host,
        sctk_node_name);

    {
      /* Determine processor number per process */
      int processor_number = sctk_processor_number_on_node / detected_on_this_host;
      int remaining_procs = sctk_processor_number_on_node % detected_on_this_host;
      int start = processor_number * rank;
      if(processor_number < 1){
	processor_number = 1;
	remaining_procs = 0;
	start = (processor_number * rank) % sctk_processor_number_on_node;
      }

      if ( remaining_procs > 0 )
      {
        if ( rank < remaining_procs )
        {
          processor_number++;
          start += rank;
        } else {
          start += remaining_procs;
        }
      }

      sctk_update_topology ( processor_number, start ) ;
    }

  } /* share node */

  /* Choose the number of processor used */
  {
    int processor_number;

    processor_number = sctk_get_processor_nb ();

    if (processor_number > 0)
    {
      if (processor_number > sctk_processor_number_on_node)
      {
        sctk_error ("Unable to allocate %d core (%d real cores)",
            processor_number, sctk_processor_number_on_node);
        /* we cannot exit with a sctk_abort() because thread
         * library is not initialized at this moment */
        /* sctk_abort (); */
        exit(1);
      }
      else if ( processor_number < sctk_processor_number_on_node )
      {
        sctk_warning ("Do not use the entire node %d on %d",
            processor_number, sctk_processor_number_on_node);
        sctk_update_topology ( processor_number, 0 ) ;
      }
    }
  }
}

#include <sched.h>
#if defined(HP_UX_SYS)
#include <sys/param.h>
#include <sys/pstat.h>
#endif
#ifdef HAVE_UTSNAME
static struct utsname utsname;
#else
struct utsname
{
  char sysname[];
  char nodename[];
  char release[];
  char version[];
  char machine[];
};

static struct utsname utsname;

  static int
uname (struct utsname *buf)
{
  buf->sysname = "";
  buf->nodename = "";
  buf->release = "";
  buf->version = "";
  buf->machine = "";
  return 0;
}

#endif

/*! \brief Return the current core_id
*/
static __thread int sctk_get_cpu_val = -1;
static inline  int
sctk_get_cpu_intern ()
{
  hwloc_cpuset_t set = hwloc_bitmap_alloc();

  int ret = hwloc_get_last_cpu_location(topology, set, HWLOC_CPUBIND_THREAD);

  assume(ret!=-1);
  assume(!hwloc_bitmap_iszero(set));


  /* Check if HWLOC_XMLFILE env variable is set. This variable aims at modifing 
   *  a fake topology to test the runtime bahivior. On the downside, hwloc 
   *  disables binding. */
  
  int isset_hwloc_xmlfile =  (getenv ("HWLOC_XMLFILE") != NULL);

  /* Check if HWLOC_THISSYSTEM env variable is set. This variable aims at asserting 
   *  that the topology loaded with the HWLOC_XMLFILE env variable is the same as
   *  this system. */

  int isset_hwloc_thissystem = 0;
  char* hwloc_thissystem = getenv ("HWLOC_THISSYSTEM");
  if (hwloc_thissystem != NULL) isset_hwloc_thissystem =  (strcmp ( hwloc_thissystem , "1") == 0);

  if ( !isset_hwloc_xmlfile || (isset_hwloc_xmlfile && isset_hwloc_thissystem) ) {
    /* Check if only one CPU in the CPU set. Maybe there is a simpler function
     * to do that */
    if(sctk_xml_specific_path == NULL)  
      assume (hwloc_bitmap_first(set) ==  hwloc_bitmap_last(set));
  }

  hwloc_obj_t pu = hwloc_get_obj_inside_cpuset_by_type(topology, set, HWLOC_OBJ_PU, 0);

  if (!pu) {
    hwloc_bitmap_free(set);
    return -1;
  }

  /* And return the logical index */
  int cpu = pu->logical_index;

  return cpu;
}

  int
sctk_get_cpu ()
{
  if(sctk_get_cpu_val < 0){
    sctk_spinlock_lock(&topology_lock);
    int ret = sctk_get_cpu_intern();
    sctk_spinlock_unlock(&topology_lock);
    return ret;
  } else {
    return sctk_get_cpu_val;
  }
}

void sctk_topology_init_cpu(){
  sctk_get_cpu_val = -1;
}

/*! \brief Initialize the topology module
*/
  void
sctk_topology_init ()
{
  char* xml_path;
#ifdef MPC_Message_Passing
  if(sctk_process_number > 1){
    sctk_pmi_init();
  }
#endif

  xml_path = getenv("MPC_SET_XML_TOPOLOGY_FILE");
  sctk_xml_specific_path = xml_path;
  hwloc_topology_init(&topology);
  if(xml_path != NULL){
    fprintf(stderr,"USE XML file %s\n",xml_path);
    hwloc_topology_set_xml(topology,xml_path);
  }
  hwloc_topology_load(topology);

  hwloc_topology_init(&topology_full);
  if(xml_path != NULL){
    hwloc_topology_set_xml(topology_full,xml_path);
  }
  hwloc_topology_load(topology_full);

  topology_cpuset = hwloc_bitmap_alloc();

  support = hwloc_topology_get_support(topology);

  sctk_only_once ();
  sctk_print_version ("Topology", SCTK_LOCAL_VERSION_MAJOR,
      SCTK_LOCAL_VERSION_MINOR);

  sctk_restrict_topology();

#ifndef WINDOWS_SYS
  gethostname (sctk_node_name, SCTK_MAX_NODE_NAME);
#else
  sprintf (sctk_node_name, "localhost");
#endif

  uname (&utsname);

  if (!hwloc_bitmap_iszero(pin_processor_bitmap)) {
    sctk_print_topology (stderr);
  }
}

/*! \brief Destroy the topology module
*/
void sctk_topology_destroy (void)
{
  hwloc_topology_destroy(topology);
}


/*! \brief Return the hostname
*/
  char *
sctk_get_node_name ()
{
  return sctk_node_name;
}

/*! \brief Return the first child of \p obj of type \p type
 * @param obj Parent object
 * @param type Child type
 */
hwloc_obj_t
sctk_get_first_child_by_type(hwloc_obj_t obj, hwloc_obj_type_t type)
{
  hwloc_obj_t child = obj;
  do
  {
    child = child->first_child;
  }
  while(child->type != type);
  return child;
}

/*! \brief Print the topology tree into a file
 * @param fd Destination file descriptor
 */
  void
sctk_print_topology (FILE * fd)
{
  int i;
  fprintf(fd, "Node %s: %s %s %s\n", sctk_node_name, utsname.sysname,
      utsname.release, utsname.version);
  const int pu_number = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
  for(i=0;i < pu_number; ++i)
  {
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
    hwloc_obj_t tmp[3];
    unsigned int node_os_index;
    tmp[0] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, pu);
    tmp[1] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, pu);
    tmp[2] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, pu);

    if(tmp[0] == NULL){
      node_os_index = 0;
    } else {
      node_os_index = tmp[0]->os_index;
    }

    fprintf(fd, "\tProcessor %4u real (%4u:%4u:%4u)\n", pu->os_index,
	    node_os_index, tmp[1]->logical_index, tmp[2]->logical_index );
  }

  fprintf (fd, "\tNUMA: %d\n", sctk_is_numa_node ());
  if (sctk_is_numa_node ())
  {
    fprintf (fd, "\t\t* Node nb %d\n", sctk_get_numa_node_number());
  }
  return;
}

  void
sctk_print_specific_topology (FILE * fd, hwloc_topology_t topology)
{
  int i;
  fprintf(fd, "Node %s: %s %s %s\n", sctk_node_name, utsname.sysname,
      utsname.release, utsname.version);
  const int pu_number = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
  for(i=0;i < pu_number; ++i)
  {
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
    hwloc_obj_t tmp[3];
    unsigned int node_os_index;
    tmp[0] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, pu);
    tmp[1] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, pu);
    tmp[2] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, pu);

    if(tmp[0] == NULL){
      node_os_index = 0;
    } else {
      node_os_index = tmp[0]->os_index;
    }

    fprintf(fd, "\tProcessor %4u real (%4u:%4u:%4u)\n", pu->os_index,
	    node_os_index, tmp[1]->logical_index, tmp[2]->logical_index );
  }

  fprintf (fd, "\tNUMA: %d\n", sctk_is_numa_node ());
  if (sctk_is_numa_node ())
  {
    fprintf (fd, "\t\t* Node nb %d\n", sctk_get_numa_node_number());
  }
  return;
}

/* Restrict the binding to the current cpu_set */
void sctk_restrict_binding() {
  int err;

  err = hwloc_set_cpubind(topology, topology_cpuset, HWLOC_CPUBIND_THREAD);
  assume(!err);

  sctk_topology_init_cpu();
}

/*! \brief Bind the current thread. The function may return -1 if the thread
 * was previously bound to a core that was not managed by the HWLOC topology
 * @ param i The cpu_id to bind
 */
  int
sctk_bind_to_cpu (int i)
{
  char * msg = "Bind to cpu";
  int supported = support->cpubind->set_thisthread_cpubind;
  const char *errmsg = strerror(errno);

  sctk_spinlock_lock(&topology_lock);

  int ret = sctk_get_cpu_intern();

  if (i >= 0)
  {
TODO("Handle specific mapping from the user");
    hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
    assume(pu);

    int err = hwloc_set_cpubind(topology, pu->cpuset, HWLOC_CPUBIND_THREAD);
    if (err)
    {
      fprintf(stderr,"%-40s: %sFAILED (%d, %s)\n", msg, supported?"":"X", errno, errmsg);
    }
//    assume(sctk_get_cpu_intern() == i);
    sctk_get_cpu_val = i;
  }
  sctk_spinlock_unlock(&topology_lock);
  return ret;
}

/*! \brief Return the type of processor (x86, x86_64, ...)
*/
  char *
sctk_get_processor_name ()
{
  return utsname.machine;
}

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
 */
  int
sctk_get_first_cpu_in_node (int node)
{
  hwloc_obj_t currentNode = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, node);

  while(currentNode->type != HWLOC_OBJ_CORE) {
    currentNode = currentNode->children[0];
  }

  return currentNode->logical_index ;
}

/*! \brief Return the total number of core for the process
*/
  int
sctk_get_cpu_number ()
{
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
}

/*! \brief Return the total number of core for the process
*/
  int
sctk_get_cpu_number_topology (hwloc_topology_t topo)
{
  return hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU);
}

/*! \brief Set the number of core usable for the current process
 * @ param n Number of cores
 * used in ethread
 */
  int
sctk_set_cpu_number (int n)
{
  if(n <= sctk_get_cpu_number ()){
    sctk_update_topology ( n, 0 ) ;
  }
  return n;
}

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise
*/
  int
sctk_is_numa_node ()
{
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) != 0;
}

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise
*/
  int
sctk_is_numa_node_topology (hwloc_topology_t topo)
{
  return hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_NODE) != 0;
}

/*! \brief Return the number of NUMA nodes
*/
  int
sctk_get_numa_node_number ()
{
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) ;
}

/*! \brief Return the NUMA node according to the code_id number
 * @param vp VP
 */
  int
sctk_get_node_from_cpu (const int vp)
{
  if(sctk_is_numa_node ()){
    const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
    assume(pu);
    const hwloc_obj_t node = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, pu);

    return node->logical_index;
  } else {
    return 0;
  }
}

/*! \brief Return the NUMA node according to the code_id number
 * @param vp VP
 */
  int
  sctk_get_node_from_cpu_topology (hwloc_topology_t topo, const int vp)
{
  if(sctk_is_numa_node_topology (topo)){
    const hwloc_obj_t pu = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, vp);
    assume(pu);
    const hwloc_obj_t node = hwloc_get_ancestor_obj_by_type(topo, HWLOC_OBJ_NODE, pu);

    return node->logical_index;
  } else {
    return 0;
  }
}

/* print the neighborhood*/
void
print_neighborhood(int cpuid, int nb_cpus, int* neighborhood, hwloc_obj_t* objs)
{
  int i;
  hwloc_obj_t currentCPU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuid);
  hwloc_obj_type_t type = (sctk_is_numa_node()) ? HWLOC_OBJ_NODE : HWLOC_OBJ_MACHINE;
  int currentCPU_node_id = hwloc_get_ancestor_obj_by_type(topology, type, currentCPU)->logical_index;
  int currentCPU_socket_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, currentCPU)->logical_index;
  int currentCPU_core_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, currentCPU)->logical_index;
  fprintf(stderr, "Neighborhood result starting with CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
      cpuid, currentCPU_node_id, currentCPU_socket_id, currentCPU_core_id);

  for (i = 0; i < nb_cpus; i++)
  {
    hwloc_obj_t current = objs[i];

    fprintf(stderr, "\tNeighbor[%d] -> CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
        i, neighborhood[i],
        hwloc_get_ancestor_obj_by_type(topology, type, current)->logical_index,
        hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, current)->logical_index,
        hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index);
  }
}

/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
 */
  void
sctk_get_neighborhood(int cpuid, int nb_cpus, int* neighborhood)
{
  int i;
  hwloc_obj_t *objs;
  hwloc_obj_t currentCPU ; 
  unsigned nb_cpus_found;

  currentCPU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuid);

  /* alloc size for retreiving objects. We could also use a static array */
  objs = sctk_malloc(nb_cpus * sizeof(hwloc_obj_t));
  sctk_assert( objs != NULL ) ;

  /* The closest CPU is actually... the current CPU :-). 
   * Set the closest CPU as the current CPU */
  objs[0] = currentCPU;

  /* get closest objects to the current CPU */
  nb_cpus_found = hwloc_get_closest_objs(topology, currentCPU, &objs[1], (nb_cpus-1));
  /*  +1 because of the current cpu not returned by hwloc_get_closest_objs */
  assume( (nb_cpus_found + 1) == nb_cpus);

  /* fill the neighborhood variable */
  for (i = 0; i < nb_cpus; i++)
  {
    /*
       hwloc_obj_t current = objs[i];
       neighborhood[i] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index;
       */
    neighborhood[i] = objs[i]->logical_index;
  }

  /* uncomment for printing the returned result */
  //print_neighborhood(cpuid, nb_cpus, neighborhood, objs);

  sctk_free(objs);
}

/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
 */
  void
  sctk_get_neighborhood_topology(hwloc_topology_t topo, int cpuid, int nb_cpus, int* neighborhood)
{
  int i;
  hwloc_obj_t *objs;
  hwloc_obj_t currentCPU ; 
  unsigned nb_cpus_found;

  currentCPU = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, cpuid);

  /* alloc size for retreiving objects. We could also use a static array */
  objs = sctk_malloc(nb_cpus * sizeof(hwloc_obj_t));
  sctk_assert( objs != NULL ) ;

  /* The closest CPU is actually... the current CPU :-). 
   * Set the closest CPU as the current CPU */
  objs[0] = currentCPU;

  /* get closest objects to the current CPU */
  nb_cpus_found = hwloc_get_closest_objs(topo, currentCPU, &objs[1], (nb_cpus-1));
  /*  +1 because of the current cpu not returned by hwloc_get_closest_objs */
  assume( (nb_cpus_found + 1) == nb_cpus);

  /* fill the neighborhood variable */
  for (i = 0; i < nb_cpus; i++)
  {
    /*
       hwloc_obj_t current = objs[i];
       neighborhood[i] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index;
       */
    neighborhood[i] = objs[i]->logical_index;
  }

  /* uncomment for printing the returned result */
  //print_neighborhood(cpuid, nb_cpus, neighborhood, objs);

  sctk_free(objs);
}

/*
 * For OpenFabrics
 */
#ifdef MPC_USE_INFINIBAND
/* Add functions here for IB */

int sctk_topology_is_ib_device_close_from_cpu (struct ibv_device * dev, int logical_core_id) {
  int err;

  hwloc_bitmap_t ib_set;
  ib_set = hwloc_bitmap_alloc();
  err = hwloc_ibv_get_device_cpuset(topology, dev, ib_set);
  if (err < 0) {
    return -1;
  } else {
    int res;

    hwloc_obj_t obj_cpu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logical_core_id);
    assume(obj_cpu);

#if 0
    char *cpuset_string = NULL;
    hwloc_bitmap_list_asprintf(&cpuset_string, ib_set);
    sctk_debug("String: %s", cpuset_string);
#endif

    hwloc_bitmap_and(ib_set, ib_set, obj_cpu->cpuset);
    res = hwloc_bitmap_iszero(ib_set);

    hwloc_bitmap_free(ib_set);
    /* If the bitmap is different from 0, we are close to the IB card, so
     * we return 1 */
    return ! res;
  }
}



#endif
