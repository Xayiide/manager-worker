#include <stdio.h>      /* printf, perror                */
#include <stdlib.h>     /* exit, atoi, EXIT_SUCCESS, ... */
#include <netdb.h>      /* getaddrinfo                   */
#include <string.h>     /* memset                        */
#include <unistd.h>     /* close                         */
#include <netinet/in.h> /* sockaddr_in, INET6_ADDRSTRLEN */
#include <arpa/inet.h>  /* inet_ntop                     */

#include <poll.h>       /* polling                       */

#define LISTENQUEUE 10
#define MAX_CLIENTS 10

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int i;
    int ret_tmp;

    struct addrinfo         hints, *res;
    char                    yes = 1;
    int                     new_fd, sockfd;
    struct sockaddr_storage their_addr;
    char                    s[INET6_ADDRSTRLEN];

    struct pollfd pfds[MAX_CLIENTS];
    int           poll_count = 0;
    int           fd_highest = 0;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <name> <service>\n", argv[0]);
        return EXIT_FAILURE;
    }



    printf("[Manager]\n");

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE; /* Usar nuestra IP propia */

    ret_tmp = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (ret_tmp != 0) {
        fprintf(stderr, "getaddrinfo: %s\n",
                gai_strerror(ret_tmp));
        return EXIT_FAILURE;
    }

    /* getaddrinfo nos rellena structs con información sobre el destino al que
     * nos vamos a conectar. */
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        fprintf(stderr, "socket\n");
        return EXIT_FAILURE;
    }

    ret_tmp = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (ret_tmp == -1) {
        fprintf(stderr, "setsockopt\n");
        return EXIT_FAILURE;
    }

    ret_tmp = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret_tmp == -1) {
        close(sockfd);
        fprintf(stderr, "bind\n");
        return EXIT_FAILURE;
    }

    freeaddrinfo(res);

    ret_tmp = listen(sockfd, LISTENQUEUE);
    if (ret_tmp == -1) {
        fprintf(stderr, "listen\n");
        return EXIT_FAILURE;
    }

    /* Inicializar todos los pfd como inutilizados */
    for (i = 0; i < MAX_CLIENTS; i++)
        pfds[i].fd = -1;
    /* Inicializamos el primer pfd para que sea el del servidor */
    pfds[0].fd     = sockfd;
    pfds[0].events = POLLIN;

    printf("Waiting for connections...\n");
    socklen_t sin_size = sizeof(their_addr);
    while (1) {
        poll_count = poll(pfds, fd_highest + 1, -1);
        if (poll_count <= 0)
            continue;

        /* evento de input en el server (0) */
        if (pfds[0].revents & POLLIN) { 
            new_fd = accept(sockfd, (struct sockaddr *) &their_addr,
                            &sin_size);
            if (new_fd == -1) {
                fprintf(stderr, "accept\n");
                continue;
            }

            inet_ntop(their_addr.ss_family,
                      get_in_addr((struct sockaddr *) &their_addr),
                      s, sizeof(s));
            printf("server: got connection from %s\n", s);

            /* Encuentro el primer pfd que esté inutilizado y lo utilizo */
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (pfds[i].fd < 0) {
                    pfds[i].fd = new_fd;
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                fprintf(stderr, "Demasiados clientes [%d]\n", MAX_CLIENTS);
                close(new_fd);
            }

            /* Marcar el nuevo fd como escuchando cambios en INPUT */
            pfds[i].events = POLLIN;

            if (i < fd_highest)
                fd_highest = i;

            poll_count--;
            if (poll_count <= 0)
                continue;
        }

        /* Compruebo eventos de escritura en el resto de fds */
        int  nbytes;
        char buf[100];
        int  auxfd;
        for (i = 1; i <= fd_highest; i++) {
            auxfd = pfds[i].fd;
            /* Ignora fds a -1 */
            if (auxfd < 0)
                continue;

            if (pfds[i].revents & (POLLIN | POLLERR)) {
                nbytes = recv(auxfd, buf, 100, 0);

                if (nbytes <= 0) {
                    if (nbytes < 0) {
                        fprintf(stderr, "recv\n");
                    }
                    else if (nbytes == 0) {
                        printf("Close socket %d\n", auxfd);
                    }
                    close(auxfd);
                    pfds[i].fd = -1;
                }
                else {
                    printf("recv: Work to do!\n");
                }

                poll_count--;
                if (poll_count <= 0)
                    break;
            }
        }
    }

    return EXIT_SUCCESS;
}
















