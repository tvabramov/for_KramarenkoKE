#ifndef _CLINIT_H_
#define _CLINIT_H_

/** Macro, containig max length of user's nickname */
#define MAX_NICKNAME_LEN 10

typedef struct CLINIT_PARAMS_struct {
        int i_am_server;                       //!< 1 - start mqchat as server, otherwise - client.
	char nickname[MAX_NICKNAME_LEN + 1];   //!< User's nickname. For server it is used too.
} CLINIT_PARAMS;

int clinit(int argc, char **argv, CLINIT_PARAMS *clinit_params);
void print_clinit_params(CLINIT_PARAMS clinit_params);

#endif
