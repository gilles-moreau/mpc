/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
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
/* #   - PERACHE Marc    marc.perache@cea.fr                              # */
/* #                                                                      # */
/* ######################################################################## */
#ifndef LOWCOMM_CONFIG_H_
#define LOWCOMM_CONFIG_H_

#include <mpc_config.h>
#include <mpc_lowcomm_types.h>
#include <mpc_conf.h>

/*******************
 * COLLECTIVE CONF *
 *******************/

struct _mpc_lowcomm_coll_conf
{
    char * algorithm;
    void ( *mpc_lowcomm_coll_init_hook )( mpc_lowcomm_communicator_t id );

    /* Barrier */
    int barrier_arity;

    /* Bcast */
    size_t bcast_max_size;
    int bcast_max_arity;
    size_t bcast_check_threshold;

    /* Allreduce */
    size_t allreduce_max_size;
    int allreduce_max_arity;
    size_t allreduce_check_threshold;
    int allreduce_max_slots;

    /* SHM */
    int shm_reduce_interleave;
    int shm_reduce_pipelined_blocks;
    int shm_bcast_interleave;
};

struct _mpc_lowcomm_coll_conf * _mpc_lowcomm_coll_conf_get(void);

void mpc_lowcomm_coll_init_hook( mpc_lowcomm_communicator_t id );

/************************
 * DRIVER CONFIGURATION *
 ************************/


/********************************** ENUM ************************************/
/****/
enum ibv_rdvz_protocol
{
	IBV_RDVZ_WRITE_PROTOCOL,
	IBV_RDVZ_READ_PROTOCOL
};

/********************************** ENUM ************************************/
/**Values used for topological polling in the rail configuration**/
enum rail_topological_polling_level
{
	RAIL_POLL_NONE,
	RAIL_POLL_PU,
	RAIL_POLL_CORE,
	RAIL_POLL_SOCKET,
	RAIL_POLL_NUMA,
	RAIL_POLL_MACHINE
};

/******************************** STRUCTURE *********************************/
/**Options for MPC Fault-Tolerance module.**/
struct sctk_runtime_config_struct_ft
{	int init_done;
	/**Set to true to enable Fault-Tolerance support**/
	int enabled;
};

/******************************** STRUCTURE *********************************/
/**Declare a topological driver.**/
struct sctk_runtime_config_struct_net_driver_topological
{	int init_done;
	/**A test Param**/
	int dummy;
};

