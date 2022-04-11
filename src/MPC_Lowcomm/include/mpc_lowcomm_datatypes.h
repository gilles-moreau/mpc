#ifndef MPC_LOWCOMM_DATATYPES
#define MPC_LOWCOMM_DATATYPES

#include<stdio.h>
#include<stdint.h>

typedef struct{
    int typesize;
    char typename[64];
} mpc_lowcomm_datatype;

/* ******************
 * Common datatypes *
 * *****************/

typedef enum{
    MPC_LOWCOMM_PACKED,
    MPC_LOWCOMM_BYTE,
    MPC_LOWCOMM_SHORT,
    MPC_LOWCOMM_INT,
    MPC_LOWCOMM_LONG,
    MPC_LOWCOMM_FLOAT,
    MPC_LOWCOMM_DOUBLE,
    MPC_LOWCOMM_UNSIGNED_CHAR,
    MPC_LOWCOMM_UNSIGNED_SHORT,
    MPC_LOWCOMM_UNSIGNED,
    MPC_LOWCOMM_UNSIGNED_LONG,
    MPC_LOWCOMM_LONG_DOUBLE,
    MPC_LOWCOMM_LONG_LONG,
    MPC_LOWCOMM_CHAR,
    MPC_LOWCOMM_LOGICAL,
    MPC_LOWCOMM_INTEGER1,
    MPC_LOWCOMM_INTEGER2,
    MPC_LOWCOMM_INTEGER4,
    MPC_LOWCOMM_INTEGER8,
    MPC_LOWCOMM_REAL4,
    MPC_LOWCOMM_REAL8,
    MPC_LOWCOMM_REAL16,
    MPC_LOWCOMM_SIGNED_CHAR,
    MPC_LOWCOMM_UNSIGNED_LONG_LONG,
    MPC_LOWCOMM_INT8_T,
    MPC_LOWCOMM_UINT8_T,
    MPC_LOWCOMM_INT16_T,
    MPC_LOWCOMM_UINT16_T,
    MPC_LOWCOMM_INT32_T,
    MPC_LOWCOMM_UINT32_T,
    MPC_LOWCOMM_INT64_T,
    MPC_LOWCOMM_UINT64_T,
    MPC_LOWCOMM_WCHAR,
    MPC_LOWCOMM_INTEGER16,
    MPC_LOWCOMM_AINT,
    MPC_LOWCOMM_OFFSET,
    MPC_LOWCOMM_COUNT,
    MPC_LOWCOMM_LONG_LONG_INT,
    MPC_LOWCOMM_C_BOOL,
    MPC_LOWCOMM_CHARACTER,
    MPC_LOWCOMM_INTEGER,
    MPC_LOWCOMM_REAL,
    MPC_LOWCOMM_DOUBLE_PRECISION,
    MPC_LOWCOMM_UNSIGNED_LONG_LONG_INT,
    MPC_LOWCOMM_TYPE_COMMON_LIMIT
} mpc_lowcomm_type_common;

/* ****************************
 * common datatypes functions *
 * ***************************/

/**
 * @brief initializes common datatypes
 *
 * @return MPI_SUCCESS on success
 */

int mpc_lowcomm_datatype_common_init();

/**
 * @brief get the size of a datatype
 * 
 * @param datatype datatype to get the size of
 * @return MPI_SUCCESS on success
 */

int mpc_lowcomm_datatype_common_get_size(int datatype);

/**
 * @brief get the name of a common datatype
 * 
 * @param datatype datatype to get the name of
 * @return MPI_SUCCESS on success
 */

char* mpc_lowcomm_datatype_common_get_name(int datatype);

/**
 * @brief set the name of a common datatype
 * 
 * @param datatype datatype to set the name of
 * @param name new name of the datatype
 * @return MPI_SUCCESS on success
 */

int mpc_lowcomm_datatype_common_set_name(int datatype, char *name);

#endif