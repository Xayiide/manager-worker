#include <stdio.h>       /* perror */
#include <stdlib.h>      /* atoi, exit, EXIT_FAILURE   */
#include <string.h>      /* memset */
#include <unistd.h>      /* close  */
#include <sys/socket.h>  /* recv   */
#include <poll.h>        /* poll   */
#include <stddef.h>      /* size_t */
#include <curl/curl.h>

#include "net/net.h"

#define MAX_CLIENTS 50
#define LISTENQ     5
#define NUM_FILES   100

typedef struct {
    char   *data;
    size_t  size;
} curl_data;

int manager_distribute_work(int   fd,           int *files_status,
                            char *files_todo[], int *files_done)
{
    int   index   = 0;
    int   found   = 0;
    int   ret_tmp = 0;
    char *file;

    if (*files_done == NUM_FILES) {
        printf("No work to distribute, everything's done\n");
        return -1;
    }

    while (found == 0 && index < NUM_FILES) {
        if (files_status[index] == 0)
            found = 1;
        else
            index++;
    }

    if (found == 0) {
        fprintf(stderr, "Error: no files available but files_done=%d\n",
                *files_done);
        return -1;
    }

    file = files_todo[index];
    ret_tmp = send(fd, file, strlen(file), 0);
    if (ret_tmp == -1) {
        fprintf(stderr, "Error sending file %s to fd %d\n", file, fd);
        return -1;
    }

    files_status[index] = fd;

    return index;
}

size_t manager_get_data(void *buf, size_t itemsize, size_t nitems, void *userdata)
{
    size_t     chunksize;
    curl_data *curldata;
    char      *ptr;

    chunksize = itemsize * nitems;
    curldata  = (curl_data *) userdata;

    /* +1 for NULL terminator */
    ptr = realloc(curldata->data, curldata->size + chunksize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "realloc\n");
        return CURLE_WRITE_ERROR;
    }

    curldata->data = ptr;
    /* Append new data to previous data */
    memcpy(&(curldata->data[curldata->size]), buf, chunksize);
    curldata->size += chunksize;
    curldata->data[curldata->size] = 0; /* null-terminator */

    return chunksize;
}

void manager_download_file(char *url, curl_data *curldata)
{
    CURL     *curl;
    CURLcode  result;

    /*TODO: Hacerlo sólo una vez */
    //curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "curl_easy_init\n");
        exit(EXIT_FAILURE);
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, manager_get_data);
    /* Manda el struct curldata como argumento userdata a manager_get_data */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) curldata);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "CURL problem: %s\n", curl_easy_strerror(result));
        exit(EXIT_FAILURE);
    }

    curl_easy_cleanup(curl);
}

void manager_finalize_work(int fd, )
{

}



int main(int argc, char *argv[])
{
    struct pollfd pfds[MAX_CLIENTS]; /* storing the server fd (pfds[0]) */
    int           fd_highest = 0;

    int           poll_count;
    int           text_count, total_count = 0;
    int           i, nbytes;
    int           aux_fd, port;

    server        srv;
    connection    conn;

    curl_data curldata;
    curldata.data = malloc(1);
    curldata.size = 0;

    /* Guarda las 100 urls de los ficheros csv que se quieren repartir */
    char *files_todo[NUM_FILES];
    /* Guarda el estado de procesamiento de cada elemento de files_todo.
     * files_status[i] puede tener 3 valores:
     *  · > 0: fd del socket al que se ha mandado files_todo[i] para procesar.
     *  · = 0: files_todo[i] todavía no ha sido repartido.
     *  · < 0: -fd del socket al que se mandó files_todo[i] y procesó. */
    int   files_status[NUM_FILES] = { 0 };
    int   files_done = 0;
    char *pch        = NULL;




    if (argc != 3) {
        fprintf(stderr, "usage: %s <URL to file> <port>\n", argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    manager_download_file(argv[1], &curldata);

    i = 0;
    pch = strtok(curldata.data, "\n"); /* pch -> null-terminated */
    while (pch != NULL) {
        files_todo[i] = malloc(strlen(pch));
        strncpy(files_todo[i], pch, strlen(pch) + 1);
        pch = strtok(NULL, "\n");
        i++;
    }

    for (i = 0; i < NUM_FILES; i++)
        printf("%d: %s\n", i, files_todo[i]);

    printf("file size: %zu bytes\n", curldata.size);

    port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number %d\n", port);
        return 1;
    }

    srv = net_server_create(argv[2], LISTENQ);
    printf("Listening socket: %d\n", srv->fd);
    
    for (i = 1; i < MAX_CLIENTS; i++) {
        pfds[i].fd = -1;
    }

    pfds[0].fd     = srv->fd;
    pfds[0].events = POLLIN;

    while (1) {
        poll_count = poll(pfds, fd_highest + 1, -1);

        if (poll_count <= 0)
            continue;

        if (pfds[0].revents & POLLIN) {
            conn = net_server_accept(srv);
            printf("New connection %d from %s:%hu\n",
                    conn->fd, conn->dir, conn->port);

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (pfds[i].fd < 0) {
                    pfds[i].fd = conn->fd;
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                fprintf(stderr, "Demasiados clientes [%d]\n", MAX_CLIENTS);
                close(conn->fd);
            }

            pfds[i].events = POLLIN;

            if (i > fd_highest)
                fd_highest = i;

            manager_distribute_work(conn->fd, files_status,
                                    files_todo, &files_done);

            poll_count--;
            if (poll_count <= 0)
                continue;
        }

        for (i = 1; i <= fd_highest; i++) {
            aux_fd = pfds[i].fd;
            if (aux_fd < 0) /* Ignora fds a -1 */
                continue;

            if (pfds[i].revents & (POLLIN | POLLERR)) {
                nbytes = recv(aux_fd, &text_count, sizeof(text_count), 0);

                if (nbytes <= 0) {
                    if (nbytes < 0)
                        perror("read");
                    else if (nbytes == 0)
                        printf("Close socket %d\n", aux_fd);
                    close(aux_fd);
                    pfds[i].fd = -1;
                }
                else {
                    /* El byte va a ser el número de apariciones de un patrón */
                    /* Convertimos ese byte al número apropiado */
                    /* Buscamos en files_status el fichero que procesaba ese fd:
                     *  · No se encuentra:
                     *      Se ignora el mensaje (y se imprime algo)
                     *  · Se encuentra:
                     *      La posición correspondiente de files_status -> -fd
                     *      Se incrementa el número de apariciones
                     */
                    printf("Read %d bytes from socket %d: [%d]\n",
                            nbytes, aux_fd, ntohl(text_count));
                    manager_finalize_work(aux_fd, text_count,
                            &total_count, files_status);

                    total_count += ntohl(text_count);
                    manager_distribute_work(conn->fd, files_status,
                                            files_todo, &files_done);
                }


                poll_count--;
                if (poll_count <= 0)
                    break;
            }
        }
    }

    /* for i in files_todo:
     *     if (files_status[i] == 0)
     *         error: todavía procesándose
     *     if (files_status[i] > 0)
     *         error: no repartido
     *     if (files_status[i] < 0)
     *         free(files_todo[i])
     */

    free(curldata.data);
    net_conn_delete(&conn);
    net_server_delete(&srv);
    
    return 0;
}
