#ifndef NET_H_
#define NET_H_

#include <stdint.h>     /* uint_t */
#include <netinet/in.h> /* INET_ADDRSTRLEN */

int net_client_create(char ip[INET_ADDRSTRLEN], uint16_t port);


#endif
