/* ************************************************************************
 *       Filename:  test_libskt.c
 *    Description:  
 *        Version:  1.0
 *        Created:  07/06/2017 10:59:38 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include "socket_tcp.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>



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
        int sd, acc_sd;
        char port[MAX_BUF_LEN];
        snprintf(port, MAX_BUF_LEN, "%u", 17903);
        tcp_sock_hostlisten(&sd, NULL, port, 256, 0, 1);

        show_sd(sd);

        printf("listen  port %u\n", port);
        while (1) {
                tcp_sock_accept_sd(&acc_sd, sd, 0, 0);
                printf("accept %d\n", acc_sd);
                show_sd(acc_sd);
                sleep(3);
                tcp_sock_close(acc_sd);
        }

        tcp_sock_close(sd);
        
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
        /*gethostname(host, 64);*/
        sprintf(host, "%s", "node3");

        printf("connect to %s:%s\n", host, port);

        tcp_sock_hostconnect(nh, host, port, 0, 60, 1);
        sleep(3);
        printf("get a new nh sd %d\n", nh->u.sd.sd);
        tcp_sock_close(nh->u.sd.sd);
        
}

int main(int argc, char *argv[])
{
        if (argc > 1 && !strcmp(argv[1], "ser")) {
                printf("run as ser\n");
                test_hostlisten();
        } else {
               test_connect(); 
        }

        return 0;
}


