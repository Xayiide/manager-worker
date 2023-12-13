#include <stdio.h>
#include <unistd.h> /* sleep */
#include <signal.h> /* signal */
#include <stdlib.h> /* exit */
#include <string.h> /* memset */
#include <sys/socket.h> /* shutdown, SHUT_RDWR */
#include <curl/curl.h> /* curl */

#include "net/net.h"

typedef struct {
    char   *data;
    size_t  size;
} curl_data;

#define BUF_SIZE 256

int g_run = 1;
client g_clnt;


void ctrl_c_handler(int d) {
    g_run = 0;
    printf("Closing fd %d [%d]\n", g_clnt->fd, d);
    //shutdown(g_clnt->fd, SHUT_RDWR);
    close(g_clnt->fd);
}

size_t worker_get_data(void *buf, size_t itemsize, size_t nitems, void *userdata)
{
    size_t     chunksize;
    curl_data *curldata;
    char      *ptr;

    chunksize = itemsize * nitems;
    curldata  = (curl_data *) userdata;

    ptr = realloc(curldata->data, curldata->size + chunksize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "realloc\n");
        return CURLE_WRITE_ERROR;
    }

    curldata->data = ptr;
    memcpy(&(curldata->data[curldata->size]), buf, chunksize);
    curldata->size += chunksize;
    curldata->data[curldata->size] = 0; /* null-terminator */

    printf("data [%ld] %s\n", curldata->size, curldata->data);

    return chunksize;
}

void worker_download_file(char *url, char **data, size_t *size)
{
    CURL     *curl;
    CURLcode  result;
    curl_data curldata;

    curldata.data = *data;
    curldata.size = *size;

    printf("Downloading file %s\n", url);

    /* TODO: Hacerlo sólo una vez */
    //curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "curl_easy_init\n");
        exit(EXIT_FAILURE);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, worker_get_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &curldata);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "CURL problem: %s\n", curl_easy_strerror(result));
        exit(EXIT_FAILURE);
    }
    *data = curldata.data;

    curl_easy_cleanup(curl);
}



int main(int argc, char *argv[])
{
    char   url[BUF_SIZE] = { 0 };
    int    nbytes        = 0;
    int    result        = 0;
    char  *file;
    size_t size;

    if (argc != 3) {
        fprintf(stderr, "Uso: %s <hostname> <puerto>\n", argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    signal(SIGINT, ctrl_c_handler);

    file = malloc(1);
    size = 0;

    g_clnt = net_client_create(argv[1], argv[2]);
    while (g_run == 1) {
        nbytes = recv(g_clnt->fd, url, BUF_SIZE, 0);
        if (nbytes <= 0) {
            if (nbytes < 0)
                perror("read");
            else if (nbytes == 0)
                printf("Server closed the socket\n");
            break;
        }
        url[nbytes] = '\0'; /* por si acaso */
        printf("Received url: %s\n", url);

        worker_download_file(url, &file, &size);
        printf("url contents: %s\n", file);

        /* TODO: Procesar file */

        sleep(5);
        result = 6199281;
        result = htonl(result);
        nbytes = send(g_clnt->fd, &result, sizeof(result), 0);
        if (nbytes == -1) {
            fprintf(stderr, "Error sending result to server\n");
            break;
        }
    }

    free(file);
    net_client_delete(&g_clnt);

    return 0;
}
