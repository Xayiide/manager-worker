#include <stdio.h>      /* printf, perror                */
#include <stdlib.h>     /* exit, EXIT_SUCCESS            */
#include <netdb.h>      /* getaddrinfo                   */
#include <string.h>     /* memset                        */
#include <unistd.h>     /* close, sleep                  */
#include <netinet/in.h> /* sockaddr_in, INET6_ADDRSTRLEN */
#include <arpa/inet.h>  /* inet_ntop                     */



void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main (int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int             sockfd;
    char            s[INET6_ADDRSTRLEN];

    int             ret_tmp;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <server name> <server port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET; /* IPv4. AF_UNSPEC for any version */
    hints.ai_socktype = SOCK_STREAM;

    ret_tmp = getaddrinfo(argv[1], argv[2], &hints, &res);
    if (ret_tmp != 0) {
        fprintf(stderr, "getaddrinfo\n");
        return EXIT_FAILURE;
    }

    if (res == NULL) {
        fprintf(stderr, "getaddrinfo - res was NULL\n");
        return EXIT_FAILURE;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        fprintf(stderr, "socket\n");
        return EXIT_FAILURE;
    }
    
    ret_tmp = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (ret_tmp == -1) {
        close(sockfd);
        fprintf(stderr, "connect\n");
        return EXIT_FAILURE;
    }

    inet_ntop(res->ai_family, get_in_addr((struct sockaddr *) res->ai_addr),
              s, sizeof(s));
    printf("client: connecting to %s\n", s);

    freeaddrinfo(res);

    int nbytes;
    char buf[100];
    while (1) {
        nbytes = recv(sockfd, buf, 100, 0);
        if (nbytes <= 0) {
            if (nbytes < 0)
                fprintf(stderr, "read\n");
            else if (nbytes == 0)
                fprintf(stderr, "Server closed connection\n");
            break;
        }
    }
    
    close(sockfd);

    return EXIT_SUCCESS;
}
