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
#include <sys/ioctl.h>  /* ioctl                                        */
#include <errno.h>      /* errno                                        */

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


/* DEFINICIÓN FUNCIONES DE CLIENTE */
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
    client c;

    if (clnt == NULL)
        return;

    c = *clnt;
    close(c->fd);

    free(*clnt);
}

/* DEFINICIÓN FUNCIONES DE CONEXIÓN */
void net_conn_delete(connection *conn)
{
    connection c;

    if (conn == NULL)
        return;

    c = *conn;
    close(c->fd);

    free(*conn);
}


/* DEFINICIÓN FUNCIONES DE SERVIDOR */
server net_server_create(char *service, int backlog)
{
    server              srv;
    struct addrinfo     hints, *res;
    struct sockaddr_in *aux_saddr;
    int                 yes = 1;
    int                 ret_tmp;

    srv = malloc(sizeof *(srv));
    if (srv == NULL) {
        fprintf(stderr, "[%s:%d] malloc\n", __FILE__, __LINE__);
        return NULL;
    }

    memset(&(srv->local_saddr.sin_zero), 0, sizeof(srv->local_saddr.sin_zero));

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE; /* Usar IP Propia */

    ret_tmp = getaddrinfo(NULL, service, &hints, &res);
    if (ret_tmp != 0) {
        fprintf(stderr, "[%s:%d] getaddrinfo: %s\n", __FILE__, __LINE__,
                gai_strerror(ret_tmp));
        return NULL;
    }

    aux_saddr = (struct sockaddr_in *) res->ai_addr;
    srv->local_saddr.sin_family      = res->ai_family;
    srv->local_saddr.sin_port        = aux_saddr->sin_port;
    srv->local_saddr.sin_addr.s_addr = aux_saddr->sin_addr.s_addr;
    srv->local_port                  = ntohs(aux_saddr->sin_port);
    inet_ntop(res->ai_family,
              net_get_in_addr((struct sockaddr *) res->ai_addr),
              srv->local_dir, sizeof(srv->local_dir));

    int pedos;
    pedos = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (pedos == -1) {
        fprintf(stderr, "[%s:%d] socket\n", __FILE__, __LINE__);
        return NULL;
    }

    ret_tmp = setsockopt(pedos, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (ret_tmp < 0) {
        fprintf(stderr, "[%s:%d] setsockopt\n", __FILE__, __LINE__);
        //printf("%s\n", explain_setsockopt(pedos, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)));
        return NULL;
    }

    /* Non-blocking */
    ret_tmp = ioctl(srv->fd, FIONBIO, (char *)&yes);
    if (ret_tmp < 0) {
        fprintf(stderr, "[%s:%d] ioctl\n", __FILE__, __LINE__);
        return NULL;
    }

    ret_tmp = bind(srv->fd, (struct sockaddr *) &(srv->local_saddr),
                   sizeof(struct sockaddr_in));
    if (ret_tmp == 0) {
        (void) listen(srv->fd, backlog);
    }
    else {
        close(srv->fd);
    }

    freeaddrinfo(res);

    return srv;
}

connection net_server_accept(server srv)
{
    connection     conn;
    socklen_t      sin_size;
    int            tmp_ret;
    struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};

    conn = malloc(sizeof *(conn));
    if (conn == NULL) {
        fprintf(stderr, "[%s:%d] malloc\n", __FILE__, __LINE__);
        return NULL;
    }

    sin_size = sizeof(conn->saddr);
    conn->fd = accept(srv->fd, (struct sockaddr *) &(conn->saddr), &sin_size);
    if (conn->fd == -1) {
        fprintf(stderr, "[%s:%d] accept\n", __FILE__, __LINE__);
        return NULL;
    }



    tmp_ret = setsockopt(conn->fd, SOL_SOCKET, SO_RCVTIMEO,
                         &timeout, sizeof timeout);
    if (tmp_ret < 0) {
        fprintf(stderr, "[%s:%d] setsockopt\n", __FILE__, __LINE__);
        return NULL;
    }

    inet_ntop(conn->saddr.sin_family,
              net_get_in_addr((struct sockaddr *) &(conn->saddr)),
              conn->dir, sizeof(conn->dir));
    conn->port = ntohs(conn->saddr.sin_port);

    return conn;
}

int net_server_recv(connection conn, uint8_t *buf, size_t len)
{
    int nbytes = 0;


    return nbytes;
}


void net_server_delete(server *srv)
{
    server s;

    if (srv == NULL)
        return;

    s = *srv;
    close(s->fd);

    free(*srv);
}
