#include "mpc_mpi.h"
#include "sctk_debug.h"
#include <string.h>
#include "mpc_datatypes.h"
#include "mpc_common.h"
#include "uthash.h"

/************************************************************************/
/* ERROR Handling                                                       */
/************************************************************************/

int MPIR_Err_create_code_valist(int a, int b, const char c[], int d, int e, 
				const char f[], const char g[], va_list args )
{
	return MPI_SUCCESS;
}

int MPIR_Err_is_fatal(int a)
{
	return 0;
}

typedef int (* MPIR_Err_get_class_string_func_t)(int error, char *str, int length);

void MPIR_Err_get_string( int errcode, char *msg, int maxlen, MPIR_Err_get_class_string_func_t fcname )
{
	char buff[128];
	int len;
	
	if( !msg )
		return;
	
	buff[0] = '\0';
	msg[0] = '\0';
	
	PMPC_Error_string (errcode, buff, &len);

	if( strlen( buff ) )
		snprintf( msg, maxlen, "%s", buff );
}


struct MPID_Comm;
int MPID_Abort(struct MPID_Comm *comm, int mpi_errno, int exit_code, const char *error_msg)
{
	sctk_error("FATAL : %d [exit : %d ] : %s", mpi_errno, exit_code, error_msg );
	MPI_Abort( MPI_COMM_WORLD, mpi_errno );
	return MPI_SUCCESS;
}


int PMPI_File_set_errhandler( void * file,  MPI_Errhandler errhandler )
{
	return MPI_SUCCESS;
}

int PMPI_File_get_errhandler(void * file, MPI_Errhandler *errhandler)
{
	return MPI_SUCCESS;
}


void MPIR_Get_file_error_routine( MPI_Errhandler a, 
				  void (**errr)(void * , int * , ...), 
				  int * b)
{
	
	
	
}


int MPIR_File_call_cxx_errhandler( void *fh, int *errorcode, 
			   void (*c_errhandler)(void  *, int *, ... ) )
{
	
	return MPI_SUCCESS;
}


int PMPI_Comm_call_errhandler( MPI_Comm comm, int errorcode )
{
	
	return MPI_SUCCESS;
}


/* Default error handling implementation.
 *
 * Note that only MPI_ERRORS_ARE_FATAL and MPI_ERRORS_RETURN are
 * handled correctly; other handlers cause an abort.
 */

int MPIO_Err_create_code(int lastcode, int fatal, const char fcname[],
			 int line, int error_class, const char generic_msg[],
			 const char specific_msg[], ... )
{
    va_list Argp;
    int idx = 0;
    char *buf;

    buf = (char *) sctk_malloc(1024);
    if (buf != NULL) {
	idx += snprintf(buf, 1023, "%s (line %d): ", fcname, line);
	if (specific_msg == NULL) {
	    snprintf(&buf[idx], 1023 - idx, "%s\n", generic_msg);
	}
	else {
	    va_start(Argp, specific_msg);
	    vsnprintf(&buf[idx], 1023 - idx, specific_msg, Argp);
	    va_end(Argp);
	}
	fprintf(stderr, "%s", buf);
	sctk_free(buf);
    }

    return error_class;
}


/************************************************************************/
/* Datatype Optimization                                                */
/************************************************************************/

void MPIR_Datatype_iscontig(MPI_Datatype datatype, int *flag)
{
	sctk_task_specific_t *task_specific;
	
	*flag = 0;
	
	if( sctk_datatype_is_boundary(datatype) )
		return;
	
	switch( sctk_datatype_kind( datatype ) )
	{
		case MPC_DATATYPES_CONTIGUOUS:
			*flag = 1;
		break;
		
		case MPC_DATATYPES_DERIVED:
			task_specific = __MPC_get_task_specific ();	
			sctk_derived_datatype_t *target_type = sctk_task_specific_get_derived_datatype(  task_specific, datatype );
			assume( target_type != NULL );
			
			if( (target_type->count == 0) ||  (target_type->opt_count == 1) )
				*flag = 1;

		break;
		
		case MPC_DATATYPES_COMMON:
			*flag = 1;
		break;
		default:
			not_reachable();
	}

}


