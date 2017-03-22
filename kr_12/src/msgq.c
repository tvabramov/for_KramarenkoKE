#include <stdio.h>
#include "msgq.h"

int start_server()
{
	key_t key = ftok("/home/students/tvabramov/kr_12/bin/mqchat", (int)'a');
	if (key < (key_t)1) {
		perror("ftok error");
		return -1;
	}

	int msgid = msgget(key, IPC_CREAT | 0666); // Must be 666, not 0x666
	if (msgid < 1) {
                perror("msgget error");
                return -1;
        }

	return msgid;
}

int start_client()
{
	key_t key = ftok("/home/students/tvabramov/kr_12/bin/mqchat", (int)'a');
        if (key < (key_t)1) {
                perror("ftok error");
                return -1;
        }

        int msgid = msgget(key, 0);
        if (msgid < 1) {
                perror("msgget error");
                return -1;
        }

        return msgid;
}

int close_connection(int msgid)
{
	if (msgid < 0) return -1;

	if (msgctl(msgid, IPC_RMID, NULL) < 0) {
		perror("msgctl error");
                return -1;
	}

	return 0;
}
