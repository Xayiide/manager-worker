#include "inc/manager_sm.h"
#include "inc/manager.h"

#include <string.h>     /* strlen, memset */
#include <stdio.h>      /* stidn          */
#include <unistd.h>     /* close, usleep  */
#include <poll.h>       /* polling        */
#include <netinet/in.h> /* sockaddr_in    */
#include <arpa/inet.h>  /* inet_ntop      */
#include <sys/socket.h> /* send           */

/* ********************************************************************** */
#define NELEMS(x) ((sizeof (x)) / (sizeof ((x)[0])))



/* ********************************************************************** */
/* *********************** Declaración de tipos ************************* */
/* ********************************************************************** */

typedef struct {
    struct sockaddr_in  saddr;
    char                ip_str[INET6_ADDRSTRLEN];
    uint16_t            cln_port;
    char                cln_name[CLN_NAME_MAXLEN];
    struct pollfd      *pfd;
} manager_conn_t;

typedef enum {
    STATE_POLLING,
    STATE_ACCEPT,
    STATE_RECV,
    STATE_BCAST,
    STATE_SRV_CLOSE,
    STATE_CLN_CLOSE,
} manager_state_e ;

typedef enum  {
    EVENT_SERVER,
    EVENT_CLIENT,
    EVENT_ERROR,
    EVENT_NONE,
} manager_event_e ;

typedef struct {
    manager_state_e from;
    manager_event_e event;
    manager_state_e to;
} manager_transition_t;

typedef struct {
    manager_state_e  state;
    struct pollfd   *pfds;
    /* Por ahora siempre es MAX_CLIENTS + 1 */
    size_t           pfds_len;
    /* Número de conexiones activas, para pasarle a poll */
    int              num_conns;
    /* Guarda el siguiente cliente libre */
    int              next_free_cln;
    /* Array de clientes */
    manager_conn_t   clients[MAX_CLIENTS];
    /* Recepción de mensajes */
    char             msg_buf[1024];
    /* cliente que envía el mensaje a redistribuir */
    manager_conn_t  *sender;
} manager_data_t;


/* ************************* Functiones estáticas *********************** */
static void            manager_sm_init         (manager_data_t *data,
                                                struct pollfd  *pfds,
                                                size_t          pfds_len);
static manager_event_e manager_state_polling   (manager_data_t *data);
static manager_event_e manager_state_accept    (manager_data_t *data);
static manager_event_e manager_state_recv      (manager_data_t *data);
static manager_event_e manager_state_bcast     (manager_data_t *data);
static manager_event_e manager_state_srv_close (manager_data_t *data);
static manager_event_e manager_state_cln_close (manager_data_t *data);
static void            manager_sm_do_transition(manager_data_t *data,
                                                manager_event_e event);
static void            manager_next_free_cln   (manager_data_t *data);

static void           *get_in_addr(struct sockaddr *sa);
static size_t          create_msg(manager_conn_t *client, char *send_buf,
                                  char *buf, size_t len);

/* manager_sm_cb: tipo nuevo. Puntero a función que como parámetro toma un
 * puntero a struct manager_data_t y devuelve enum manager_event_t */
typedef manager_event_e (* manager_sm_cb)(manager_data_t *);

/* Array de manager_sm_cb. En cada índice está la función que se deberá
 * llamar para el valor correspondiente del enum manager_state_e */
static manager_sm_cb manager_sm_states[] = {
    manager_state_polling,
    manager_state_accept,
    manager_state_recv,
    manager_state_bcast,
    manager_state_srv_close,
    manager_state_cln_close,
};


/* Array de transiciones */
static const manager_transition_t transitions[] = {
    {STATE_POLLING,   EVENT_SERVER, STATE_ACCEPT},
    {STATE_POLLING,   EVENT_CLIENT, STATE_RECV},
    {STATE_POLLING,   EVENT_NONE,   STATE_POLLING},
    {STATE_POLLING,   EVENT_ERROR,  STATE_SRV_CLOSE},

    {STATE_ACCEPT,    EVENT_NONE,   STATE_POLLING},
    {STATE_ACCEPT,    EVENT_ERROR,  STATE_POLLING},

    {STATE_RECV,      EVENT_NONE,   STATE_BCAST},
    {STATE_RECV,      EVENT_ERROR,  STATE_CLN_CLOSE},

    {STATE_BCAST,     EVENT_NONE,   STATE_POLLING},
    {STATE_BCAST,     EVENT_ERROR,  STATE_POLLING},

    {STATE_CLN_CLOSE, EVENT_NONE, STATE_POLLING},
};

