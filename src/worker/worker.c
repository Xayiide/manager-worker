#include <stdio.h>
#include <unistd.h> /* sleep */

#include "net/net.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <hostname> <puerto>\n", argv[0]);
        return 1;
    }

    return 0;
}
