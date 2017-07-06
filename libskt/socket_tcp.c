/* ************************************************************************
 *       Filename:  socket_tcp.c
 *    Description:  
 *        Version:  1.0
 *        Created:  07/05/2017 05:35:44 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "socket_tcp.h"



int _inet_addr(struct sockaddr_in *sin, const char *host)
{
        int ret, retry = 0, herrno = 0;
        struct hostent  hostbuf, *result;
        char buf[MAX_BUF_LEN];

retry:
        ret = gethostbyname_r(host, &hostbuf, buf, sizeof(buf),  &result, &herrno);
        if (ret) {
                ret = errno;
                if (ret == EALREADY || ret == EAGAIN) {
                        printf("[warnning] connect addr %s\n", host);
                        ret = EAGAIN;
                        if (retry < 50) {
                                usleep(100 * 1000);
                                retry++;
                                goto retry;
                        } else
                                goto err_ret;
                } else
                        goto err_ret;
        }

        if (result)
                memcpy(&sin->sin_addr, result->h_addr, result->h_length);
        else {
                ret = ENONET;
                printf("[warnning] connect addr %s ret (%u) %s\n", host, ret, strerror(ret));
                goto err_ret;
        }

        return 0;
err_ret:
        return ret;
}

static int __tcp_connect(int s, const struct sockaddr *sin, socklen_t addrlen, int timeout)
{
        int  ret, flags, err;
        socklen_t len;

        flags = fcntl(s, F_GETFL, 0);
        if (flags < 0 ) {
                ret = errno;
                goto err_ret;
        }

        ret = fcntl(s, F_SETFL, flags | O_NONBLOCK);
        if (ret < 0 ) {
                ret = errno;
                goto err_ret;
        }

        ret = connect(s, sin, addrlen);
        if (ret < 0 ) {
                ret = errno;
                if (ret != EINPROGRESS ) {
                        goto err_ret;
                }
        } else
                goto out;

        /*
         *ret = sock_poll_sd(s, timeout * 1000 * 1000, POLLOUT);
         *if (ret) {
         *        goto err_ret;
         *}
         */

        len = sizeof(err);

        ret = getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len);
        if (ret < 0)
                goto err_ret;

        if (err) {
                ret = err;
                goto err_ret;
        }

out:
        ret = fcntl(s, F_SETFL, flags);
        if (ret < 0) {
                ret = errno;
                goto err_ret;
        }

        return 0;
err_ret:
        return ret;
}

static int __tcp_accept(int s, struct sockaddr *sin, socklen_t *addrlen, int timeout)
{
        int  ret, fd;

        (void) timeout;

        fd = accept(s, sin, addrlen);
        if (fd < 0 ) {
                ret = errno;
                goto err_ret;
        }

        return fd;
err_ret:
        return -ret;
}

int tcp_sock_bind(int *srv_sd, struct sockaddr_in *sin, int nonblock, int tuning)
{
        int ret, sd, reuse;
        struct protoent ppe, *result;
        char buf[MAX_BUF_LEN];

        /* map protocol name to protocol number */
        ret = getprotobyname_r(NET_TRANSPORT, &ppe, buf, MAX_BUF_LEN, &result);
        if (ret) {
//                ret = ENOENT;
                printf("%s", "can't get \"tcp\" protocol entry\n");
                goto err_ret;
        }

        /* allocate a socket */
        sd = socket(PF_INET, SOCK_STREAM, ppe.p_proto);
        if (sd == -1) {
                ret = errno;
                printf("proto %d name %s\n", ppe.p_proto, ppe.p_name);
                goto err_ret;
        }

        if (tuning) {
                /*ret = tcp_sock_tuning(sd, 1, nonblock);*/
                /*if (ret)*/
                        /*GOTO(err_sd, ret);*/
        }

        reuse = 1;
        ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
        if (ret == -1) {
                ret = errno;
                goto err_sd;
        }

        /* bind the socket */
        ret = bind(sd, (struct sockaddr *)sin, sizeof(struct sockaddr));
        if (ret == -1) {
                ret = errno;
                printf("bind %d to %s errno (%d)%s\n", ntohs(sin->sin_port), inet_ntoa(sin->sin_addr), ret, strerror(ret));
                goto err_sd;
        }

        *srv_sd = sd;

        return 0;
err_sd:
        close(sd);
err_ret:
        return ret;
}


int tcp_sock_hostbind(int *srv_sd, const char *host, const char *service, int nonblock)
{
        int ret;
        struct servent  *pse;
        struct sockaddr_in sin;

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

        if (host) {
                ret = _inet_addr(&sin, host);
                if (ret)
                        goto err_ret;
        } else
                sin.sin_addr.s_addr = INADDR_ANY;

        if ((pse = getservbyname(service, "tcp")))
                sin.sin_port = pse->s_port;
        else if ((sin.sin_port=htons((unsigned short)atoi(service))) == 0) {
                printf("[error] can't get \"%s\" service entry\n", service);
                ret = ENOENT;
                goto err_ret;
        }

        ret = tcp_sock_bind(srv_sd, &sin, nonblock, 1);
        if (ret)
                goto err_ret;

        return 0;
err_ret:
        return ret;
}