int MPCX_Type_flatten( MPI_Datatype datatype, long int ** blocklen, long int ** indices , MPI_Count * count )
{
	sctk_task_specific_t *task_specific = __MPC_get_task_specific ();
	sctk_derived_datatype_t *target_derived_type;
	sctk_contiguous_datatype_t *contiguous_type;
	
	if( datatype == MPI_DATATYPE_NULL )
	{
		*count = 0;
		return MPI_SUCCESS;
	}
	
	
	switch( sctk_datatype_kind( datatype ) )
	{
		case MPC_DATATYPES_COMMON:
			*count = 1;
			*blocklen = malloc( sizeof( long int ) );
			*indices = malloc( sizeof( long int ) );
			(*indices)[0] = 0;
			(*blocklen)[0] = sctk_common_datatype_get_size( datatype );
		break;
		case MPC_DATATYPES_CONTIGUOUS :
			contiguous_type = sctk_task_specific_get_contiguous_datatype( task_specific, datatype );
			*count = 1;
			*blocklen = malloc( sizeof( long int ) );
			*indices = malloc( sizeof( long int ) );
			(*indices)[0] = 0;
			(*blocklen)[0] = contiguous_type->size;
		break;
		
		case MPC_DATATYPES_DERIVED :
			target_derived_type = sctk_task_specific_get_derived_datatype( task_specific, datatype );
			
			*count = target_derived_type->opt_count;
			*blocklen = malloc( *count * sizeof( long int ) );
			*indices = malloc( *count * sizeof( long int ) );
			
			int i;
			
			for( i = 0 ; i < *count ; i++ )
			{
				(*indices)[i] = target_derived_type->opt_begins[ i ];
				(*blocklen)[i] = target_derived_type->opt_ends[ i ] - target_derived_type->opt_begins[ i ] + 1;
			}
			
		break;
		
		case MPC_DATATYPES_UNKNOWN:
			sctk_fatal("CANNOT PROCESS AN UNKNOWN DATATYPE");
		break;
	}

	return MPI_SUCCESS;
}






int MPIR_Status_set_bytes(MPI_Status *status, MPI_Datatype datatype, MPI_Count nbytes)
{
	MPI_Count type_size;
	PMPI_Type_size_x(datatype, &type_size);
	
	MPI_Count count = 0;
	
	if( type_size != 0 )
	{
		count = nbytes / type_size;
	}
	
	return PMPI_Status_set_elements_x(status, datatype, count);
}

/************************************************************************/
/* Locks                                                                */
/************************************************************************/


sctk_spinlock_t mpio_strided_lock;

void MPIO_lock_strided()
{
	sctk_spinlock_lock(&mpio_strided_lock);
}


void MPIO_unlock_strided()
{
	sctk_spinlock_unlock(&mpio_strided_lock);
}

sctk_spinlock_t mpio_shared_lock;

void MPIO_lock_shared()
{
	sctk_spinlock_lock(&mpio_shared_lock);
}


void MPIO_unlock_shared()
{
	sctk_spinlock_unlock(&mpio_shared_lock);
}

/************************************************************************/
/* Dummy IO requests                                                    */
/************************************************************************/

int PMPIO_Wait(void *r, MPI_Status *s)
{
	return MPI_SUCCESS;
}

int PMPIO_Test(void * r, int * c, MPI_Status *s)
{
	return MPI_SUCCESS;
}


/************************************************************************/
/* C2F/F2C Functions                                                    */
/************************************************************************/

/* These requests conversion functions are needed because
 * the fortran interface in ROMIO reffers to the old IOWait
 * requests, therefore we just have to remap these calls to
 * the classical request management functions as MPIO_Requests
 * are in fact normal MPI_Requests */

MPI_Fint PMPIO_Request_c2f( MPI_Request request )
{
	return MPI_Request_c2f( request );
}


MPI_Request PMPIO_Request_f2c(MPI_Fint rid )
{
	return MPI_Request_f2c( rid );
}

/* File Pointers */

unsigned int MPI_File_fortran_id = 0;

struct MPI_File_Fortran_cell
{
	int id;
	void * fh;
	UT_hash_handle hh;
};

struct MPI_File_Fortran_cell * mpi_file_lookup_table = NULL;
sctk_spinlock_t mpi_file_lookup_table_lock = SCTK_SPINLOCK_INITIALIZER;

MPI_Fint MPIO_File_c2f(void * fh)
{
	MPI_Fint ret = 0;
	
	sctk_spinlock_lock( &mpi_file_lookup_table_lock );

	struct MPI_File_Fortran_cell * new_cell = NULL;
	new_cell = malloc( sizeof( struct MPI_File_Fortran_cell ) );
	assume( new_cell != NULL );
	new_cell->fh = fh;
	new_cell->id = ++MPI_File_fortran_id;
	ret = new_cell->id;
	HASH_ADD_INT( mpi_file_lookup_table, id, new_cell );

	sctk_spinlock_unlock( &mpi_file_lookup_table_lock );
	
	return ret;
}

void * MPIO_File_f2c(int fid)
{
	void * ret = NULL;
	
	sctk_spinlock_lock( &mpi_file_lookup_table_lock );
	
	struct MPI_File_Fortran_cell * previous_cell = NULL;
	
	HASH_FIND_INT( mpi_file_lookup_table, &fid, previous_cell );
	
	if( previous_cell )
		ret = previous_cell->fh;
	
	sctk_spinlock_unlock( &mpi_file_lookup_table_lock );
	
	return ret;
}



