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

/* Declaración de funciones estáticas */
static void *net_get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    else {
        return &(((struct sockaddr_in6 *)sa)->sin6_addr);
    }
}


client net_client_create(char *name, char *service)
{
    client              clnt;
    struct addrinfo     hints, *res;
    struct sockaddr_in *aux_saddr;
    char                yes = 1;
    int                 ret_tmp;

    clnt = malloc(sizeof *(clnt));
    if (clnt == NULL) {
        fprintf(stderr, "[%s:%d] malloc\n", __FILE__, __LINE__);
        return NULL;
    }
    memset(&(clnt->dst_saddr.sin_zero), 0,
        sizeof(clnt->dst_saddr.sin_zero));
    memset(&(clnt->local_saddr.sin_zero), 0,
        sizeof(clnt->local_saddr.sin_zero));

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET; /* IPv4. AF_UNSPEC for any version */
    hints.ai_socktype = SOCK_STREAM;

    ret_tmp = getaddrinfo(name, service, &hints, &res);
    if (ret_tmp != 0) {
        fprintf(stderr, "[%s:%d] getaddrinfo: %s\n", __FILE__, __LINE__,
                gai_strerror(ret_tmp));
        return NULL;
    }

    /* getaddrinfo nos rellena structs con información sobre el destino al que
     * nos vamos a conectar. */
    aux_saddr = (struct sockaddr_in *) res->ai_addr;
    clnt->dst_saddr.sin_family      = res->ai_family;
    clnt->dst_saddr.sin_port        = aux_saddr->sin_port;
    clnt->dst_saddr.sin_addr.s_addr = aux_saddr->sin_addr.s_addr;
    clnt->dst_port                  = ntohs(aux_saddr->sin_port);
    inet_ntop(res->ai_family,
              net_get_in_addr((struct sockaddr *) res->ai_addr),
              clnt->dst_dir, sizeof(clnt->dst_dir));

    clnt->fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (clnt->fd == -1) {
        fprintf(stderr, "[%s:%d] socket\n", __FILE__, __LINE__);
        return NULL;
    }

    ret_tmp = connect(clnt->fd, (struct sockaddr *) &(clnt->dst_saddr),
            sizeof(struct sockaddr_in));
    if (ret_tmp == 0) {
        (void) setsockopt(clnt->fd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(yes));
    }
    else {
        close(clnt->fd);
    }

    freeaddrinfo(res);

    return clnt;
}

void net_client_delete(client *clnt)
{
    if (clnt == NULL)
        return;

    free(*clnt);
}
