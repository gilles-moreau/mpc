#ifndef SCTK_PTL_IFACE_H_
#define SCTK_PTL_IFACE_H_

#include <portals4.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "sctk_ptl_types.h"
#include "sctk_debug.h"

#ifdef VERY_VERBOSE
#define sctk_ptl_chk(x) do { int __ret = x; \
    sctk_debug("%s -> %s (%s:%u)", #x, sctk_ptl_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); \
    switch (__ret) { \
	case PTL_EQ_EMPTY: \
	case PTL_CT_NONE_REACHED: \
        case PTL_OK: break; \
	default: \
		 sctk_abort(); \
    } } while (0)
#elif !defined(NDEBUG)
#define sctk_ptl_chk(x) do { int __ret = 0; \
    switch (__ret = x) { \
	case PTL_EQ_EMPTY: \
	case PTL_CT_NONE_REACHED: \
        case PTL_OK: break; \
	default: sctk_fatal("%s -> %s (%s:%u)", #x, sctk_ptl_rc_decode(__ret), __FILE__, (unsigned int)__LINE__); break; \
    } } while (0)
#else
#define sctk_ptl_chk(x) x
#endif

/**************************************************************/
/************************** FUNCTIONS *************************/
/**************************************************************/

/* Hardware-related init */
void sctk_ptl_hardware_init();
void sctk_ptl_hardware_fini();

/* Software-related init */
sctk_ptl_pte_t* sctk_ptl_software_init(int);
void sctk_ptl_software_fini(sctk_ptl_pte_t*);

/* ME management */
sctk_ptl_me_t* sctk_ptl_me_create(void*, size_t, sctk_ptl_id_t, sctk_ptl_matchbits_t, sctk_ptl_matchbits_t, int);
sctk_ptl_meh_t* sctk_ptl_me_register(sctk_ptl_me_t*, sctk_ptl_pte_t*, ptl_list_t);
void sctk_ptl_me_release(sctk_ptl_meh_t*);

/* MD management */
sctk_ptl_md_t* sctk_ptl_md_create(void*, size_t, int);
sctk_ptl_mdh_t* sctk_ptl_md_register(const sctk_ptl_md_t*);
void sctk_ptl_md_release(sctk_ptl_mdh_t*);

/* EQ management */
int sctk_ptl_eq_poll_md(sctk_ptl_event_t*);
int sctk_ptl_eq_poll_me(sctk_ptl_pte_t*, sctk_ptl_event_t*);

/* Request management */
int sctk_ptl_emit_get(sctk_ptl_mdh_t* mdh, size_t size, sctk_ptl_id_t remote, sctk_ptl_pte_t* pte, sctk_ptl_matchbits_t match);
int sctk_ptl_emit_push();
int sctk_ptl_emit_atomic();
int sctk_ptl_emit_fetch_atomic();


/**************************************************************/
/*************************** HELPERS **************************/
/**************************************************************/
sctk_ptl_id_t sctk_ptl_self();

static inline const char const * sctk_ptl_rc_decode(int rc)
{
	switch(rc)
	{
		case PTL_OK: return "PTL_OK"; break;
		case PTL_FAIL: return "PTL_FAIL"; break;
		case PTL_IN_USE: return "PTL_IN_USE"; break;
		case PTL_NO_INIT: return "PTL_NO_INIT"; break;
		case PTL_IGNORED: return "PTL_IGNORED"; break;
		case PTL_PID_IN_USE: return "PTL_PID_IN_USE"; break;
		case PTL_INTERRUPTED: return "PTL_INTERRUPTED"; break;
		
		case PTL_NO_SPACE: return "PTL_NO_SPACE"; break;
		case PTL_ARG_INVALID: return "PTL_ARG_INVALID"; break;
		
		case PTL_PT_IN_USE: return "PTL_PT_IN_USE"; break;
		case PTL_PT_FULL: return "PTL_PT_FULL"; break;
		case PTL_PT_EQ_NEEDED: return "PTL_PT_EQ_NEEDED"; break;
		
		case PTL_LIST_TOO_LONG: return "PTL_LIST_TOO_LONG"; break;
		case PTL_EQ_EMPTY: return "PTL_EQ_EMPTY"; break;
		case PTL_EQ_DROPPED: return "PTL_EQ_DROPPED"; break;
		case PTL_CT_NONE_REACHED: return "PTL_CT_NONE_REACHED"; break;
		default:
		{
			char* buf = sctk_malloc(sizeof(char) * 40);
			snprintf(buf, 40, "Portals return code not known: %d", rc); 
			return buf;
			break;
		}
	}
	return NULL;
}

