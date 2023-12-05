#include <stdio.h>
#include <unistd.h> /* sleep */

#include "net/net.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <hostname> <puerto>\n", argv[0]);
        return 1;
    }

    printf("argv[0]: %s\n", argv[0]);
    printf("argv[1]: %s\n", argv[1]);
    printf("argv[2]: %s\n", argv[2]);

    client clnt = net_client_create(argv[1], argv[2]);

    return 0;
}
