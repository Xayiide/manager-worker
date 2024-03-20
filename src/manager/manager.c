#include <stdio.h>  /* printf, perror                */
#include <stdlib.h> /* exit, atoi, EXIT_SUCCESS, ... */
#include <netdb.h>  /* getaddrinfo                   */
#include <string.h> /* memset                        */
#include <unistd.h> /* close                         */
#include <netinet/in.h> /* sockaddr_in, INET_ADDRSTRLEN */
#include <arpa/inet.h>  /* inet_ntop                    */

#define LISTENQUEUE 10

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{

    struct addrinfo     hints, *res;
    char                yes = 1;
    int                 new_fd, sockfd;
    struct sockaddr_storage their_addr;
    char s[INET6_ADDRSTRLEN];

    int                 ret_tmp;

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


    printf("Waiting for connections...\n");
    while (1) {
        socklen_t sin_size;
        sin_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            fprintf(stderr, "accept\n");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  s, sizeof(s));
        printf("server: got connection from %s\n", s);

    }


    return EXIT_SUCCESS;
}
















