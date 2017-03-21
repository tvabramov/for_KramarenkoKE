#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "my_daemon.h"

// Daemon's log file name
#define DAEMON_LOG_FILE "/home/students/tvabramov/kr_5/my_daemon.pid"
// Maximum number of file descriptiors, that must be closed to create a daemon
#define FD_COUNT 1000

int main(int argc, char **argv)
{
	printf("RUNNER: My pid = %d\n", getpid());

	int daemon_pid = fork();

	// Error
	if (daemon_pid == -1) {
		perror("RUNNER, fork");
		exit(1);
	}

	// Child (daemon) process
	if (daemon_pid == 0) {		
		umask(0); // create world-writable files

		setsid(); // set new session

		chdir("/"); // untouch form the file system

		int fd = 0;
			
		for (fd = 0; fd < FD_COUNT; fd++)
			close(fd); // close all file descriptors

		exit(work_daemon(DAEMON_LOG_FILE));
	}

	// Otherwise, it is master thread
	// It is dangerous to use wait() function here,
        // because child process may already finish its work.
	// If it is needed, use twice fork()
	printf("RUNNER: created daemon's pid = %d, my work is done\n", daemon_pid);

	exit(0);
}

