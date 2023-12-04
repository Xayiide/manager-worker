#include <stdio.h>
#include <stdlib.h>    /* exit, EXIT_FAILURE */
#include <unistd.h>    /* sleep              */
#include <sys/types.h> /* ssize_t            */

#include "net/net.h"

int main()
{
    server_t     srv;
    connection_t conn;

    printf("[Manager]\n");

    srv = net_server_new("127.0.0.1", 42069);

    if (net_server_listen(srv, 1) == -1) {
        printf("Error.\n");
        exit(EXIT_FAILURE);
    }

    char    buf[128];
    ssize_t recv_size;
    while (1) {
        conn      = net_server_accept(srv);
        recv_size = net_conn_recv(conn, buf, 128);

        if (recv_size == -1) {
            printf("net_conn_recv returned -1\n");
            break;
        }
        else {
            printf("Received msg: [%s]\n", buf);
        }

        if (buf[0] == 'A') {
            printf("Client sent an A. Stopping\n");
            break;
        }
    }
    
    net_conn_delete(&conn);
    net_server_delete(&srv);

    return 0;
}

