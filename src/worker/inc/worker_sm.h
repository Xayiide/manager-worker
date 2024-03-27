#ifndef WORKER_SM_H_
#define WORKER_SM_H_

#include <stddef.h> /* size_t  */
#include <poll.h>   /* polling */

void worker_sm_run(struct pollfd *pfds, size_t pfds_len);

#endif