int tcp_sock_listen(int *srv_sd, struct sockaddr_in *sin, int qlen, int nonblock, int tuning)
{
        int ret, sd;

        ret = tcp_sock_bind(&sd, sin, nonblock, tuning);
        if (ret)
                goto err_ret;


        ret = listen(sd, qlen);
        if (ret == -1) {
                ret = errno;
                goto err_sd;
        }

        *srv_sd = sd;

        return 0;
err_sd:
        close(sd);
err_ret:
        return ret;
}


int tcp_sock_hostlisten(int *srv_sd, const char *host, const char *service,
                        int qlen, int nonblock, int tuning)
{
        int ret;
        struct servent  *pse;
        struct sockaddr_in sin;

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

        if (host) {
                ret = _inet_addr(&sin, host);
                if (ret)
                        goto err_ret;
        } else
                sin.sin_addr.s_addr = INADDR_ANY;

        if ((pse = getservbyname(service, "tcp")))
                sin.sin_port = pse->s_port;
        else if ((sin.sin_port=htons((unsigned short)atoi(service))) == 0) {
                printf("[error] can't get \"%s\" service entry\n", service);
                ret = ENOENT;
                goto err_ret;
        }

        ret = tcp_sock_listen(srv_sd, &sin, qlen, nonblock, tuning);
        if (ret) {
                goto err_ret;
        }

        return 0;
err_ret:
        return ret;
}


int tcp_sock_addrlisten(int *srv_sd, uint32_t addr, uint32_t port, int qlen, int nonblock)
{
        int ret;
        struct sockaddr_in sin;

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

        if (addr != 0)
                sin.sin_addr.s_addr = addr;
        else
                sin.sin_addr.s_addr = INADDR_ANY;

        sin.sin_port = htons(port);

        ret = tcp_sock_listen(srv_sd, &sin, qlen, nonblock, 1);
        if (ret)
                goto err_ret;

        return 0;
err_ret:
        return ret;
}

/**
 * 随机返回一个port
 * */
int tcp_sock_portlisten(int *srv_sd, uint32_t addr, uint32_t *_port, int qlen, int nonblock)
{
        int ret;
        struct sockaddr_in sin;
        uint16_t port = 0;

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

        if (addr != 0)
                sin.sin_addr.s_addr = addr;
        else
                sin.sin_addr.s_addr = INADDR_ANY;

        while (1) {
                port = (uint16_t)(NET_SERVICE_BASE
                                  + (random() % NET_SERVICE_RANGE));

                sin.sin_port = htons(port);

                ret = tcp_sock_listen(srv_sd, &sin, qlen, nonblock, 1);
                if (ret) {
                        if (ret == EADDRINUSE) {
                                printf("port (%u + %u) %s\n", NET_SERVICE_BASE,
                                     port - NET_SERVICE_BASE, strerror(ret));
                                continue;
                        }
                        goto err_ret;

                } else
                        break;
        }

        *_port = port;

        return 0;
err_ret:
        return ret;
}


int tcp_sock_accept(net_handle_t *nh, int srv_sd, int tuning, int nonblock)
{
        int ret, sd;
        struct sockaddr_in sin;
        socklen_t alen;

        memset(&sin, 0, sizeof(sin));
        alen = sizeof(struct sockaddr_in);

        sd = __tcp_accept(srv_sd, (struct sockaddr *)&sin, &alen, 60 / 2);
        if (sd < 0) {
                ret = -sd;
                printf("[error] srv_sd %d, %u\n", srv_sd, ret);
                goto err_ret;
        }

        /*
         *ret = tcp_sock_tuning(sd, tuning, nonblock);
         *if (ret)
         *        goto err_ret;
         */

        memset(nh, 0x0, sizeof(*nh));
        nh->type = NET_HANDLE_TRANSIENT;
        nh->u.sd.sd = sd;
        nh->u.sd.addr = sin.sin_addr.s_addr;

        return 0;
err_sd:
        close(sd);
err_ret:
        return ret;
}

int tcp_sock_accept_sd(int *_sd, int srv_sd, int tuning, int nonblock)
{
        int ret, sd;
        struct sockaddr_in sin;
        socklen_t alen;

        memset(&sin, 0, sizeof(sin));
        alen = sizeof(struct sockaddr_in);

        sd = __tcp_accept(srv_sd, (struct sockaddr *)&sin, &alen, 60 / 2);
        if (sd < 0) {
                ret = -sd;
                printf("[error] srv_sd %d\n", srv_sd);
                goto err_ret;
        }

        /*
         *ret = tcp_sock_tuning(sd, tuning, nonblock);
         *if (ret)
         *        goto err_ret;
         */

        *_sd = sd;

        return 0;
err_sd:
        close(sd);
err_ret:
        return ret;
}


