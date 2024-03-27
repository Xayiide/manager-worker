#include "inc/worker_sm.h"

#include <unistd.h> /* sleep */
#include <stdio.h>

/* ********************************************************************** */
#define NELEMS(x) ((sizeof (x)) / (sizeof ((x)[0])))



/* ********************************************************************** */
/* *********************** Declaración de tipos ************************* */
/* ********************************************************************** */
struct worker_sm_data
{
    worker_state_e state;
    int            fd;
};

/* ********************************************************************** */
/* ************************* Functiones estáticas *********************** */
/* ********************************************************************** */
static void           worker_sm_init(struct worker_sm_data *data, int fd);
static worker_event_e worker_state_polling   (struct worker_sm_data *data);
static worker_event_e worker_state_sendmsg   (struct worker_sm_data *data);
static worker_event_e worker_state_recvmsg   (struct worker_sm_data *data);
static worker_event_e worker_state_close     (struct worker_sm_data *data);
static void           worker_sm_do_transition(struct worker_sm_data *data,
                                              worker_event_e         event);


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

    {STATE_SENDMSG, EVENT_ERROR,  STATE_CLOSE},
    {STATE_SENDMSG, EVENT_NONE,   STATE_POLLING},

    {STATE_RECVMSG, EVENT_ERROR,  STATE_CLOSE},
    {STATE_RECVMSG, EVENT_NONE,   STATE_POLLING},
};



void worker_sm_run(int fd)
{
    struct worker_sm_data data;
    worker_sm_cb          state_callback;
    worker_event_e        event;

    worker_sm_init(&data, fd);

    while (1) {
        state_callback = worker_sm_states[data.state];
        event          = state_callback(&data);
        if (event == EVENT_ERROR)
            return;
        worker_sm_do_transition(&data, event);
        sleep(1);
    }

    return;
}

void worker_sm_init(struct worker_sm_data *data, int fd)
{
    data->state = STATE_POLLING;
    data->fd = fd;

    return;
}

worker_event_e worker_state_polling(struct worker_sm_data *data)
{
    (void) data;
    static int i = 0;
    printf("ESTADO POLLING - ");
    if (i++ % 2 == 0 ) {
        printf("SOCKET\n");
        return EVENT_SOCKET;
    }
    else {
        printf("STDIN\n");
        return EVENT_STDIN;
    }
}

worker_event_e worker_state_sendmsg(struct worker_sm_data *data)
{
    (void) data;
    printf("ESTADO SENDMSG\n");
    return EVENT_NONE;
}

worker_event_e worker_state_recvmsg(struct worker_sm_data *data)
{
    (void) data;
    printf("ESTADO RECVMSG\n");
    return EVENT_NONE;
}

worker_event_e worker_state_close(struct worker_sm_data *data)
{
    (void) data;
    printf("ESTADO CLOSED\n");
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
