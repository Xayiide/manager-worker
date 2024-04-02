#ifndef MANAGER_H_
#define MANAGER_H_

#include <stdint.h>     /* uint_t           */
#include <netinet/in.h> /* INET6_ADDRSTRLEN */
#include <poll.h>       /* polling          */

#define CLN_NAME_MAXLEN 16
#define MAX_CLIENTS     10

typedef struct {
    struct sockaddr_in  saddr;
    char                ip_str[INET6_ADDRSTRLEN];
    uint16_t            cln_port;
    char                cln_name[CLN_NAME_MAXLEN];
    struct pollfd      *pfd;
} manager_conn_t;



#endif
