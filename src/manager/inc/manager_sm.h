#ifndef MANAGER_SM_H
#define MANAGER_SM_H

#include <stddef.h> /* size_t  */
#include <poll.h>   /* polling */

void manager_sm_run(struct pollfd *pfds, size_t pfds_len);

#endif
