#include <stdio.h>
#include <stdlib.h>     /* exit, EXIT_FAILURE, atoi */
#include <unistd.h>     /* sleep                    */
#include <poll.h>       /* poll */
#include <errno.h>      /* errno */
#include <string.h>     /* memset */

#include "net/net.h"

/* TODO: Temporalmente globales */
//int        BACKLOG = 50;
#define BACKLOG 50
connection conns[BACKLOG];

void add_to_pfds(struct pollfd *pfds, connection conn, int *fd_count)
{
    int found = 0;
    int index = 0;
    int fd    = conn->fd;

    /* Busca el primer struct no usado */
    while (found == 0) {
        if (pfds[index].fd == -1) {
            found = 1;
            pfds[index].fd     = fd;
            pfds[index].events = POLLIN;
            conns[index - 1] = conn;
            (*fd_count)++;
        }
        else {
            index++;
        }
    }
}

void del_from_pfds(struct pollfd *pfds, int i, int *fd_count)
{
    pfds[i].fd       = -1;
    pfds[i].events   = 0;
    pfds[i].revents  = 0;
    conns[i - 1]->fd = -1;
    (*fd_count)--;
}


int main(int argc, char *argv[])
{
    server        srv;
    connection    aux_conn;
    struct pollfd pfds[BACKLOG + 1];
    int           fd_count = 0;
    int           run_srv  = 1;
    int i;

    /* Inicializa pfds a un estado correcto: los fds a un valor inválido */
    memset(pfds, 0, sizeof(pfds));
    for (i = 0; i < BACKLOG + 1; i++) {
        pfds[i].fd = -1;
    }

    printf("[Manager]\n");

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <puerto>\n", argv[0]);
        return 1;
    }

    srv = net_server_create(argv[1], BACKLOG);
    if (srv == NULL) {
        fprintf(stderr, "No se ha podido crear el servidor\n");
        return 1;
    }
    printf("Server listening on %s:%d\n", srv->local_dir, srv->local_port);

    fd_count++;
    pfds[0].fd     = srv->fd;
    pfds[0].events = POLLIN;

    while (run_srv == 1) {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count < 0) {
            fprintf(stderr, "poll\n");
            return 1;
        }

        if (poll_count == 0) {
            fprintf(stderr, "poll timeout\n");
            return 1;
        }

        for (i = 0; i < fd_count; i++) {
            if (pfds[i].revents == 0)
                continue;
            if (pfds[i].revents != POLLIN) {
                fprintf(stderr, "Error. Revents: %d\n", pfds[i].revents);
                run_srv = 0;
                break;
            }
            if (pfds[i].fd == srv->fd) { /* Es el servidor */
                if ((fd_count - 1) < BACKLOG) {
                    aux_conn = net_server_accept(srv);
                    if (aux_conn == NULL) {
                        fprintf(stderr, "Error: net_server_accept\n");
                        return 1;
                    }
                    add_to_pfds(pfds, aux_conn, &fd_count);
                    printf("Conexión desde %s:%d\n",
                               aux_conn->dir, aux_conn->port);
                }
                else {
                    printf("Máximo número de conexiones alcanzado\n");
                    printf(" BACKLOG: %d\n", BACKLOG);
                    printf(" fd_count: %d\n", fd_count);
                }
            }
            else { /* Es el cliente */
                char buf[128];
                int nbytes    = recv(pfds[i].fd, buf, sizeof buf, 0);
                int sender_fd = pfds[i].fd;

                if (nbytes <= 0) { /* Error o conexión cerrada */
                    if (nbytes == 0) { /* Conexión cerrada */
                        printf("Socket %d cerró\n", sender_fd);
                    }
                    else {
                        fprintf(stderr, "recv: ");
                        fprintf(stderr, "%s\n", strerror(errno));
                    }

                    close(pfds[i].fd);
                    del_from_pfds(pfds, i, &fd_count);
                }
                else {
                    printf("Recibido: %s\n", buf);
                }
            }
        }

    }

    for (i = 0; i < BACKLOG; i++)
        net_conn_delete(&conns[i]);
    net_conn_delete(&aux_conn);
    net_server_delete(&srv);

    return 0;
}

