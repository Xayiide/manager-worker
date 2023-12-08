#include <stdio.h>
#include <unistd.h> /* sleep */
#include <signal.h>
#include <sys/socket.h> /* shutdown, SHUT_RDWR */

#include "net/net.h"

int g_run = 1;
client g_clnt;

void ctrl_c_handler(int d) {
    g_run = 0;
    printf("Closing fd %d\n", g_clnt->fd);
    shutdown(g_clnt->fd, SHUT_RDWR);
    //close(g_clnt->fd);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <hostname> <puerto>\n", argv[0]);
        return 1;
    }

    printf("argv[0]: %s\n", argv[0]);
    printf("argv[1]: %s\n", argv[1]);
    printf("argv[2]: %s\n", argv[2]);

    signal(SIGINT, ctrl_c_handler);

    g_clnt = net_client_create(argv[1], argv[2]);

    while (g_run == 1) {
        usleep(100000);
    }
    net_client_delete(&g_clnt);

    return 0;
}
