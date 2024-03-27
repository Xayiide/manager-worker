#include <stdio.h>      /* printf, perror                */
#include <stdlib.h>     /* exit, EXIT_SUCCESS            */
#include <netdb.h>      /* getaddrinfo                   */
#include <string.h>     /* memset                        */
#include <unistd.h>     /* close, sleep                  */
#include <netinet/in.h> /* sockaddr_in, INET6_ADDRSTRLEN */
#include <arpa/inet.h>  /* inet_ntop                     */

#include <poll.h>       /* polling                        */

#include "inc/worker_sm.h"

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int readline(uint8_t *buf, int maxlen)
{
    fgets((char *) buf, maxlen, stdin);

    return strlen((char *) buf);
}

/* Devuelve el sockfd o -1 si hay error */
int worker_init_connections(const char *name, const char *service)
{
    int             ret_tmp;
    int             sockfd;
    struct addrinfo hints, *res;
    char            s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET; /* IPv4. AF_UNSPEC for any version */
    hints.ai_socktype = SOCK_STREAM;


    ret_tmp = getaddrinfo(name, service, &hints, &res);
    if (ret_tmp != 0) {
        fprintf(stderr, "getaddrinfo\n");
        return -1;
    }

    if (res == NULL) {
        fprintf(stderr, "getaddrinfo - res was NULL\n");
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        fprintf(stderr, "socket\n");
        return -1;
    }

    ret_tmp = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret_tmp == -1) {
        close(sockfd);
        fprintf(stderr, "connect\n");
        return -1;
    }

    inet_ntop(res->ai_family, get_in_addr((struct sockaddr *) res->ai_addr),
              s, sizeof(s));
    printf("client: connecting to %s\n", s);

    freeaddrinfo(res);

    return sockfd;
}



int main (int argc, char *argv[])
{
    int sockfd;

    struct pollfd pfds[2]; /* stdin y el socket */
    int           poll_count = 0;


    if (argc != 3) {
        fprintf(stderr, "usage: %s <server name> <server port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    sockfd = worker_init_connections(argv[1], argv[2]);
    if (sockfd == -1)
        exit(EXIT_FAILURE);

    pfds[0].fd     = 0; /* stdin */
    pfds[0].events = POLLIN;

    pfds[1].fd     = sockfd;
    pfds[1].events = POLLIN;
    

    worker_sm_run(pfds, 2);

    int     nbytes;
    uint8_t stdin_buf[100];
    uint8_t recv_buf[100];
    while (1) {
        poll_count = poll(pfds, 2, -1);
        if (poll_count <= 0)
            continue;

        /* Evento de input en stdin (pfds[0]) (se ha pulsado ENTER) */
        if (pfds[0].revents & POLLIN) {
            nbytes = readline(stdin_buf, 100);
            nbytes = send(pfds[1].fd, stdin_buf, nbytes, 0); /* Cambiar esto wtf */
            if (nbytes == -1) {
                fprintf(stderr, "send.\n");
                continue;
            }
        }

        /* Evento de input en el socket (pfds[1]) */
        if (pfds[1].revents & POLLIN) {
            nbytes = recv(pfds[1].fd, recv_buf, 100, 0);
            if (nbytes <= 0) {
                if (nbytes < 0)
                    fprintf(stderr, "read\n");
                else if (nbytes == 0)
                    fprintf(stderr, "Server closed connection\n");
                break;
            }
            else {
                printf("%s", recv_buf);
            }
        }

        poll_count--;
        if (poll_count <= 0)
            continue;
    }
    
    close(sockfd);

    return EXIT_SUCCESS;
}