char *client_names[MAX_CLIENTS + 1] = {
    "server",
    "client 1",
    "client 2",
    "client 3",
    "client 4",
    "client 5",
    "client 6",
    "client 7",
    "client 8",
    "client 9",
    "client 10",
};




void manager_sm_run(struct pollfd *pfds, size_t pfds_len)
{
    manager_data_t  data;
    manager_sm_cb   state_callback;
    manager_event_e event;

    manager_sm_init(&data, pfds, pfds_len);

    while (1) {
        state_callback = manager_sm_states[data.state];
        event          = state_callback(&data);

        manager_sm_do_transition(&data, event);
        //sleep(1);
    }

    return;
}

void manager_sm_init(manager_data_t *data, struct pollfd *pfds, size_t pfds_len)
{
    int i;

    data->state         = STATE_POLLING;
    data->pfds          = pfds;
    data->pfds_len      = pfds_len;
    data->num_conns     = 0;
    data->sender        = NULL;
    data->next_free_cln = 0;
    memset(data->clients, 0, sizeof(data->clients));

    for (i = 0; i < MAX_CLIENTS; i++)
        data->clients[i].pfd = &(data->pfds[i + 1]);

    return;
}

manager_event_e manager_state_polling(manager_data_t *data)
{
    int            i;
    int            poll_count;
    struct pollfd* pfd_aux;

    poll_count = poll(data->pfds, MAX_CLIENTS, -1);
    if (poll_count <= 0)
        return EVENT_ERROR;

    /* Evento de input en socket de escucha. Pasamos a STATE_ACCEPT */
    if (data->pfds[0].revents & POLLIN) {
        return EVENT_SERVER; /* -> STATE_ACCEPT */
    }

    /* Pasar por todos los pfds (TODO: Mejorar esto) y comprobar si hay
     * eventos en ellos */
    for (i = 0; i < MAX_CLIENTS; i++) {
        pfd_aux = data->clients[i].pfd;
        if (pfd_aux->fd < 0)
            continue;

        if (pfd_aux->revents & (POLLIN | POLLERR)) {
            /* Leer el fd correspondiente y recibir el mensaje enviado */
            data->sender = &(data->clients[i]);
            return EVENT_CLIENT; /* -> STATE_RECV */
        }
    }

    return EVENT_NONE;
}


manager_event_e manager_state_accept(manager_data_t *data)
{
    int             new_fd;
    socklen_t       sin_size;
    manager_conn_t *client;
    ssize_t         nbytes;
    char            name_buf[CLN_NAME_MAXLEN] = {0};

    client = &(data->clients[data->next_free_cln]);

    sin_size = sizeof(client->saddr);
    new_fd = accept(data->pfds[0].fd,
            (struct sockaddr *) &(client->saddr),
            &sin_size);
    if (new_fd == -1) {
        fprintf(stderr, "accept.\n");
        return EVENT_ERROR;
    }


    nbytes = recv(new_fd, name_buf, CLN_NAME_MAXLEN, 0);
    if (nbytes == -1) {
        fprintf(stderr, "Error receiving client %d name.\n", new_fd);
        return EVENT_NONE;
    }

    client->pfd->fd     = new_fd;
    client->pfd->events = POLLIN;
    data->num_conns++;

    inet_ntop(client->saddr.sin_family,
              get_in_addr((struct sockaddr *) &(client->saddr)),
              client->ip_str, INET6_ADDRSTRLEN);
    client->cln_port = ntohs(client->saddr.sin_port);

    memset(client->cln_name, 0, CLN_NAME_MAXLEN);
    strncpy(client->cln_name, name_buf, CLN_NAME_MAXLEN);

    printf("Nueva conexión de %s:%d\n", client->ip_str, client->cln_port);
    printf("    Cliente %d: %s\n", client->pfd->fd, client->cln_name);

    manager_next_free_cln(data);

    return EVENT_NONE;
}

