#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "my_daemon.h"

int work_daemon(char* log_file_name)
{
	FILE* log = fopen(log_file_name, "w");
	
	if (stdout < 0) return 1;

	fprintf(log, "DAEMON: My pid = %d, starting the work...\n", getpid());

	sleep(10);

	fprintf(log, "DAEMON: The work is finished.\n");

	fclose(log);

	return 0;
}

