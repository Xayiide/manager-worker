#include <stdio.h>      /* printf, perror                */
#include <stdlib.h>     /* exit, atoi, EXIT_SUCCESS, ... */
#include <netdb.h>      /* getaddrinfo                   */
#include <string.h>     /* memset                        */
#include <unistd.h>     /* close                         */
#include <netinet/in.h> /* sockaddr_in, INET6_ADDRSTRLEN */
#include <arpa/inet.h>  /* inet_ntop                     */

#include <poll.h>       /* polling                       */

#include "inc/manager.h"
#include "inc/manager_sm.h"

#define LISTENQUEUE 10


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int manager_init_connections(const char *name, const char *service)
{
    int             ret_tmp;
    int             sockfd;
    struct addrinfo hints, *res;
    char            yes = '1';

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE; /* Usar nuestra IP propia */

    ret_tmp = getaddrinfo(name, service, &hints, &res);
    if (ret_tmp != 0) {
        fprintf(stderr, "getaddrinfo: %s\n",
                gai_strerror(ret_tmp));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo nos rellena structs con información sobre el destino al que
     * nos vamos a conectar. */
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        fprintf(stderr, "socket\n");
        exit(EXIT_FAILURE);
    }

    ret_tmp = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (ret_tmp == -1) {
        fprintf(stderr, "setsockopt\n");
        exit(EXIT_FAILURE);
    }

    ret_tmp = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret_tmp == -1) {
        close(sockfd);
        fprintf(stderr, "bind\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);

    ret_tmp = listen(sockfd, LISTENQUEUE);
    if (ret_tmp == -1) {
        fprintf(stderr, "listen\n");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main(int argc, char *argv[])
{
    int           i;
    int           sockfd;
    struct pollfd pfds[MAX_CLIENTS + 1];

    if (argc != 3) {
        fprintf(stderr, "usage: %s <name> <service>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("[Manager]\n");
    sockfd = manager_init_connections(argv[1], argv[2]);
    if (sockfd == -1)
        exit(EXIT_FAILURE);

    /* Inicializar todos los pfd como inutilizados */
    for (i = 0; i < MAX_CLIENTS + 1; i++) {
        pfds[i].fd      = -1;
        pfds[i].events  = 0;
        pfds[i].revents = 0;
    }

    /* Inicializamos el primer pfd para que sea el del servidor */
    pfds[0].fd     = sockfd;
    pfds[0].events = POLLIN;


    printf("Waiting for connections...\n");
    manager_sm_run(pfds, MAX_CLIENTS);

    return EXIT_SUCCESS;
}
















