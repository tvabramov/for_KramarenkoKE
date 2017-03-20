#ifndef _MSGQ_H_
#define _MSGQ_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int start_server();
int start_client();
int close_connection(int msgid);

#endif