/******************************** STRUCTURE *********************************/
/**Declare a fake driver to test the configuration system.**/
struct sctk_runtime_config_struct_net_driver_infiniband
{	int init_done;
	/**Define the pkey value**/
	char * pkey;
	/**Defines the port number to use.**/
	int adm_port;
	/**Defines the verbose level of the Infiniband interface .**/
	int verbose_level;
	/**Size of the eager buffers (short messages).**/
	size_t eager_limit;
	/**Max size for using the Buffered protocol (message split into several Eager messages).**/
	size_t buffered_limit;
	/**Number of entries to allocate in the QP for sending messages. If too low, may cause an QP overrun**/
	int qp_tx_depth;
	/**Number of entries to allocate in the QP for receiving messages. Must be 0 if using SRQ**/
	int qp_rx_depth;
	/**Number of entries to allocate in the CQ. If too low, may cause a CQ overrun**/
	int cq_depth;
	/**Number of RDMA resources on QP (covers both max_dest_rd_atomic and max_rd_atomic)**/
	int rdma_depth;
	/**Max pending RDMA operations for send**/
	int max_sg_sq;
	/**Max pending RDMA operations for recv**/
	int max_sg_rq;
	/**Max size for inlining messages**/
	size_t max_inline;
	/**Defines if RDMA connections may be resized.**/
	int rdma_resizing;
	/**Number of RDMA buffers allocated for each neighbor**/
	int max_rdma_connections;
	/**Max number of RDMA buffers resizing allowed**/
	int max_rdma_resizing;
	/**Max number of Eager buffers to allocate during the initialization step**/
	int init_ibufs;
	/**Defines the number of receive buffers initially allocated. The number is on-the-fly expanded when needed (see init_recv_ibufs_chunk)**/
	int init_recv_ibufs;
	/**Max number of Eager buffers which can be posted to the SRQ. This number cannot be higher than the number fixed by the HW**/
	int max_srq_ibufs_posted;
	/**Max number of Eager buffers which can be used by the SRQ. This number is not fixed by the HW**/
	int max_srq_ibufs;
	/**Min number of free recv Eager buffers before posting a new buffer.**/
	int srq_credit_limit;
	/**Min number of free recv Eager buffers before the activation of the asynchronous thread. If this thread is activated too many times, the performance may be decreased.**/
	int srq_credit_thread_limit;
	/**Number of new buffers allocated when no more buffers are available.**/
	int size_ibufs_chunk;
	/**Number of MMU entries allocated during the MPC initlization.**/
	int init_mr;
	/**Defines if the steal in MPI is allowed **/
	int steal;
	/**Defines if the Infiniband interface must crash quietly.**/
	int quiet_crash;
	/**Defines if the asynchronous may be started at the MPC initialization.**/
	int async_thread;
	/**Defines the number of entries for the CQ dedicated to received messages.**/
	int wc_in_number;
	/**Defines the number of entries for the CQ dedicated to sent messages.**/
	int wc_out_number;
	/**Defines if the low memory mode should be activated**/
	int low_memory;
	/**Defines the Rendezvous protocol to use (IBV_RDVZ_WRITE_PROTOCOL or IBV_RDVZ_READ_PROTOCOL)**/
	enum ibv_rdvz_protocol rdvz_protocol;
	/**Defines the minimum size for the Eager RDMA buffers**/
	size_t rdma_min_size;
	/**Defines the maximun size for the Eager RDMA buffers**/
	size_t rdma_max_size;
	/**Defines the minimum number of Eager RDMA buffers**/
	int rdma_min_nb;
	/**Defines the maximum number of Eager RDMA buffers**/
	int rdma_max_nb;
	/**Defines the minimum size for the Eager RDMA buffers (resizing)**/
	size_t rdma_resizing_min_size;
	/**Defines the maximum size for the Eager RDMA buffers (resizing)**/
	size_t rdma_resizing_max_size;
	/**Defines the minimum number of Eager RDMA buffers (resizing)**/
	int rdma_resizing_min_nb;
	/**Defines the maximum number of Eager RDMA buffers (resizing)**/
	int rdma_resizing_max_nb;
	/**Defines the number of receive buffers allocated on the fly.**/
	int size_recv_ibufs_chunk;
};

/******************************** STRUCTURE *********************************/
/**Global Parameters for IB common structs.**/
struct sctk_runtime_config_struct_ib_global
{	int init_done;
	/**Defines if the MMU cache is enabled.**/
	int mmu_cache_enabled;
	/**Number of entries to keep in the cache.**/
	int mmu_cache_entry_count;
	/**Total size of entries to keep in the cache.**/
	size_t mmu_cache_maximum_size;
	/**Maximum size of an pinned entry.**/
	size_t mmu_cache_maximum_pin_size;
};

/******************************** STRUCTURE *********************************/
/****/
struct sctk_runtime_config_struct_offload_ops_t
{	int init_done;
	/**Enable on-demand optimization through ID hardware propagation**/
	int ondemand;
	/**Enable collective optimization for Portals**/
	int collectives;
};

/******************************** STRUCTURE *********************************/
/**Portals-based driver**/
struct sctk_runtime_config_struct_net_driver_portals
{	int init_done;
	/**Max size of messages allowed to use the eager protocol.**/
	size_t eager_limit;
	/**Min number of communicators (help to avoid dynamic PT entry allocation)**/
	int min_comms;
	/**Above this value, RDV messages will be split in multiple GET requests**/
	size_t block_cut;
	/**List of available optimizations taking advantage of triggered Ops**/
	struct sctk_runtime_config_struct_offload_ops_t offloading;
};

/******************************** STRUCTURE *********************************/
/**TCP-based driver**/
struct sctk_runtime_config_struct_net_driver_tcp
{	int init_done;
	/**Enable TCP over Infiniband (if elligible).**/
	int tcpoib;
};

/******************************** STRUCTURE *********************************/
/**TCP-Based RDMA implementation**/
struct sctk_runtime_config_struct_net_driver_tcp_rdma
{	int init_done;
	/**Enable TCP over Infiniband (if elligible).**/
	int tcpoib;
};

