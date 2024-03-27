#ifndef WORKER_SM_H_
#define WORKER_SM_H_

#include <stddef.h> /* size_t  */
#include <poll.h>   /* polling */

typedef enum {
    STATE_POLLING,
    STATE_SENDMSG,
    STATE_RECVMSG,
    STATE_CLOSE,
} worker_state_e;

typedef enum {
    EVENT_STDIN,
    EVENT_SOCKET,
    EVENT_ERROR,
    EVENT_NONE,
} worker_event_e;

typedef struct {
    worker_state_e from;
    worker_event_e event;
    worker_state_e to;
} worker_transition_t;

void worker_sm_run(struct pollfd *pfds, size_t pfds_len);


#endif
