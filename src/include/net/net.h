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

client net_client_create(char *name, char *service);

void net_client_delete(client *clnt);

#endif