/******************************** STRUCTURE *********************************/
/**Inter-Process shared memory communication implementation**/
struct sctk_runtime_config_struct_net_driver_shm
{	int init_done;
	/**Defines priority for the SHM buffered message**/
	int buffered_priority;
	/**Defines the min size for the SHM buffered message**/
	int buffered_min_size;
	/**Defines the min size for the SHM buffered message**/
	int buffered_max_size;
	/**Defines if mode zerocopy should be actived for SHM buffered message**/
	int buffered_zerocopy;
	/****/
	int cma_enable;
	/**Defines priority for the SHM CMA message**/
	int cma_priority;
	/**Defines the min size for the SHM CMA message**/
	int cma_min_size;
	/**Defines the min size for the SHM CMA message**/
	int cma_max_size;
	/**Defines if mode zerocopy should be actived for SHM CMA message**/
	int cma_zerocopy;
	/**Defines priority for the SHM fragmented message**/
	int frag_priority;
	/**Defines the min size for the SHM fragmented message**/
	int frag_min_size;
	/**Defines the min size for the SHM fragmented message**/
	int frag_max_size;
	/**Defines if mode zerocopy should be actived for SHM fragmented message**/
	int frag_zerocopy;
	/**Size of shared memory region.**/
	int shmem_size;
	/**Size of shared memory region.**/
	int cells_num;
};

/********************************** ENUM ************************************/
/**Define a specific configuration for a network driver to apply in rails.**/
enum sctk_runtime_config_struct_net_driver_type
{
	SCTK_RTCFG_net_driver_NONE,
	SCTK_RTCFG_net_driver_infiniband,
	SCTK_RTCFG_net_driver_portals,
	SCTK_RTCFG_net_driver_tcp,
	SCTK_RTCFG_net_driver_tcprdma,
	SCTK_RTCFG_net_driver_shm,
	SCTK_RTCFG_net_driver_topological,
};

/******************************** STRUCTURE *********************************/
/**Define a specific configuration for a network driver to apply in rails.**/
struct sctk_runtime_config_struct_net_driver
{
	enum sctk_runtime_config_struct_net_driver_type type;
	union {
		struct sctk_runtime_config_struct_net_driver_infiniband infiniband;
		struct sctk_runtime_config_struct_net_driver_portals portals;
		struct sctk_runtime_config_struct_net_driver_tcp tcp;
		struct sctk_runtime_config_struct_net_driver_tcp_rdma tcprdma;
		struct sctk_runtime_config_struct_net_driver_shm shm;
		struct sctk_runtime_config_struct_net_driver_topological topological;
	} value;
};

/******************************** STRUCTURE *********************************/
/**Contain a list of driver configuration reused by rail definitions.**/
struct sctk_runtime_config_struct_net_driver_config
{	int init_done;
	/**Name of the driver configuration to be referenced in rail definitions.**/
	char * name;
	/**Define the related driver to use and its configuration.**/
	struct sctk_runtime_config_struct_net_driver driver;
};


/**********************
 * RAIL CONFIGURATION *
 **********************/

struct sctk_runtime_config_funcptr
{
	char * name;
	void (* value)();
};


