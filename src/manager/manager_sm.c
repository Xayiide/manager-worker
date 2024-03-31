#include "inc/manager_sm.h"

#include <string.h>     /* strlen      */
#include <stdio.h>      /* stidn       */
#include <unistd.h>     /* close       */
#include <poll.h>       /* polling     */
#include <netinet/in.h> /* sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop   */
#include <sys/socket.h> /* send        */


/* ********************************************************************** */
#define NELEMS(x) ((sizeof (x)) / (sizeof ((x)[0])))



/* ********************************************************************** */
/* *********************** Declaración de tipos ************************* */
/* ********************************************************************** */

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
    size_t           pfds_len;
    int              fd_highest;
    char             msg_buf[1024];
    int              fd_sender;
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

static void           *get_in_addr(struct sockaddr *sa);
static int             create_msg(int fd, char *send_buf, char *buf, size_t len);

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

char *client_names[10 /* MAX_CLIENTS */ + 1] = {
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
    data->state      = STATE_POLLING;
    data->pfds       = pfds;
    data->pfds_len   = pfds_len;
    data->fd_highest = 0;
    data->fd_sender  = -1;

    return;
}

manager_event_e manager_state_polling(manager_data_t *data)
{
    int i;
    int poll_count;

    printf("ESTADO POLLING\n");

    poll_count = poll(data->pfds, data->fd_highest + 1, -1);
    if (poll_count <= 0)
        return EVENT_ERROR;

    /* Evento de input en socket de escucha. Pasamos a STATE_ACCEPT */
    if (data->pfds[0].revents & POLLIN) {
        return EVENT_SERVER; /* -> STATE_ACCEPT */
    }

    for (i = 1; i <= data->fd_highest; i++) {
        /* Ignora fds a -1 */
        if (data->pfds[i].fd < 0)
            continue;

        if (data->pfds[i].revents & (POLLIN | POLLERR)) {
            /* Lee el fd correspondiente */
            data->fd_sender = data->pfds[i].fd;
            return EVENT_CLIENT; /* -> STATE_RECV */
        }
    }

    return EVENT_ERROR;
}

manager_event_e manager_state_accept(manager_data_t *data)
{
    int             i;
    int             new_fd;
    struct sockaddr their_addr;
    socklen_t       sin_size = sizeof(their_addr);
    char            s[INET6_ADDRSTRLEN];

    printf("ESTADO ACCEPT\n");

    new_fd = accept(data->pfds[0].fd, &their_addr, &sin_size);
    if (new_fd == -1) {
        fprintf(stderr, "accept.\n");
        return EVENT_ERROR;
    }

    inet_ntop(their_addr.sa_family,
              get_in_addr((struct sockaddr *) &their_addr),
              s, sizeof(s));

    printf("server: got connection from %s\n", s);

    /* Encuentro el primer pfd inutilizado y lo utilizo */
    for (i = 0; i < 10 /* MAX_CLIENTS */; i++) {
        if (data->pfds[i].fd < 0) {
            data->pfds[i].fd = new_fd;
            break;
        }
    }

    if (i == 10 /* MAX_CLIENTS */) {
        fprintf(stderr, "Demasiados clientes\n");
        return EVENT_ERROR;
    }

    /* Marcar el nuevo fd como escuchando en input */
    data->pfds[i].events = POLLIN;

    if (i > data->fd_highest)
        data->fd_highest = i;

    return EVENT_NONE;
}

manager_event_e manager_state_recv(manager_data_t *data)
{
    int nbytes;
    char buf[1024];
    printf("ESTADO RECV\n");

    memset(buf, 0, 1024);
    nbytes = recv(data->fd_sender, buf, 1024, 0);

    if (nbytes <= 0) {
        if (nbytes < 0) {
            fprintf(stderr, "recv (sockt: %d)\n", data->fd_sender);
        }
        else if (nbytes == 0) {
            printf("Close socket %d\n", data->fd_sender);
        }
        return EVENT_ERROR; /* -> STATE_CLN_CLOSE */
    }
    else {
        create_msg(data->fd_sender, data->msg_buf, buf, nbytes);
        return EVENT_NONE; /* -> STATE_BCAST */
    }

    return EVENT_NONE; /* -> STATE_BCAST */
}

manager_event_e manager_state_bcast(manager_data_t *data)
{
    int i;
    int nbytes;
    int send_buf_len;

    printf("ESTADO BCAST\n");

    send_buf_len = strlen(data->msg_buf);

    /* Reenvía el mensaje a todos los que hay en
     * data->pfds excepto al server y a sí mismo */
    for (i = 1; i <= data->fd_highest; i++) {
        if (data->pfds[i].fd != data->fd_sender) {
            nbytes = send(data->pfds[i].fd, data->msg_buf, send_buf_len, 0);
            if (nbytes == -1) {
                fprintf(stderr, "error bcasting to fd %d.\n", data->pfds[i].fd);
                continue; /* TODO: ¿Qué hacer aquí? */
            }
        }
    }

    return EVENT_NONE;
}

manager_event_e manager_state_cln_close(manager_data_t *data)
{
    int i     = 0;
    int found = 0;

    printf("ESTADO CLOSE CLIENT\n");

    /* Busca el fd del cliente, lo cierra y lo marca como -1 */
    do {
        if (data->pfds[i].fd == data->fd_sender) {
            data->pfds[i].fd = -1;
            close(data->pfds[i].fd);
            found = 1;
        }
        i++;
    } while ((found == 0) && (i < 10 /* MAX_CLIENTS */));

    if (found == 0) {
        fprintf(stderr, "No se ha podido cerrar el fd %d.\n", data->fd_sender);
        return EVENT_ERROR;
    }

    return EVENT_NONE;
}

manager_event_e manager_state_srv_close(manager_data_t *data)
{
    printf("ESTADO CLOSE SERVER\n");
    int i;

    for (i = 0; i < (int) data->fd_highest; i++) {
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

int create_msg(int fd, char *send_buf, char *buf, size_t len)
{
    int pos       = 0;
    int name_len  = 0;
    int total_len = 0;

    name_len = strlen(client_names[fd]);

    send_buf[pos++] = '[';
    strncpy(&send_buf[pos], client_names[fd], name_len);
    pos += name_len;
    send_buf[pos++] = ']';
    send_buf[pos++] = ' ';

    strncpy(&send_buf[pos], buf, len);
    pos += len;
    send_buf[pos++] = '\0';

    total_len = strlen(send_buf);

    return total_len;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
