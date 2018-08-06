#include <signal.h>
#include <stdio.h>

#include "onlfds.h"

#include "tcp_connection.h"

#include "tcp_server.h"

static int  g_running = 0;

static void __sigint_handler(int sig)
{
    g_running = 0;
}

int main(int argc, char *argv[])
{
    void *listener1 = NULL;
    void *listener2 = NULL;
    void *tcp_conn = NULL;
    void *lfds = NULL;

    if (tcp_server_init(1, NULL, 32) < 0) {
        return -1;
    }

    listener1 = tcp_server_listen(onc_iport_parse_ip("127.0.0.1"), 10688, 1000);
    if (NULL == listener1) {
        goto L_ERROR_LISTEN_1;
    }

    listener2 = tcp_server_listen(onc_iport_parse_ip("127.0.0.1"), 10689, 1000);
    if (NULL == listener2) {
        goto L_ERROR_LISTEN_2;
    }

    signal(SIGINT, __sigint_handler);
    g_running = 1;

    lfds = onc_lfds_new();

    do {
        tcp_conn = tcp_server_accept(1000);
        if (NULL != tcp_conn) {
            int rc = 0;
            char buf[1024];
            int recv_result = 0;

            rc = tcp_connection_event_wait(tcp_conn, lfds, 1000);
            if (0 < rc) {
                if (0 != tcp_connection_readable(tcp_conn, lfds)) {
                    recv_result = tcp_connection_read(tcp_conn, buf, sizeof(buf));
                    if (0 < recv_result) {
                        printf("Client msg: %s\n", buf);
                        tcp_connection_write(tcp_conn, "world", 6);
                    }
                }
            } else if (rc < 0) {
                printf("Error---\n");
            } else {
                printf("Connection timeout---\n");
            }
        } else {
            printf("Server timeout---\n");
        }
    } while (1 == g_running);

    onc_lfds_del(lfds);
    tcp_server_remove(listener2);
L_ERROR_LISTEN_2:
    tcp_server_remove(listener1);
L_ERROR_LISTEN_1:
    tcp_server_final();
    return 0;
}
