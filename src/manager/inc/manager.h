#ifndef MANAGER_H_
#define MANAGER_H_

#include <stdint.h>     /* uint_t           */
#include <netinet/in.h> /* INET6_ADDRSTRLEN */

typedef struct {
    int                fd;
    struct sockaddr_in cln_saddr;
    struct sockaddr_in srv_saddr;
    char               ip_str[INET6_ADDRSTRLEN];
    uint16_t           srv_port;
} connection;



#endif
