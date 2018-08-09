#include <stdlib.h>

#include "ocnet_malloc.h"
#include "ocnet_thread.h"

#include "onlfds.h"
#include "onevgrp.h"

#include "tcp_socket.h"
#include "tcp_connection.h"

#include "tcp_server.h"

#define OCNET_MAX_TCP_SERVER_COUNT        32

typedef struct {
    void               *tcp_sock;
    int                 max_connections;
    unsigned char       used:   1;
} tcp_listener_t;

typedef struct {
    unsigned char       internal_evgrp: 1;
    tcp_listener_t     *servers;
    void               *evgrp;
    int                 max_servers;
    int                 cur_server_index;
} tcp_srvmod_t;

static tcp_srvmod_t     g_tcp_srvmod;

static tcp_listener_t *__alloc_listener(void)
{
    int i = 0;

    for (i = 0; i < g_tcp_srvmod.max_servers; i++) {
        tcp_listener_t *server = &g_tcp_srvmod.servers[i];
        if (0 == server->used) {
            server->used = 1;
            return server;
        }
    }

    return NULL;
}

static void __free_listener(tcp_listener_t *listener)
{
    listener->used = 0;
}

static tcp_listener_t *
__create_listener(ocnet_ip_t ip, ocnet_port_t port, int max_connections)
{
    void *tcp_sock = NULL;
    tcp_listener_t *listener = NULL;

    listener = __alloc_listener();
    if (NULL == listener) {
        return NULL;
    }

    tcp_sock = tcp_socket_new(ip, port, max_connections,
            OCNET_EVENT_READ | OCNET_EVENT_WRITE | OCNET_EVENT_ERROR);
    if (NULL == tcp_sock) {
        __free_listener(listener);
        return NULL;
    }

    if (tcp_socket_listen(tcp_sock) < 0) {
        tcp_socket_del(tcp_sock);
        __free_listener(listener);
        return NULL;
    }

    listener->tcp_sock = tcp_sock;
    listener->max_connections = max_connections;

    return listener;
}

static void __destroy_listener(tcp_listener_t *listener)
{
    tcp_socket_del(listener->tcp_sock);
    __free_listener(listener);
}

static void __destroy_listeners(void)
{
    int i = 0;

    for (i = 0; i < g_tcp_srvmod.max_servers; i++) {
        tcp_listener_t *listener = &g_tcp_srvmod.servers[i];
        if (1 == listener->used) {
            __destroy_listener(listener);
        }
    }
}

int tcp_server_init(int internal_evgrp,
        void *evgrp, int max_servers)
{
    g_tcp_srvmod.servers = ocnet_malloc(
            max_servers * sizeof(tcp_listener_t));
    if (NULL == g_tcp_srvmod.servers) {
        return -1;
    }

    ocnet_memset(g_tcp_srvmod.servers, 0x0,
            max_servers * sizeof(tcp_listener_t));
    g_tcp_srvmod.max_servers = max_servers;
    g_tcp_srvmod.cur_server_index = 0;

    if (0 != internal_evgrp) {
        g_tcp_srvmod.internal_evgrp = internal_evgrp;
        g_tcp_srvmod.evgrp = ocnet_evgrp_create(
                OCNET_MAX_TCP_SERVER_COUNT);
        if (NULL == g_tcp_srvmod.evgrp) {
            ocnet_free(g_tcp_srvmod.servers);
            return -1;
        }
    } else {
        if (NULL == evgrp) {
            ocnet_free(g_tcp_srvmod.servers);
            return -1;
        }
        g_tcp_srvmod.evgrp = evgrp;
    }

    return 0;
}

void tcp_server_final(void)
{
    if (0 != g_tcp_srvmod.internal_evgrp) {
        ocnet_evgrp_destroy(g_tcp_srvmod.evgrp);
    }
    __destroy_listeners();
    ocnet_free(g_tcp_srvmod.servers);
}

void *tcp_server_listen(ocnet_ip_t ip, ocnet_port_t port, int max_connections)
{
    tcp_listener_t *listener = NULL;

    listener = __create_listener(ip, port, max_connections);
    if (NULL == listener) {
        return NULL;
    }

    if (tcp_socket_event_enroll(
                listener->tcp_sock,
                g_tcp_srvmod.evgrp) < 0) {
        __destroy_listener(listener);
        return NULL;
    }

    return listener;
}

void tcp_server_remove(void *listener)
{
    tcp_listener_t *tcp_listener = (tcp_listener_t *)listener;
    __destroy_listener(tcp_listener);
}

void *tcp_server_accept(int millseconds)
{
    void *lfds;
    int rc = 0;
    int i = g_tcp_srvmod.cur_server_index;
    void *tcp_conn = NULL;

    lfds = ocnet_lfds_new();
    if (NULL == lfds) {
        return NULL;
    }

    rc = ocnet_evgrp_wait(g_tcp_srvmod.evgrp, millseconds, lfds);
    if (rc < 0) {
        goto L_ERROR;
    } else if (0 < rc) {
        tcp_listener_t *listener = NULL;

        for (i = g_tcp_srvmod.cur_server_index;
                i < g_tcp_srvmod.max_servers;
                i++) {
            listener = &g_tcp_srvmod.servers[i];
            g_tcp_srvmod.cur_server_index =
                (i + 1) % g_tcp_srvmod.max_servers;
            if (0 != listener->used) {
                tcp_conn = tcp_connection_accept(
                        listener->tcp_sock,
                        0, lfds, millseconds);
                if (NULL != tcp_conn) {
                    goto L_END;
                }
            }
        }
    } else {
        goto L_ERROR;
    }

L_ERROR:
L_END:
    ocnet_lfds_del(lfds);
    return tcp_conn;
}
