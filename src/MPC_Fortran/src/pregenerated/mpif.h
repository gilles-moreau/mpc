! ############################# MPC License ############################## 
! # Wed Nov 19 15:19:19 CET 2008                                         # 
! # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # 
! #                                                                      # 
! # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # 
! # This file is part of the MPC Runtime.                                # 
! #                                                                      # 
! # This software is governed by the CeCILL-C license under French law   # 
! # and abiding by the rules of distribution of free software.  You can  # 
! # use, modify and/ or redistribute the software under the terms of     # 
! # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # 
! # following URL http://www.cecill.info.                                # 
! #                                                                      # 
! # The fact that you are presently reading this means that you have     # 
! # had knowledge of the CeCILL-C license and that you accept its        # 
! # terms.                                                               # 
! #                                                                      # 
! # Authors:                                                             # 
! #   - BESNARD Jean-Baptiste jbbesnard@paratools.fr                     # 
! #   - AUTOGENERATED by genmod.py                                       # 
! #                                                                      # 
! ######################################################################## 

! /!\ DO NOT EDIT THIS FILE IS AUTOGENERATED BY modulegen.py

       INTEGER MPI_MODE_RDONLY
       PARAMETER (MPI_MODE_RDONLY=2)
       INTEGER MPI_MODE_RDWR
       PARAMETER (MPI_MODE_RDWR=8)
       INTEGER MPI_MODE_WRONLY
       PARAMETER (MPI_MODE_WRONLY=4)
       INTEGER MPI_MODE_CREATE
       PARAMETER (MPI_MODE_CREATE=1)
       INTEGER MPI_MODE_DELETE_ON_CLOSE
       PARAMETER (MPI_MODE_DELETE_ON_CLOSE=16)
       INTEGER MPI_MODE_UNIQUE_OPEN
       PARAMETER (MPI_MODE_UNIQUE_OPEN=32)
       INTEGER MPI_MODE_EXCL
       PARAMETER (MPI_MODE_EXCL=64)
       INTEGER MPI_MODE_APPEND
       PARAMETER (MPI_MODE_APPEND=128)
       INTEGER MPI_MODE_SEQUENTIAL
       PARAMETER (MPI_MODE_SEQUENTIAL=256)
       INTEGER MPI_FILE_NULL
       PARAMETER (MPI_FILE_NULL=0)
       INTEGER MPI_MAX_DATAREP_STRING
       PARAMETER (MPI_MAX_DATAREP_STRING=128)
       INTEGER MPI_SEEK_SET
       PARAMETER (MPI_SEEK_SET=600)
       INTEGER MPI_SEEK_CUR
       PARAMETER (MPI_SEEK_CUR=602)
       INTEGER MPI_SEEK_END
       PARAMETER (MPI_SEEK_END=604)
       INTEGER MPIO_REQUEST_NULL
       PARAMETER (MPIO_REQUEST_NULL=0)
       INTEGER*8 MPI_DISPLACEMENT_CURRENT
       PARAMETER (MPI_DISPLACEMENT_CURRENT=-54278278)
       INTEGER MPI_SOURCE
       PARAMETER (MPI_SOURCE=1)
       INTEGER MPI_TAG
       PARAMETER (MPI_TAG=2)
       INTEGER MPI_ERROR
       PARAMETER (MPI_ERROR=3)
       INTEGER MPI_OFFSET_KIND
       PARAMETER (MPI_OFFSET_KIND=8)
       INTEGER MPI_ADDRESS_KIND
       PARAMETER (MPI_ADDRESS_KIND=8)
       INTEGER MPI_COUNT_KIND
       PARAMETER (MPI_COUNT_KIND=8)
       INTEGER MPI_INTEGER_KIND
       PARAMETER (MPI_INTEGER_KIND=4)
       INTEGER MPI_DISTRIBUTE_DFLT_DARG
       PARAMETER (MPI_DISTRIBUTE_DFLT_DARG=100)
       INTEGER MPI_DISTRIBUTE_BLOCK
       PARAMETER (MPI_DISTRIBUTE_BLOCK=101)
       INTEGER MPI_DISTRIBUTE_CYCLIC
       PARAMETER (MPI_DISTRIBUTE_CYCLIC=102)
       INTEGER MPI_DISTRIBUTE_NONE
       PARAMETER (MPI_DISTRIBUTE_NONE=1)
       INTEGER MPI_ORDER_C
       PARAMETER (MPI_ORDER_C=200)
       INTEGER MPI_ORDER_FORTRAN
       PARAMETER (MPI_ORDER_FORTRAN=201)
       INTEGER MPI_MAX_OBJECT_NAME
       PARAMETER (MPI_MAX_OBJECT_NAME=256)
       INTEGER MPI_MAX_INFO_VAL
       PARAMETER (MPI_MAX_INFO_VAL=1024)
       INTEGER MPI_MAX_INFO_KEY
       PARAMETER (MPI_MAX_INFO_KEY=255)
       INTEGER MPI_MAX_PROCESSOR_NAME
       PARAMETER (MPI_MAX_PROCESSOR_NAME=255)
       INTEGER MPI_MAX_NAME_STRING
       PARAMETER (MPI_MAX_NAME_STRING=256)
       INTEGER MPI_MAX_PORT_NAME
       PARAMETER (MPI_MAX_PORT_NAME=256)
       INTEGER MPI_MAX_ERROR_STRING
       PARAMETER (MPI_MAX_ERROR_STRING=512)
       INTEGER MPI_MAX_KEY_DEFINED
       PARAMETER (MPI_MAX_KEY_DEFINED=7)
       INTEGER MPI_COMBINER_UNKNOWN
       PARAMETER (MPI_COMBINER_UNKNOWN=0)
       INTEGER MPI_COMBINER_NAMED
       PARAMETER (MPI_COMBINER_NAMED=1)
       INTEGER MPI_COMBINER_DUP
       PARAMETER (MPI_COMBINER_DUP=2)
       INTEGER MPI_COMBINER_CONTIGUOUS
       PARAMETER (MPI_COMBINER_CONTIGUOUS=3)
       INTEGER MPI_COMBINER_VECTOR
       PARAMETER (MPI_COMBINER_VECTOR=4)
       INTEGER MPI_COMBINER_HVECTOR
       PARAMETER (MPI_COMBINER_HVECTOR=5)
       INTEGER MPI_COMBINER_INDEXED
       PARAMETER (MPI_COMBINER_INDEXED=6)
       INTEGER MPI_COMBINER_HINDEXED
       PARAMETER (MPI_COMBINER_HINDEXED=7)
       INTEGER MPI_COMBINER_INDEXED_BLOCK
       PARAMETER (MPI_COMBINER_INDEXED_BLOCK=8)
       INTEGER MPI_COMBINER_HINDEXED_BLOCK
       PARAMETER (MPI_COMBINER_HINDEXED_BLOCK=9)
       INTEGER MPI_COMBINER_STRUCT
       PARAMETER (MPI_COMBINER_STRUCT=10)
       INTEGER MPI_COMBINER_SUBARRAY
       PARAMETER (MPI_COMBINER_SUBARRAY=11)
       INTEGER MPI_COMBINER_DARRAY
       PARAMETER (MPI_COMBINER_DARRAY=12)
       INTEGER MPI_COMBINER_F90_REAL
       PARAMETER (MPI_COMBINER_F90_REAL=13)
       INTEGER MPI_COMBINER_F90_INTEGER
       PARAMETER (MPI_COMBINER_F90_INTEGER=15)
       INTEGER MPI_COMBINER_F90_COMPLEX
       PARAMETER (MPI_COMBINER_F90_COMPLEX=14)
       INTEGER MPI_COMBINER_RESIZED
       PARAMETER (MPI_COMBINER_RESIZED=16)
       INTEGER MPI_COMBINER_HINDEXED_INTEGER
       PARAMETER (MPI_COMBINER_HINDEXED_INTEGER=17)
       INTEGER MPI_COMBINER_STRUCT_INTEGER
       PARAMETER (MPI_COMBINER_STRUCT_INTEGER=18)
       INTEGER MPI_COMBINER_HVECTOR_INTEGER
       PARAMETER (MPI_COMBINER_HVECTOR_INTEGER=19)
       INTEGER MPI_REQUEST_NULL
       PARAMETER (MPI_REQUEST_NULL=-1)
       INTEGER MPI_COMM_NULL
       PARAMETER (MPI_COMM_NULL=-1)
       INTEGER MPI_DATATYPE_NULL
       PARAMETER (MPI_DATATYPE_NULL=-1)
       INTEGER MPI_OP_NULL
       PARAMETER (MPI_OP_NULL=-1)
       INTEGER MPI_WIN_NULL
       PARAMETER (MPI_WIN_NULL=-1)
       INTEGER MPI_GROUP_NULL
       PARAMETER (MPI_GROUP_NULL=-1)
       INTEGER MPI_INFO_NULL
       PARAMETER (MPI_INFO_NULL=-1)
       INTEGER MPI_ERRHANDLER_NULL
       PARAMETER (MPI_ERRHANDLER_NULL=-1)
       INTEGER MPI_MESSAGE_NULL
       PARAMETER (MPI_MESSAGE_NULL=-1)
       INTEGER MPI_PROC_NULL
       PARAMETER (MPI_PROC_NULL=-2)
       INTEGER MPI_ARGV_NULL
       PARAMETER (MPI_ARGV_NULL=0)
       INTEGER MPI_ARGVS_NULL
       PARAMETER (MPI_ARGVS_NULL=0)
       INTEGER MPI_UNDEFINED
       PARAMETER (MPI_UNDEFINED=-1)
       INTEGER MPI_COMM_WORLD
       PARAMETER (MPI_COMM_WORLD=0)
       INTEGER MPI_COMM_SELF
       PARAMETER (MPI_COMM_SELF=1)
       INTEGER MPI_CART
       PARAMETER (MPI_CART=-2)
       INTEGER MPI_GRAPH
       PARAMETER (MPI_GRAPH=-3)
       INTEGER MPI_DIST_GRAPH
       PARAMETER (MPI_DIST_GRAPH=-4)
       INTEGER MPI_UNWEIGHTED
       PARAMETER (MPI_UNWEIGHTED=2)
       INTEGER MPI_WEIGHTS_EMPTY
       PARAMETER (MPI_WEIGHTS_EMPTY=3)
       INTEGER MPI_IDENT
       PARAMETER (MPI_IDENT=0)
       INTEGER MPI_CONGRUENT
       PARAMETER (MPI_CONGRUENT=1)
       INTEGER MPI_SIMILAR
       PARAMETER (MPI_SIMILAR=2)
       INTEGER MPI_UNEQUAL
       PARAMETER (MPI_UNEQUAL=3)
       INTEGER MPI_ANY_TAG
       PARAMETER (MPI_ANY_TAG=-1)
       INTEGER MPI_ANY_SOURCE
       PARAMETER (MPI_ANY_SOURCE=-1)
       INTEGER MPI_ROOT
       PARAMETER (MPI_ROOT=-4)
       INTEGER MPI_IN_PLACE
       PARAMETER (MPI_IN_PLACE=-1)
       INTEGER MPI_SUCCESS
       PARAMETER (MPI_SUCCESS=0)
       INTEGER MPI_ERR_BUFFER
       PARAMETER (MPI_ERR_BUFFER=1)
       INTEGER MPI_ERR_COUNT
       PARAMETER (MPI_ERR_COUNT=2)
       INTEGER MPI_ERR_TYPE
       PARAMETER (MPI_ERR_TYPE=3)
       INTEGER MPI_ERR_TAG
       PARAMETER (MPI_ERR_TAG=4)
       INTEGER MPI_ERR_COMM
       PARAMETER (MPI_ERR_COMM=5)
       INTEGER MPI_ERR_RANK
       PARAMETER (MPI_ERR_RANK=6)
       INTEGER MPI_ERR_REQUEST
       PARAMETER (MPI_ERR_REQUEST=7)
       INTEGER MPI_ERR_ROOT
       PARAMETER (MPI_ERR_ROOT=8)
       INTEGER MPI_ERR_GROUP
       PARAMETER (MPI_ERR_GROUP=9)
       INTEGER MPI_ERR_OP
       PARAMETER (MPI_ERR_OP=10)
       INTEGER MPI_ERR_TOPOLOGY
       PARAMETER (MPI_ERR_TOPOLOGY=11)
       INTEGER MPI_ERR_DIMS
       PARAMETER (MPI_ERR_DIMS=12)
       INTEGER MPI_ERR_ARG
       PARAMETER (MPI_ERR_ARG=13)
       INTEGER MPI_ERR_UNKNOWN
       PARAMETER (MPI_ERR_UNKNOWN=14)
       INTEGER MPI_ERR_TRUNCATE
       PARAMETER (MPI_ERR_TRUNCATE=15)
       INTEGER MPI_ERR_OTHER
       PARAMETER (MPI_ERR_OTHER=16)
       INTEGER MPI_ERR_INTERN
       PARAMETER (MPI_ERR_INTERN=17)
       INTEGER MPI_ERR_IN_STATUS
       PARAMETER (MPI_ERR_IN_STATUS=18)
       INTEGER MPI_ERR_PENDING
       PARAMETER (MPI_ERR_PENDING=19)
       INTEGER MPI_ERR_KEYVAL
       PARAMETER (MPI_ERR_KEYVAL=20)
       INTEGER MPI_ERR_NO_MEM
       PARAMETER (MPI_ERR_NO_MEM=21)
       INTEGER MPI_ERR_BASE
       PARAMETER (MPI_ERR_BASE=22)
       INTEGER MPI_ERR_INFO_KEY
       PARAMETER (MPI_ERR_INFO_KEY=23)
       INTEGER MPI_ERR_INFO_VALUE
       PARAMETER (MPI_ERR_INFO_VALUE=24)
       INTEGER MPI_ERR_INFO_NOKEY
       PARAMETER (MPI_ERR_INFO_NOKEY=25)
       INTEGER MPI_ERR_SPAWN
       PARAMETER (MPI_ERR_SPAWN=26)
       INTEGER MPI_ERR_PORT
       PARAMETER (MPI_ERR_PORT=27)
       INTEGER MPI_ERR_SERVICE
       PARAMETER (MPI_ERR_SERVICE=28)
       INTEGER MPI_ERR_NAME
       PARAMETER (MPI_ERR_NAME=29)
       INTEGER MPI_ERR_WIN
       PARAMETER (MPI_ERR_WIN=30)
       INTEGER MPI_ERR_SIZE
       PARAMETER (MPI_ERR_SIZE=31)
       INTEGER MPI_ERR_DISP
       PARAMETER (MPI_ERR_DISP=32)
       INTEGER MPI_ERR_INFO
       PARAMETER (MPI_ERR_INFO=33)
       INTEGER MPI_ERR_LOCKTYPE
       PARAMETER (MPI_ERR_LOCKTYPE=34)
       INTEGER MPI_ERR_ASSERT
       PARAMETER (MPI_ERR_ASSERT=35)
       INTEGER MPI_ERR_RMA_CONFLICT
       PARAMETER (MPI_ERR_RMA_CONFLICT=36)
       INTEGER MPI_ERR_RMA_SYNC
       PARAMETER (MPI_ERR_RMA_SYNC=37)
       INTEGER MPI_ERR_FILE
       PARAMETER (MPI_ERR_FILE=38)
       INTEGER MPI_ERR_NOT_SAME
       PARAMETER (MPI_ERR_NOT_SAME=39)
       INTEGER MPI_ERR_AMODE
       PARAMETER (MPI_ERR_AMODE=40)
       INTEGER MPI_ERR_UNSUPPORTED_DATAREP
       PARAMETER (MPI_ERR_UNSUPPORTED_DATAREP=41)
       INTEGER MPI_ERR_UNSUPPORTED_OPERATION
       PARAMETER (MPI_ERR_UNSUPPORTED_OPERATION=42)
       INTEGER MPI_ERR_NO_SUCH_FILE
       PARAMETER (MPI_ERR_NO_SUCH_FILE=43)
       INTEGER MPI_ERR_FILE_EXISTS
       PARAMETER (MPI_ERR_FILE_EXISTS=44)
       INTEGER MPI_ERR_BAD_FILE
       PARAMETER (MPI_ERR_BAD_FILE=45)
       INTEGER MPI_ERR_ACCESS
       PARAMETER (MPI_ERR_ACCESS=46)
       INTEGER MPI_ERR_NO_SPACE
       PARAMETER (MPI_ERR_NO_SPACE=47)
       INTEGER MPI_ERR_QUOTA
       PARAMETER (MPI_ERR_QUOTA=48)
       INTEGER MPI_ERR_READ_ONLY
       PARAMETER (MPI_ERR_READ_ONLY=49)
       INTEGER MPI_ERR_FILE_IN_USE
       PARAMETER (MPI_ERR_FILE_IN_USE=50)
       INTEGER MPI_ERR_DUP_DATAREP
       PARAMETER (MPI_ERR_DUP_DATAREP=51)
       INTEGER MPI_ERR_CONVERSION
       PARAMETER (MPI_ERR_CONVERSION=52)
       INTEGER MPI_ERR_IO
       PARAMETER (MPI_ERR_IO=53)
       INTEGER MPI_ERR_LASTCODE
       PARAMETER (MPI_ERR_LASTCODE=77)
       INTEGER MPI_NOT_IMPLEMENTED
       PARAMETER (MPI_NOT_IMPLEMENTED=56)
       INTEGER MPI_ERRORS_RETURN
       PARAMETER (MPI_ERRORS_RETURN=-6)
       INTEGER MPI_ERRORS_ARE_FATAL
       PARAMETER (MPI_ERRORS_ARE_FATAL=-7)
       INTEGER MPI_UB
       PARAMETER (MPI_UB=-2)
       INTEGER MPI_LB
       PARAMETER (MPI_LB=-3)
       INTEGER MPI_CHAR
       PARAMETER (MPI_CHAR=13)
       INTEGER MPI_WCHAR
       PARAMETER (MPI_WCHAR=45)
       INTEGER MPI_CHARACTER
       PARAMETER (MPI_CHARACTER=54)
       INTEGER MPI_BYTE
       PARAMETER (MPI_BYTE=1)
       INTEGER MPI_SHORT
       PARAMETER (MPI_SHORT=2)
       INTEGER MPI_INT
       PARAMETER (MPI_INT=3)
       INTEGER MPI_INTEGER
       PARAMETER (MPI_INTEGER=55)
       INTEGER MPI_2INTEGER
       PARAMETER (MPI_2INTEGER=4083)
       INTEGER MPI_INTEGER1
       PARAMETER (MPI_INTEGER1=24)
       INTEGER MPI_INTEGER2
       PARAMETER (MPI_INTEGER2=25)
       INTEGER MPI_INTEGER4
       PARAMETER (MPI_INTEGER4=26)
       INTEGER MPI_INTEGER8
       PARAMETER (MPI_INTEGER8=27)
       INTEGER MPI_AINT
       PARAMETER (MPI_AINT=49)
       INTEGER MPI_COMPLEX8
       PARAMETER (MPI_COMPLEX8=4079)
       INTEGER MPI_COMPLEX16
       PARAMETER (MPI_COMPLEX16=4080)
       INTEGER MPI_COMPLEX32
       PARAMETER (MPI_COMPLEX32=4081)
       INTEGER MPI_INT8_T
       PARAMETER (MPI_INT8_T=34)
       INTEGER MPI_INT16_T
       PARAMETER (MPI_INT16_T=36)
       INTEGER MPI_INT32_T
       PARAMETER (MPI_INT32_T=38)
       INTEGER MPI_INT64_T
       PARAMETER (MPI_INT64_T=40)
       INTEGER MPI_UINT8_T
       PARAMETER (MPI_UINT8_T=35)
       INTEGER MPI_UINT16_T
       PARAMETER (MPI_UINT16_T=37)
       INTEGER MPI_UINT32_T
       PARAMETER (MPI_UINT32_T=39)
       INTEGER MPI_UINT64_T
       PARAMETER (MPI_UINT64_T=41)
       INTEGER MPI_C_BOOL
       PARAMETER (MPI_C_BOOL=53)
       INTEGER MPI_C_FLOAT_COMPLEX
       PARAMETER (MPI_C_FLOAT_COMPLEX=4079)
       INTEGER MPI_C_COMPLEX
       PARAMETER (MPI_C_COMPLEX=4076)
       INTEGER MPI_C_DOUBLE_COMPLEX
       PARAMETER (MPI_C_DOUBLE_COMPLEX=4080)
       INTEGER MPI_C_LONG_DOUBLE_COMPLEX
       PARAMETER (MPI_C_LONG_DOUBLE_COMPLEX=4081)
       INTEGER MPI_LONG
       PARAMETER (MPI_LONG=4)
       INTEGER MPI_LONG_INT
       PARAMETER (MPI_LONG_INT=4071)
       INTEGER MPI_FLOAT
       PARAMETER (MPI_FLOAT=5)
       INTEGER MPI_DOUBLE
       PARAMETER (MPI_DOUBLE=6)
       INTEGER MPI_DOUBLE_PRECISION
       PARAMETER (MPI_DOUBLE_PRECISION=57)
       INTEGER MPI_UNSIGNED_CHAR
       PARAMETER (MPI_UNSIGNED_CHAR=7)
       INTEGER MPI_UNSIGNED_SHORT
       PARAMETER (MPI_UNSIGNED_SHORT=8)
       INTEGER MPI_UNSIGNED
       PARAMETER (MPI_UNSIGNED=9)
       INTEGER MPI_UNSIGNED_LONG
       PARAMETER (MPI_UNSIGNED_LONG=10)
       INTEGER MPI_LONG_DOUBLE
       PARAMETER (MPI_LONG_DOUBLE=11)
       INTEGER MPI_LONG_LONG_INT
       PARAMETER (MPI_LONG_LONG_INT=52)
       INTEGER MPI_LONG_LONG
       PARAMETER (MPI_LONG_LONG=12)
       INTEGER MPI_UNSIGNED_LONG_LONG_INT
       PARAMETER (MPI_UNSIGNED_LONG_LONG_INT=58)
       INTEGER MPI_UNSIGNED_LONG_LONG
       PARAMETER (MPI_UNSIGNED_LONG_LONG=33)
       INTEGER MPI_PACKED
       PARAMETER (MPI_PACKED=0)
       INTEGER MPI_FLOAT_INT
       PARAMETER (MPI_FLOAT_INT=4070)
       INTEGER MPI_DOUBLE_INT
       PARAMETER (MPI_DOUBLE_INT=4072)
       INTEGER MPI_LONG_DOUBLE_INT
       PARAMETER (MPI_LONG_DOUBLE_INT=4078)
       INTEGER MPI_SHORT_INT
       PARAMETER (MPI_SHORT_INT=4073)
       INTEGER MPI_2INT
       PARAMETER (MPI_2INT=4074)
       INTEGER MPI_2FLOAT
       PARAMETER (MPI_2FLOAT=4075)
       INTEGER MPI_COMPLEX
       PARAMETER (MPI_COMPLEX=4076)
       INTEGER MPI_DOUBLE_COMPLEX
       PARAMETER (MPI_DOUBLE_COMPLEX=4082)
       INTEGER MPI_2DOUBLE_PRECISION
       PARAMETER (MPI_2DOUBLE_PRECISION=4077)
       INTEGER MPI_LOGICAL
       PARAMETER (MPI_LOGICAL=22)
       INTEGER MPI_2REAL
       PARAMETER (MPI_2REAL=4084)
       INTEGER MPI_REAL4
       PARAMETER (MPI_REAL4=28)
       INTEGER MPI_REAL8
       PARAMETER (MPI_REAL8=29)
       INTEGER MPI_REAL16
       PARAMETER (MPI_REAL16=30)
       INTEGER MPI_SIGNED_CHAR
       PARAMETER (MPI_SIGNED_CHAR=31)
       INTEGER MPI_REAL
       PARAMETER (MPI_REAL=56)
       INTEGER MPI_COUNT
       PARAMETER (MPI_COUNT=51)
       INTEGER MPI_OFFSET
       PARAMETER (MPI_OFFSET=50)
       INTEGER MPI_SUM
       PARAMETER (MPI_SUM=0)
       INTEGER MPI_MAX
       PARAMETER (MPI_MAX=1)
       INTEGER MPI_MIN
       PARAMETER (MPI_MIN=2)
       INTEGER MPI_PROD
       PARAMETER (MPI_PROD=3)
       INTEGER MPI_LAND
       PARAMETER (MPI_LAND=4)
       INTEGER MPI_BAND
       PARAMETER (MPI_BAND=5)
       INTEGER MPI_LOR
       PARAMETER (MPI_LOR=6)
       INTEGER MPI_BOR
       PARAMETER (MPI_BOR=7)
       INTEGER MPI_LXOR
       PARAMETER (MPI_LXOR=8)
       INTEGER MPI_BXOR
       PARAMETER (MPI_BXOR=9)
       INTEGER MPI_MINLOC
       PARAMETER (MPI_MINLOC=10)
       INTEGER MPI_MAXLOC
       PARAMETER (MPI_MAXLOC=11)
       INTEGER MPI_REPLACE
       PARAMETER (MPI_REPLACE=13)
       INTEGER MPI_NO_OP
       PARAMETER (MPI_NO_OP=14)
       INTEGER MPI_BOTTOM
       PARAMETER (MPI_BOTTOM=0)
       INTEGER MPI_GROUP_EMPTY
       PARAMETER (MPI_GROUP_EMPTY=0)
       INTEGER MPI_KEYVAL_INVALID
       PARAMETER (MPI_KEYVAL_INVALID=-1)
       INTEGER MPI_THREAD_SINGLE
       PARAMETER (MPI_THREAD_SINGLE=0)
       INTEGER MPI_THREAD_FUNNELED
       PARAMETER (MPI_THREAD_FUNNELED=1)
       INTEGER MPI_THREAD_SERIALIZED
       PARAMETER (MPI_THREAD_SERIALIZED=2)
       INTEGER MPI_THREAD_MULTIPLE
       PARAMETER (MPI_THREAD_MULTIPLE=3)
       INTEGER MPI_STATUS_SIZE
       PARAMETER (MPI_STATUS_SIZE=5)
       INTEGER MPI_REQUEST_SIZE
       PARAMETER (MPI_REQUEST_SIZE=1)
       INTEGER MPI_BSEND_OVERHEAD
       PARAMETER (MPI_BSEND_OVERHEAD=4)
       INTEGER MPI_VERSION
       PARAMETER (MPI_VERSION=3)
       INTEGER MPI_SUBVERSION
       PARAMETER (MPI_SUBVERSION=1)
       INTEGER MPI_COMM_TYPE_SHARED
       PARAMETER (MPI_COMM_TYPE_SHARED=1)
       INTEGER MPI_MESSAGE_NO_PROC
       PARAMETER (MPI_MESSAGE_NO_PROC=-2)
       INTEGER MPI_WTIME_IS_GLOBAL
       PARAMETER (MPI_WTIME_IS_GLOBAL=3)
       INTEGER MPI_UNIVERSE_SIZE
       PARAMETER (MPI_UNIVERSE_SIZE=5)
       INTEGER MPI_LASTUSEDCODE
       PARAMETER (MPI_LASTUSEDCODE=6)
       INTEGER MPI_APPNUM
       PARAMETER (MPI_APPNUM=4)
       INTEGER MPI_TAG_UB
       PARAMETER (MPI_TAG_UB=0)
       INTEGER MPI_HOST
       PARAMETER (MPI_HOST=1)
       INTEGER MPI_IO
       PARAMETER (MPI_IO=2)
       INTEGER MPI_WIN_BASE
       PARAMETER (MPI_WIN_BASE=1)
       INTEGER MPI_WIN_SIZE
       PARAMETER (MPI_WIN_SIZE=2)
       INTEGER MPI_WIN_DISP_UNIT
       PARAMETER (MPI_WIN_DISP_UNIT=3)
       INTEGER MPI_WIN_CREATE_FLAVOR
       PARAMETER (MPI_WIN_CREATE_FLAVOR=4)
       INTEGER MPI_WIN_MODEL
       PARAMETER (MPI_WIN_MODEL=0)
       INTEGER MPI_WIN_FLAVOR_CREATE
       PARAMETER (MPI_WIN_FLAVOR_CREATE=1)
       INTEGER MPI_WIN_FLAVOR_ALLOCATE
       PARAMETER (MPI_WIN_FLAVOR_ALLOCATE=2)
       INTEGER MPI_WIN_FLAVOR_DYNAMIC
       PARAMETER (MPI_WIN_FLAVOR_DYNAMIC=3)
       INTEGER MPI_WIN_FLAVOR_SHARED
       PARAMETER (MPI_WIN_FLAVOR_SHARED=4)
       INTEGER MPI_WIN_SEPARATE
       PARAMETER (MPI_WIN_SEPARATE=1)
       INTEGER MPI_WIN_UNIFIED
       PARAMETER (MPI_WIN_UNIFIED=0)
       INTEGER MPI_MODE_NOCHECK
       PARAMETER (MPI_MODE_NOCHECK=1)
       INTEGER MPI_MODE_NOSTORE
       PARAMETER (MPI_MODE_NOSTORE=2)
       INTEGER MPI_MODE_NOPUT
       PARAMETER (MPI_MODE_NOPUT=4)
       INTEGER MPI_MODE_NOPRECEDE
       PARAMETER (MPI_MODE_NOPRECEDE=8)
       INTEGER MPI_MODE_NOSUCCEED
       PARAMETER (MPI_MODE_NOSUCCEED=16)
       INTEGER MPI_LOCK_EXCLUSIVE
       PARAMETER (MPI_LOCK_EXCLUSIVE=234)
       INTEGER MPI_LOCK_SHARED
       PARAMETER (MPI_LOCK_SHARED=235)
       integer MPI_STATUS_IGNORE(5)
       COMMON MPI_STATUS_IGNORE
       integer MPI_STATUSES_IGNORE(5,1)
       COMMON MPI_STATUSES_IGNORE
       EXTERNAL MPI_NULL_DELETE_FN
       EXTERNAL MPI_NULL_COPY_FN
       EXTERNAL MPI_DUP_FN
       EXTERNAL MPI_TYPE_NULL_DELETE_FN
       EXTERNAL MPI_TYPE_NULL_COPY_FN
       EXTERNAL MPI_TYPE_DUP_FN
       EXTERNAL MPI_COMM_NULL_DELETE_FN
       EXTERNAL MPI_COMM_NULL_COPY_FN
       EXTERNAL MPI_COMM_DUP_FN
       EXTERNAL MPI_WIN_NULL_DELETE_FN
       EXTERNAL MPI_WIN_NULL_COPY_FN
       EXTERNAL MPI_WIN_DUP_FN
       EXTERNAL MPI_WTICK
       EXTERNAL PMPI_WTICK
       EXTERNAL MPI_WTIME
       EXTERNAL PMPI_WTIME
       DOUBLE PRECISION MPI_WTIME
       DOUBLE PRECISION PMPI_WTIME
       DOUBLE PRECISION MPI_WTICK
       DOUBLE PRECISION PMPI_WTICK