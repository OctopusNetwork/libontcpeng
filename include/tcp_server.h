#ifndef __ONC_TCP_SERVER____H__
#define __ONC_TCP_SERVER____H__

#include "ocnet_iport.h"

#ifdef __cplusplus
extern "C" {
#endif

int     tcp_server_init(int internal_evgrp, void *evgrp, int max_servers);
void    tcp_server_final(void);
void   *tcp_server_listen(ocnet_ip_t ip, ocnet_port_t port, int max_connections);
void    tcp_server_remove(void *listener);
void   *tcp_server_accept(int millseconds);

#ifdef __cplusplus
}
#endif

#endif
