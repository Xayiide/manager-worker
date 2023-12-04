#include <netinet/in.h> /* sockaddr_in, INET_ADDRSTRLEN, in_addr, htons */
#include <arpa/inet.h>  /* inet_pton                                    */
#include <stdint.h>     /* uint_t                                       */
#include <stdio.h>      /* fprintf, NULL                                */
#include <stdlib.h>     /* malloc                                       */
#include <string.h>     /* strncpy, memset                              */
#include <unistd.h>     /* close                                        */
#include <sys/socket.h> /* socket                                       */
#include <sys/types.h>  /* ssize_t                                      */
#include <sys/time.h>   /* timeval                                      */
#include <netdb.h>      /* getaddrinfo                                  */

#include "inc/net.h"


int net_client_create(char *name, char *service)
{
    int             ret = 0;
    int             status;
    struct addrinfo hints, *res, *p;
    char            ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(name, service, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "[%s:%d] getaddrinfo: %s\n", __FILE__, __LINE__,
                gai_strerror(status));
        return -1;
    }

    printf("[DEBUG] IP para %s: [", name);
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
            addr  = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
        else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) p->ai_addr;
            addr  = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        printf("%s:%s]\n", ipver, ipstr);
    }

    freeaddrinfo(res);
}
