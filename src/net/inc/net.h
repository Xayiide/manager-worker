#ifndef NET_H_
#define NET_H_

#include <stdint.h>     /* uint_t */
#include <netinet/in.h> /* INET_ADDRSTRLEN */

typedef struct
{
    int                fd;
    struct sockaddr_in local_saddr;
    struct sockaddr_in dst_saddr;
    uint16_t           dst_port;
    char               dst_dir[INET_ADDRSTRLEN];
} *client;

typedef struct
{
    int                fd;
    struct sockaddr_in local_saddr;
    uint16_t           local_port;
    char               local_dir[INET_ADDRSTRLEN];
    /* - array de conexiones
     * - num de conexiones activas */
} *server;

typedef struct
{
    int                fd;
    struct sockaddr_in saddr;
    uint16_t           port;
    char               dir[INET_ADDRSTRLEN];
} *connection;

/* FUNCIONES DE CLIENTE */
client net_client_create(char *name, char *service);

void net_client_delete(client *clnt);

/* FUNCIONES DE CONEXIÓN */
void net_conn_delete(connection *conn);

/* FUNCIONES DE SERVIDOR */
server net_server_create(char *service, int backlog);

connection net_server_accept(server srv);

int net_server_recv(connection conn, uint8_t *buf, size_t len);

void net_server_delete(server *srv);

#endif
