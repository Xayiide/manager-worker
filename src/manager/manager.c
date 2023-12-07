#include <stdio.h>
#include <stdlib.h>     /* exit, EXIT_FAILURE, atoi */
#include <unistd.h>     /* sleep                    */
#include <poll.h>       /* poll */
#include <errno.h>      /* errno, EWOULDBLOCK */
#include <string.h>     /* memset */

#include "net/net.h"

void add_to_pfds(struct pollfd *pfds, int fd, int *fd_count)
{
    pfds[*fd_count].fd     = fd;
    pfds[*fd_count].events = POLLIN;

    (*fd_count)++;
}

void del_from_pfds(struct pollfd *pfds, int i, int *fd_count)
{
    /* Copia el último en la posición actual */
    pfds[i] = pfds[(*fd_count)-1];
    pfds[(*fd_count)-1].fd = 0;
    (*fd_count)--;
}


int main(int argc, char *argv[])
{
    server        srv;
    connection    conn;
    int           backlog = 50;
    struct pollfd pfds[backlog + 1];
    int           fd_count = 0, new_fd;

    memset(pfds, 0, sizeof(pfds));
    conn = malloc(sizeof *(conn));
    if (conn == NULL) {
        fprintf(stderr, "MALLOC\n");
        return -1;
    }

    printf("[Manager]\n");

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        return 1;
    }

    srv = net_server_create(argv[1], backlog);
    if (srv == NULL) {
        fprintf(stderr, "No se ha podido crear el servidor\n");
        return 1;
    }
    printf("Server listening on %s:%d\n", srv->local_dir, srv->local_port);

    fd_count++;
    pfds[0].fd     = srv->fd;
    pfds[0].events = POLLIN;

    while (1) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count < 0) {
            fprintf(stderr, "poll\n");
            return 1;
        }

        if (poll_count == 0) {
            fprintf(stderr, "poll timeout\n");
            return 1;
        }

        for (int i = 0; i < fd_count; i++) {
            if (pfds[i].revents & POLLIN) { /* Este fd tiene lectura lista */
                if (pfds[i].fd == srv->fd) { /* Es el servidor */
                    net_server_accept(srv, conn);
                    new_fd = conn->fd;
                    pfds[fd_count].fd     = new_fd;
                    pfds[fd_count].events = POLLIN;
                    fd_count++;
                    printf("Conexión desde %s:%d\n",
                               conn->dir, conn->port);
                }
                else { /* Es el cliente */
                    char buf[128];
                    int nbytes    = recv(pfds[i].fd, buf, sizeof buf, 0);
                    int sender_fd = pfds[i].fd;

                    if (nbytes <= 0) { /* Error o conexión cerrada */
                        if (nbytes == 0) /* Conexión cerrada */
                            printf("Socket %d cerró\n", sender_fd);
                        else
                            fprintf(stderr, "recv\n");

                        close(pfds[i].fd);
                        pfds[i].fd = -1;
                        //del_from_conns(conns, i-1, &conns_count);
                    }
                    else {
                        printf("Recibido: %s\n", buf);
                    }
                }
            }
        }

    }

    net_conn_delete(&conn);
    net_server_delete(&srv);

    return 0;
}

