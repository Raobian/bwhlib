
#ifndef __SOCKET_TCP_H__
#define __SOCKET_TCP_H__

#include <stdint.h>

#define MAX_BUF_LEN (1024 * 64)
#define NET_TRANSPORT "tcp"
#define NET_SERVICE_BASE 49152 
#define NET_SERVICE_RANGE (65535 - NET_SERVICE_BASE)

typedef enum {
        NET_HANDLE_NULL,
        NET_HANDLE_PERSISTENT,  /* Constant connect */
        NET_HANDLE_TRANSIENT,   /* Temporary connect */
} net_handle_type_t;

typedef struct {
        uint32_t addr;
        uint32_t seq;
        int sd;
        int type;
} sockid_t;

typedef struct {
        int16_t id;
} nid_t;


typedef struct {
        net_handle_type_t type;
        union {
                nid_t nid;
                sockid_t  sd;
        } u;
} net_handle_t;


#endif
