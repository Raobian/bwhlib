
#ifndef __SOCKET_TCP_H__
#define __SOCKET_TCP_H__

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

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


int _inet_addr(struct sockaddr_in *sin, const char *host);
int tcp_sock_tuning(int sd, int tuning, int nonblock);
int tcp_sock_bind(int *srv_sd, struct sockaddr_in *sin, int nonblock, int tuning);
int tcp_sock_hostbind(int *srv_sd, const char *host, const char *service, int nonblock);
int tcp_sock_listen(int *srv_sd, struct sockaddr_in *sin, int qlen, int nonblock, int tuning);
int tcp_sock_hostlisten(int *srv_sd, const char *host, const char *service,
                        int qlen, int nonblock, int tuning);
int tcp_sock_addrlisten(int *srv_sd, uint32_t addr, uint32_t port, int qlen, int nonblock);
int tcp_sock_portlisten(int *srv_sd, uint32_t addr, uint32_t *_port, int qlen, int nonblock);
int tcp_sock_accept(net_handle_t *nh, int srv_sd, int tuning, int nonblock);
int tcp_sock_accept_sd(int *_sd, int srv_sd, int tuning, int nonblock);
int tcp_sock_connect(net_handle_t *nh, struct sockaddr_in *sin, int nonblock, int timeout, int tuning);
int tcp_sock_hostconnect(net_handle_t *nh, const char *host,
                         const char *service, int nonblock, int timeout, int tuning);
int tcp_sock_close(int sd);

#endif
