#include <stdio.h>  /* printf, perror          */
#include <stdlib.h> /* exit, EXIT_SUCCESS, ... */

#define PORT    3490
#define BACKLOG 10

static int create_srv_socket()
{
    int                srv_socket;
    int                aux_res;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    aux_res = bind(srv_socket, (struct sockaddr *) &addr,
                   sizeof(addr));
    if (aux_res == -1)
        perror("bind");

    aux_res = listen(srv_socket, BACKLOG);
    if (aux_res == -1)
        perror("listen");

    return srv_socket;
}



int main()
{

    printf("[Manager]\n");

    return EXIT_SUCCESS;
}
