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

#define SHORT_FN_LEN 12
#define BUF_SIZE     256

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
        fprintf(stderr, "[client] realloc\n");
        return CURLE_WRITE_ERROR;
    }

    curldata->data = ptr;
    memcpy(&(curldata->data[curldata->size]), buf, chunksize);
    curldata->size += chunksize;
    curldata->data[curldata->size] = 0; /* null-terminator */


    return chunksize;
}

void worker_download_file(char *url, char **data, size_t *size)
{
    CURL     *curl;
    CURLcode  result;
    curl_data curldata;

    curldata.data = *data;
    curldata.size = *size;

    /* TODO: Hacerlo sólo una vez */
    //curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "[client] error: curl_easy_init\n");
        exit(EXIT_FAILURE);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, worker_get_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &curldata);

    result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        fprintf(stderr, "[client] CURL problem: %s\n", curl_easy_strerror(result));
        exit(EXIT_FAILURE);
    }
    *data = curldata.data;

    curl_easy_cleanup(curl);
}

int32_t worker_search_file(char *file, char *search)
{
    int32_t  count  = 0;
    size_t   search_len;
    char    *pos;

    search_len = strlen(search);
    pos        = file;

    if (search_len != 0) {
        while ((pos = strstr(pos, search))) {
            pos += search_len;
            count++;
        }
    }

    return count;
}

int main(int argc, char *argv[])
{
    char     url[BUF_SIZE] = { 0 };
    int      nbytes        = 0;
    int32_t  result        = 0;
    char    *file;
    char     short_filename[SHORT_FN_LEN] = { 0 }; /* test.nn.csv */
    size_t   size;

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
                perror("[client] read");
            else if (nbytes == 0)
                printf("[client] Server closed the socket\n");
            break;
        }
        url[nbytes] = '\0'; /* por si acaso */
        strcpy(short_filename, &url[strlen(url) - SHORT_FN_LEN + 1]);
        printf("[client] Downloading file: %s\n", short_filename);

        worker_download_file(url, &file, &size);
        result = worker_search_file(file, "google.ru");

        result = htonl(result);
        nbytes = send(g_clnt->fd, &result, sizeof(result), 0);
        if (nbytes == -1) {
            fprintf(stderr, "Error sending result to server\n");
            break;
        }

        sleep(3); /* FIXME */
    }

    free(file);
    net_client_delete(&g_clnt);

    return 0;
}
