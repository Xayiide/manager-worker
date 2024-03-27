#include "inc/worker_sm.h"

#include <unistd.h>     /* sleep   */
#include <stddef.h>     /* size_t  */
#include <stdint.h>     /* uint_t  */
#include <poll.h>       /* polling */
#include <string.h>     /* strlen  */
#include <sys/socket.h> /* send    */
#include <stdio.h>      /* stdin   */

/* ********************************************************************** */
#define NELEMS(x) ((sizeof (x)) / (sizeof ((x)[0])))



/* ********************************************************************** */
/* *********************** Declaración de tipos ************************* */
/* ********************************************************************** */
struct worker_sm_data
{
    worker_state_e  state;
    struct pollfd  *pfds;
    size_t          pfds_len;
};

/* ********************************************************************** */
/* ************************* Functiones estáticas *********************** */
/* ********************************************************************** */
static void           worker_sm_init         (struct worker_sm_data *data,
                                              struct pollfd         *pfds,
                                              size_t                 pfds_len);
static worker_event_e worker_state_polling   (struct worker_sm_data *data);
static worker_event_e worker_state_sendmsg   (struct worker_sm_data *data);
static worker_event_e worker_state_recvmsg   (struct worker_sm_data *data);
static worker_event_e worker_state_close     (struct worker_sm_data *data);
static void           worker_sm_do_transition(struct worker_sm_data *data,
                                              worker_event_e         event);

static int readline(uint8_t *buf, int maxlen);

/* worker_sm_cb: tipo nuevo. Puntero a función que como parámetro toma un
 * puntero a struct worker_sm_data y devuelve un enum worker_event_t */
typedef worker_event_e (* worker_sm_cb)(struct worker_sm_data *);

/* Array de worker_sm_cb. En cada índice está la función que se deberá
 * llamar para el valor correspondiente del enum worker_state_e */
static worker_sm_cb worker_sm_states[] = {
    worker_state_polling,
    worker_state_sendmsg,
    worker_state_recvmsg,
    worker_state_close,
};

/* Array de transiciones */
static const worker_transition_t transitions[] = {
    {STATE_POLLING, EVENT_STDIN,  STATE_SENDMSG},
    {STATE_POLLING, EVENT_SOCKET, STATE_RECVMSG},
    {STATE_POLLING, EVENT_ERROR,  STATE_CLOSE},
    {STATE_POLLING, EVENT_NONE,   STATE_POLLING},

    {STATE_SENDMSG, EVENT_ERROR,  STATE_CLOSE},
    {STATE_SENDMSG, EVENT_NONE,   STATE_POLLING},

    {STATE_RECVMSG, EVENT_ERROR,  STATE_CLOSE},
    {STATE_RECVMSG, EVENT_NONE,   STATE_POLLING},
};



void worker_sm_run(struct pollfd *pfds, size_t pfds_len)
{
    struct worker_sm_data data;
    worker_sm_cb          state_callback;
    worker_event_e        event;

    worker_sm_init(&data, pfds, pfds_len);

    while (1) {
        state_callback = worker_sm_states[data.state];
        event          = state_callback(&data);
        if (event == EVENT_ERROR)
            return;
        worker_sm_do_transition(&data, event);
        //sleep(1);
    }

    return;
}

void worker_sm_init(struct worker_sm_data *data,
                    struct pollfd *pfds,
                    size_t pfds_len)
{
    data->state    = STATE_POLLING;
    data->pfds     = pfds;
    data->pfds_len = pfds_len;

    return;
}

worker_event_e worker_state_polling(struct worker_sm_data *data)
{
    int  poll_count;

    poll_count = poll(data->pfds, data->pfds_len, -1);
    if (poll_count <= 0)
        return EVENT_ERROR;

    /* Evento de input en stdin */
    if (data->pfds[0].revents & POLLIN) {
        return EVENT_STDIN;
    }

    if (data->pfds[1].revents & POLLIN) {
        return EVENT_SOCKET;
    }

    poll_count--;
    if (poll_count <= 0)
        return EVENT_NONE;

    return EVENT_NONE;
}

worker_event_e worker_state_sendmsg(struct worker_sm_data *data)
{
    int     nbytes;
    uint8_t stdin_buf[1024] = {0}; /* evitar chars basura antes del print */

    printf("ESTADO SENDMSG\n");

    nbytes = readline(stdin_buf, 1024);
    nbytes = send(data->pfds[1].fd, stdin_buf, nbytes, 0);
    if (nbytes == -1) {
        fprintf(stderr, "send.\n");
        return EVENT_ERROR;
    }

    return EVENT_NONE;
}

worker_event_e worker_state_recvmsg(struct worker_sm_data *data)
{
    int  nbytes;
    char recv_buf[1024] = {0}; /* evitar chars basura antes del print */

    printf("ESTADO RECVMSG\n");

    nbytes = recv(data->pfds[1].fd, recv_buf, 1024, 0);
    if (nbytes <= 0) {
        if (nbytes < 0)
            fprintf(stderr, "read.\n");
        else if (nbytes == 0)
            fprintf(stderr, "Server closed connection.\n");
        return EVENT_ERROR;
    }
    else {
        printf("%s", recv_buf);
    }
    return EVENT_NONE;
}

worker_event_e worker_state_close(struct worker_sm_data *data)
{
    printf("ESTADO CLOSED\n");
    close(data->pfds[1].fd);
    return EVENT_NONE;
}

void worker_sm_do_transition(struct worker_sm_data *data,
                             worker_event_e         event)
{
    int i;
    
    for (i = 0; i < (int) NELEMS(transitions); i++) {
        if (data->state == transitions[i].from
                && event == transitions[i].event) {
            data->state = transitions[i].to;
        }
    }
}

int readline(uint8_t *buf, int maxlen)
{
    fgets((char *) buf, maxlen, stdin);

    return strlen((char *) buf);
}
