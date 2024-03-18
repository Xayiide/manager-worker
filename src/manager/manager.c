#include <stdio.h>  /* printf, perror                */
#include <stdlib.h> /* exit, atoi, EXIT_SUCCESS, ... */
#include <netdb.h>  /* getaddrinfo                   */
#include <netinet/in.h> /* sockaddr_in, INET_ADDRSTRLEN */

#define LISTENQUEUE 10

int main(int argc, char *argv[])
{

    struct addrinfo     hints, *res;
    char                yes = 1;
    int                 ret_tmp;
    int                 new_fd, sockfd;
    struct sockaddr_storage their_addr;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <name> <service>\n", argv[0]);
        exit(EXIT_FAILURE);
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
        new_fd = accept(sockfd, )

    }


    return EXIT_SUCCESS;
}
