static inline const char const * sctk_ptl_event_decode(sctk_ptl_event_t ev)
{
	switch(ev.type)
	{
	case PTL_EVENT_GET: return "PTL_EVENT_GET"; break;
	case PTL_EVENT_GET_OVERFLOW: return "PTL_EVENT_GET_OVERFLOW"; break;
	
	case PTL_EVENT_PUT: return "PTL_EVENT_PUT"; break;
	case PTL_EVENT_PUT_OVERFLOW: return "PTL_EVENT_PUT_OVERFLOW"; break;
	
	case PTL_EVENT_ATOMIC: return "PTL_EVENT_ATOMIC"; break;
	case PTL_EVENT_ATOMIC_OVERFLOW: return "PTL_EVENT_ATOMIC_OVERFLOW"; break;
	
	case PTL_EVENT_FETCH_ATOMIC: return "PTL_EVENT_FETCH_ATOMIC"; break;
	case PTL_EVENT_FETCH_ATOMIC_OVERFLOW: return "PTL_EVENT_FETCH_ATOMIC_OVERFLOW"; break;
	case PTL_EVENT_REPLY: return "PTL_EVENT_REPLY"; break;
	case PTL_EVENT_SEND: return "PTL_EVENT_SEND"; break;
	case PTL_EVENT_ACK: return "PTL_EVENT_ACK"; break;
	
	case PTL_EVENT_PT_DISABLED: return "PTL_EVENT_PT_DISABLED"; break;
	case PTL_EVENT_LINK: return "PTL_EVENT_LINK"; break;
	case PTL_EVENT_AUTO_UNLINK: return "PTL_EVENT_AUTO_UNLINK"; break;
	case PTL_EVENT_AUTO_FREE: return "PTL_EVENT_AUTO_FREE"; break;
	case PTL_EVENT_SEARCH: return "PTL_EVENT_SEARCH"; break;
	default:
		return "Portals Event not known"; break;
	}

	return NULL;
}

/**
 * De-serialize an object an map it into its base struct.
 *
 * \param[in] inval the serialized data, as a string
 * \param[out] outval the effective struct to fill
 * \param[in] outvallen size of the final struct
 * \return 1 if an error occured, 0 otherwise
 */
static inline int sctk_ptl_data_deserialize ( const char *inval, void *outval, int outvallen )
{
    int i;
    char *ret = ( char * ) outval;

    if ( outvallen != strlen ( inval ) / 2 )
    {
        return 1;
    }

    for ( i = 0 ; i < outvallen ; ++i )
    {
        if ( *inval >= '0' && *inval <= '9' )
        {
            ret[i] = *inval - '0';
        }
        else
        {
            ret[i] = *inval - 'a' + 10;
        }

        inval++;

        if ( *inval >= '0' && *inval <= '9' )
        {
            ret[i] |= ( ( *inval - '0' ) << 4 );
        }
        else
        {
            ret[i] |= ( ( *inval - 'a' + 10 ) << 4 );
        }

        inval++;
    }

    return 0;
}

/**
 * Serialize a object/struct into a string.
 * \param[in] inval the object to serialize
 * \param[in] invallen input object size
 * \param[out] outval the string version of the object
 * \param[in] outvallen the max string length
 * \return 1 if an error occured, 0 otherwise
 */
static inline int sctk_ptl_data_serialize ( const void *inval, int invallen, char *outval, int outvallen )
{
    static unsigned char encodings[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', \
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
    int i;
	if ( invallen * 2 + 1 > outvallen )
    {
        return 1;
    }

    for ( i = 0; i < invallen; i++ )
    {
        outval[2 * i] = encodings[ ( ( unsigned char * ) inval ) [i] & 0xf];
        outval[2 * i + 1] = encodings[ ( ( unsigned char * ) inval ) [i] >> 4];
    }

    outval[invallen * 2] = '\0';
    return 0;
}
#endif