/******************************** STRUCTURE *********************************/
/**This gate applies given thruth value to messages.**/
struct sctk_runtime_config_struct_gate_boolean
{	int init_done;
	/**whereas to accept input messages or not**/
	int value;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail with a parametrized probability.**/
struct sctk_runtime_config_struct_gate_probabilistic
{	int init_done;
	/**Probability to choose this rail in percents (ralatively to this single rail, integer)**/
	int probability;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail if size is at least a given value.**/
struct sctk_runtime_config_struct_gate_min_size
{	int init_done;
	/**Minimum size to choose this rail (with units)**/
	size_t value;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail if size is at most a given value.**/
struct sctk_runtime_config_struct_gate_max_size
{	int init_done;
	/**Maximum size to choose this rail (with units)**/
	size_t value;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate can be used define which type of message can use a given rail.**/
struct sctk_runtime_config_struct_gate_message_type
{	int init_done;
	/**Process Specific Messages can use this rail**/
	int process;
	/**Task specific messages can use this rail**/
	int task;
	/**Task specific messages can use this rail**/
	int emulated_rma;
	/**Common messages (MPI) can use this rail**/
	int common;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/******************************** STRUCTURE *********************************/
/**This gate uses a given rail with a user defined function.**/
struct sctk_runtime_config_struct_gate_user
{	int init_done;
	/**Function to be called for this gate**/
	struct sctk_runtime_config_funcptr gatefunc;
};

/********************************** ENUM ************************************/
/**Defines gates and their configuration.**/
enum sctk_runtime_config_struct_net_gate_type
{
	SCTK_RTCFG_net_gate_NONE,
	SCTK_RTCFG_net_gate_boolean,
	SCTK_RTCFG_net_gate_probabilistic,
	SCTK_RTCFG_net_gate_minsize,
	SCTK_RTCFG_net_gate_maxsize,
	SCTK_RTCFG_net_gate_msgtype,
	SCTK_RTCFG_net_gate_user,
};

/******************************** STRUCTURE *********************************/
/**Defines gates and their configuration.**/
struct sctk_runtime_config_struct_net_gate
{
	enum sctk_runtime_config_struct_net_gate_type type;
	union {
		struct sctk_runtime_config_struct_gate_boolean boolean;
		struct sctk_runtime_config_struct_gate_probabilistic probabilistic;
		struct sctk_runtime_config_struct_gate_min_size minsize;
		struct sctk_runtime_config_struct_gate_max_size maxsize;
		struct sctk_runtime_config_struct_gate_message_type msgtype;
		struct sctk_runtime_config_struct_gate_probabilistic user;
	} value;
};

/******************************** STRUCTURE *********************************/
/**Defines a topological polling configuration.**/
struct sctk_runtime_config_struct_topological_polling
{	int init_done;
	/**Define the subset of cores involved in the polling.**/
	enum rail_topological_polling_level range;
	/**Define the subset of cores involved in the polling.**/
	enum rail_topological_polling_level trigger;
};

/******************************** STRUCTURE *********************************/
/**Define a rail which is a name, a device associate to a driver and a routing topology.**/
struct sctk_runtime_config_struct_net_rail
{	int init_done;
	/**Define the name of current rail.**/
	char * name;
	/**Number which defines the order in which routes are tested (higher first).**/
	int priority;
	/**Define the name of the device to use in this rail.**/
	char * device;
	/**Define how the any-source polling is done.**/
	struct sctk_runtime_config_struct_topological_polling any_source_polling;
	/**Define the network topology to apply on this rail.**/
	char * topology;
	/**Define if on-demand connections are allowed on this rail.**/
	int ondemand;
	/**Defines if the rail has RDMA enabled.**/
	int rdma;
	/**Define the driver config to use for this rail.**/
	char * config;
	/**List of gates to be applied in this config.**/
	struct sctk_runtime_config_struct_net_gate * gates;
	/** Number of elements in gates array. **/
	int gates_size;
	/**Used for topological rail selection**/
	struct sctk_runtime_config_struct_net_rail * subrails;
	/** Number of elements in subrails array. **/
	int subrails_size;
};


/******************************** STRUCTURE *********************************/
/**Base structure to contain the network configuration**/
struct sctk_runtime_config_struct_networks
{	int init_done;
	/**Define the configuration driver list to reuse in rail definitions.**/
	struct sctk_runtime_config_struct_net_driver_config * configs;
	/** Number of elements in configs array. **/
	int configs_size;
	/**List of rails to declare in MPC.**/
	struct sctk_runtime_config_struct_net_rail * rails;
	/** Number of elements in rails array. **/
	int rails_size;

	/** Name of the default CLI option to choose */
	char * cli_default_network;
};


struct sctk_runtime_config_struct_networks * _mpc_lowcomm_net_config_get(void);

/************************************
 * GLOBAL CONFIGURATION FOR LOWCOMM *
 ************************************/

struct _mpc_lowcomm_conf
{
#ifdef SCTK_USE_CHECKSUM
    int checksum;
#endif
};

struct _mpc_lowcomm_conf * _mpc_lowcomm_conf_get(void);

void _mpc_lowcomm_config_register(void);
void _mpc_lowcomm_config_validate(void);


#endif /* LOWCOMM_CONFIG_H_ */