int tcp_sock_connect(net_handle_t *nh, struct sockaddr_in *sin, int nonblock, int timeout, int tuning)
{
        int ret, sd;

        sd = socket(PF_INET, SOCK_STREAM, 0);
        if (sd == -1) {
                ret = errno;
                goto err_ret;
        }

        //ret = connect(sd, (struct sockaddr*)sin, sizeof(struct sockaddr));
        ret = __tcp_connect(sd, (struct sockaddr*)sin, sizeof(struct sockaddr),
                            timeout);
        if (ret) {
                goto err_sd;
        }

        if (tuning) {
                /*
                 *ret = tcp_sock_tuning(sd, 1, nonblock);
                 *if (ret) {
                 *        goto err_sd;
                 */
        }

        printf("[debug] new sock %d connected\n", sd);
        nh->u.sd.sd = sd;
        nh->u.sd.addr = sin->sin_addr.s_addr;
        //sock->proto = ng.op;

        return 0;
err_sd:
        close(sd);
err_ret:
        return ret;
}


int tcp_sock_hostconnect(net_handle_t *nh, const char *host,
                         const char *service, int nonblock, int timeout, int tuning)
{
        int ret;
        struct servent  *pse;
        struct sockaddr_in sin;

        memset(&sin, 0, sizeof(struct sockaddr_in));
        sin.sin_family = AF_INET;

        if ((pse = getservbyname(service, "tcp")))
                sin.sin_port = pse->s_port;
        else if ((sin.sin_port = htons(atoi(service))) == 0) {
                ret = ENOENT;
                printf("[error] get port from service (%s)\n", service);
                assert(0);
                goto err_ret;
        }

        ret = _inet_addr(&sin, host);
        if (ret)
                goto err_ret;

        printf("[info] host %s service %s --> ip %s port %d\n",
             host, service, 
             inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

        ret = tcp_sock_connect(nh, &sin, nonblock, timeout, tuning);
        if (ret) {
                goto err_ret;
        }

        return 0;
err_ret:
        return ret;
}


int tcp_sock_close(int sd)
{
        int ret;
        while (1) {
                ret = close(sd);
                if (ret == -1) {
                        ret = errno;

                        if(ret == EINTR)
                                continue;
                        goto err_ret;
                } else
                        break;
        }
        return 0;
err_ret:
        return ret;
}


void show_sd(int sd)
{
        unsigned char *p __attribute__((unused));
        struct sockaddr addr;
        socklen_t addrlen;
        getsockname(sd, &addr, &addrlen);
        p = (unsigned char*)&(((struct sockaddr_in*)&addr)->sin_addr);
        printf("sd %d, host %d.%d.%d.%d\n", sd, *p, *(p+1), *(p+2), *(p+3));
}

int test_hostlisten()
{
        int tcp_sd;
        char port[MAX_BUF_LEN];
        snprintf(port, MAX_BUF_LEN, "%u", 17903);
        tcp_sock_hostlisten(&tcp_sd, NULL, port, 256, 0, 1);

        show_sd(tcp_sd);

        tcp_sock_close(tcp_sd);
        
}

void test_portlisten()
{
        int sd, acc_sd;
        uint32_t port = 0;
        char host[64];
        memset(host, 0x0, 64);

        gethostname(host, 64);
        printf("host %s\n", host);

        tcp_sock_portlisten(&sd, 0, &port, 4, 0);
        show_sd(sd);

        while (1) {
                printf("----\n");
                tcp_sock_accept_sd(&acc_sd, sd, 1, 0);
                show_sd(acc_sd);
        }

        tcp_sock_close(sd);

}

void test_connect()
{
        net_handle_t *nh;
        char port[MAX_BUF_LEN];
        char host[64];

        nh = (net_handle_t *)malloc(sizeof(net_handle_t));
        memset(nh, 0x0, sizeof(net_handle_t));

        snprintf(port, MAX_BUF_LEN, "%u", 17903);

        memset(host, 0x0, 64);
        gethostname(host, 64);

        printf("connect to %s:%s\n", host, port);

        tcp_sock_hostconnect(nh, host, port, 0, 60, 1);
        sleep(3);
        printf("nh.u.sd.sd %d\n", nh->u.sd.sd);
        tcp_sock_close(nh->u.sd.sd);
        
}

int main(int argc, char *argv[])
{
        if (argc > 1 && !strcmp(argv[1], "ser")) {
                printf("run as ser\n");
                test_portlisten();
        } else {
               test_connect(); 
        }

        return 0;
}
