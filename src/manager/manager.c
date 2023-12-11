#include <stdio.h>
#include <stdlib.h>      /* atoi   */
#include <string.h>      /* memset */
#include <unistd.h>      /* close  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <poll.h>

#include "net/net.h"

#define MAX_CLIENTS 50
#define LISTENQ     5
#define BUF_SIZE    128

int main(int argc, char *argv[])
{
    struct sockaddr_in listenaddr;
    int                listenfd;

    struct sockaddr_in remoteaddr;
    socklen_t          addrlen;
    int                newfd, port;

    struct pollfd      pfds[MAX_CLIENTS];
    int                fd_count = 0;
    int                poll_count;

    char               buf[BUF_SIZE];
    int                i, nbytes;

    server             srv; // = malloc(sizeof *(srv));


    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number %d\n", port);
        return 1;
    }

    //srv->fd = socket(AF_INET, SOCK_STREAM, 0);
    //if (srv->fd < 0) {
    //    fprintf(stderr, "socket\n");
    //    return 1;
    //}

    //listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //if (listenfd < 0) {
    //    fprintf(stderr, "socket\n");
    //    return 1;
    //}
    srv = net_server_create(argv[1], LISTENQ);
    printf("Listening socket: %d\n", srv->fd);

    //memset(&(srv->local_saddr), 0, sizeof(srv->fd));
    //srv->local_saddr.sin_family      = AF_INET;
    //srv->local_saddr.sin_addr.s_addr = INADDR_ANY;
    //srv->local_saddr.sin_port        = htons(port);

    //if (bind(srv->fd, (struct sockaddr *) &(srv->local_saddr),
    //         sizeof(srv->local_saddr)) < 0) {
    //    perror("bind");
    //    return 1;
    //}

    //if (listen(srv->fd, LISTENQ) < 0) {
    //    fprintf(stderr, "listen\n");
    //    return 1;
    //}
    
    for (i = 1; i < MAX_CLIENTS; i++) {
        pfds[i].fd = -1;
    }

    pfds[0].fd     = srv->fd;
    pfds[0].events = POLLRDNORM;

    while (1) {
        poll_count = poll(pfds, fd_count + 1, -1);

        if (poll_count <= 0)
            continue;

        if (pfds[0].revents & POLLRDNORM) {
            addrlen = sizeof(remoteaddr);
            
            newfd = accept(srv->fd, (struct sockaddr *) &remoteaddr, &addrlen);
            if (newfd < 0) {
                fprintf(stderr, "accept\n");
                return 1;
            }

            printf("New connection %d from %s:%hu\n", newfd,
                    inet_ntoa(remoteaddr.sin_addr),
                    ntohs(remoteaddr.sin_port));

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (pfds[i].fd < 0) {
                    pfds[i].fd = newfd;
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                fprintf(stderr, "Demasiados clientes [%d]\n", MAX_CLIENTS);
                close(newfd);
            }

            pfds[i].events = POLLRDNORM;

            if (i > fd_count)
                fd_count = i;

            if (--poll_count <= 0)
                continue;
        }

        for (i = 1; i <= fd_count; i++) {
            newfd = pfds[i].fd;
            if (newfd < 0) /* Ignora fds a -1 */
                continue;

            if (pfds[i].revents & (POLLRDNORM | POLLERR)) {
                nbytes = recv(newfd, buf, BUF_SIZE, 0);

                if (nbytes <= 0) {
                    if (nbytes < 0)
                        perror("read");
                    else if (nbytes == 0)
                        printf("Close socket %d\n", newfd);
                    close(newfd);
                    pfds[i].fd = -1;
                }
                else {
                    printf("Read %d bytes from socket %d\n", nbytes, newfd);
                }


                if (--poll_count <= 0)
                    break;
            }
        }
    }

    free(srv);
    close(srv->fd);
    
    return 0;
}
