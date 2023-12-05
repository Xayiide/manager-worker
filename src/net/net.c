#define _POSIC_C_SOURCE 200112L /* solver "addrinfo storage size unknown" */


#include <sys/types.h>  /* ssize_t                                      */
#include <sys/socket.h> /* socket                                       */
#include <netdb.h>      /* getaddrinfo                                  */

#include <netinet/in.h> /* sockaddr_in, INET_ADDRSTRLEN, in_addr, htons */
#include <arpa/inet.h>  /* inet_pton                                    */
#include <stdint.h>     /* uint_t                                       */
#include <stdio.h>      /* fprintf, NULL                                */
#include <stdlib.h>     /* malloc                                       */
#include <string.h>     /* strncpy, memset                              */
#include <unistd.h>     /* close                                        */
#include <sys/time.h>   /* timeval                                      */

#include "inc/net.h"



client net_client_create(char *name, char *service)
{
    client          clnt;
    int             status;
    struct addrinfo hints, *res, *p;
    char            ipstr[INET6_ADDRSTRLEN];

    clnt = malloc(sizeof *(clnt));
    if (clnt == NULL) {
        fprintf(stderr, "[%s:%d] malloc\n", __FILE__, __LINE__);
        return NULL;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET; /* IPv4 */
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(name, service, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "[%s:%d] getaddrinfo: %s\n", __FILE__, __LINE__,
                gai_strerror(status));
        return NULL;
    }

    /* getaddrinfo nos rellena structs con información sobre el destino al que
     * nos vamos a conectar. */
    clnt->dst_saddr = *(struct sockaddr_in *) res->ai_addr;


    freeaddrinfo(res);



    return clnt;
}

void net_client_delete(client *clnt)
{
    if (clnt == NULL)
        return;

    free(*clnt);
}