manager_event_e manager_state_recv(manager_data_t *data)
{
    ssize_t        nbytes;
    char           buf[1024];
    struct pollfd *pfd_aux;

    pfd_aux = data->sender->pfd;

    memset(buf, 0, 1024);
    nbytes = (int) recv(pfd_aux->fd, buf, 1024, 0);

    if (nbytes <= 0) {
        if (nbytes < 0) {
            fprintf(stderr, "recv (sockt: %d)\n", pfd_aux->fd);
        }
        else if (nbytes == 0) {
            printf("Close socket %d\n", pfd_aux->fd);
        }
        return EVENT_ERROR; /* -> STATE_CLN_CLOSE */
    }
    else {
        create_msg(data->sender, data->msg_buf, buf, nbytes);
        return EVENT_NONE; /* -> STATE_BCAST */
    }

    return EVENT_NONE; /* -> STATE_BCAST */
}


manager_event_e manager_state_bcast(manager_data_t *data)
{
    int            i;
    ssize_t        nbytes;
    size_t         send_buf_len;
    struct pollfd *pfd_aux;

    send_buf_len = strlen(data->msg_buf);

    /* Reenvía el mensaje a todos los que hay en
     * data->pfds excepto al server y a sí mismo */
    for (i = 0; i < MAX_CLIENTS; i++) {
        pfd_aux = data->clients[i].pfd;
        if ((pfd_aux->fd != data->sender->pfd->fd) && (pfd_aux->fd != -1)) {
            nbytes = send(pfd_aux->fd, data->msg_buf, send_buf_len, 0);
            if (nbytes == -1) {
                fprintf(stderr, "error bcasting to fd %d.\n", pfd_aux->fd);
                continue; /* TODO: ¿Qué hacer aquí? */
            }
        }
    }

    return EVENT_NONE;
}

manager_event_e manager_state_cln_close(manager_data_t *data)
{
    int            i     = 0;
    int            found = 0;
    struct pollfd *pfd_aux;

    pfd_aux = data->sender->pfd;

    /* Busca el fd del cliente, lo cierra y lo marca como -1 */
    do {
        if (data->pfds[i].fd == pfd_aux->fd) {
            data->pfds[i].fd = -1;
            close(data->pfds[i].fd);
            data->num_conns--;
            found = 1;
        }
        i++;
    } while ((found == 0) && (i < MAX_CLIENTS));

    if (found == 0) {
        fprintf(stderr, "No se ha podido cerrar el fd %d.\n", pfd_aux->fd);
        return EVENT_ERROR;
    }

    return EVENT_NONE;
}

manager_event_e manager_state_srv_close(manager_data_t *data)
{
    int i;

    for (i = 0; i < (int) data->num_conns; i++) {
        close(data->pfds[i].fd);
    }

    return EVENT_NONE;
}

void manager_sm_do_transition(manager_data_t  *data,
                              manager_event_e  event)
{
    int i;

    for (i = 0; i < (int) NELEMS(transitions); i++) {
        if (data->state == transitions[i].from
                && event == transitions[i].event) {
            data->state = transitions[i].to;
            break;
        }
    }

    return;
}

void manager_next_free_cln(manager_data_t *data)
{
    int i;
    struct pollfd *pfd;

    for (i = 0; i < MAX_CLIENTS; i++) {
        pfd = data->clients[i].pfd;
        if (pfd->fd == -1) {
            data->next_free_cln = i;
            break;
        }
    }

    return;
}


size_t create_msg(manager_conn_t *client, char *send_buf, char *buf, size_t len)
{
    size_t pos       = 0;
    size_t name_len  = 0;
    size_t total_len = 0;

    name_len = strlen(client->cln_name);

    send_buf[pos++] = '[';
    strncpy(&send_buf[pos], client->cln_name, name_len);
    pos += name_len;
    send_buf[pos++] = ']';
    send_buf[pos++] = ' ';

    strncpy(&send_buf[pos], buf, len);
    pos += len;
    send_buf[pos++] = '\0';

    total_len = strlen(send_buf);

    return  total_len;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
