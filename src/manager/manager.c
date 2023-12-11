#include <stdio.h>
#include <stdlib.h>      /* atoi   */
#include <string.h>      /* memset */
#include <unistd.h>      /* close  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <poll.h>

#include <time.h> /* BORRAR: timeval */

#include "net/net.h"

#define MAX_CLIENTS 50
#define LISTENQ     5
#define BUF_SIZE    128

int main(int argc, char *argv[])
{
    struct pollfd      pfds[MAX_CLIENTS]; /* storing the server fd (pfds[0]) */
    int                fd_highest = 0;

    int                poll_count;
    char               buf[BUF_SIZE];
    int                i, nbytes;
    int                aux_fd, port;

    server             srv;
    connection         conn;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number %d\n", port);
        return 1;
    }

    srv = net_server_create(argv[1], LISTENQ);
    printf("Listening socket: %d\n", srv->fd);
    
    for (i = 1; i < MAX_CLIENTS; i++) {
        pfds[i].fd = -1;
    }

    pfds[0].fd     = srv->fd;
    pfds[0].events = POLLIN;

    while (1) {
        poll_count = poll(pfds, fd_highest + 1, -1);

        if (poll_count <= 0)
            continue;

        if (pfds[0].revents & POLLIN) {
            conn = net_server_accept(srv);
            printf("New connection %d from %s:%hu\n",
                    conn->fd, conn->dir, conn->port);

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (pfds[i].fd < 0) {
                    pfds[i].fd = conn->fd;
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                fprintf(stderr, "Demasiados clientes [%d]\n", MAX_CLIENTS);
                close(conn->fd);
            }

            pfds[i].events = POLLIN;

            if (i > fd_highest)
                fd_highest = i;

            poll_count--;
            if (poll_count <= 0)
                continue;
        }

        for (i = 1; i <= fd_highest; i++) {
            aux_fd = pfds[i].fd;
            if (aux_fd < 0) /* Ignora fds a -1 */
                continue;

            if (pfds[i].revents & (POLLIN | POLLERR)) {
                nbytes = recv(aux_fd, buf, BUF_SIZE, 0);

                if (nbytes <= 0) {
                    if (nbytes < 0)
                        perror("read");
                    else if (nbytes == 0)
                        printf("Close socket %d\n", aux_fd);
                    close(aux_fd);
                    pfds[i].fd = -1;
                }
                else {
                    /* Handle clients */
                    printf("Read %d bytes from socket %d\n", nbytes, aux_fd);
                }


                poll_count--;
                if (poll_count <= 0)
                    break;
            }
        }
    }

    net_server_delete(&srv);
    
    return 0;
}
