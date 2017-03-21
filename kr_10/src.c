#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>


//ls .. -1 | head -n 5
//#define prog1 "ls"
//#define vars1 "ls", "..", "-1", NULL
//#define prog2 "head"
//#define vars2 "head", "-n", "5", NULL

int ExecuteFile(char *pname, char **ppar);
int PipedExecuteFile(char *pname1, char **ppar1, char *pname2, char **ppar2);

int main(int argc, char **argv)
{	
	char prog1[] = "ls";
	char *vars1[] = { "ls", "..", "-1", (char *)NULL };
	char prog2[] = "head";
        char *vars2[] = { "head", "-n", "5", (char *)NULL };

	int erc = PipedExecuteFile(prog1, vars1, prog2, vars2);
	printf("\nPrograms \"%s\" | \"%s\" are finished with code = %d\nPress any key and \"Enter\" to continue...\n", prog1, prog2, erc);

//	int erc = ExecuteFile(prog1, vars1);
//	printf("\nProgram \"%s\" finished with code = %d\nPress any key and \"Enter\" to continue...\n", prog1, erc);

        char buf[256];
        scanf("%s", buf);

	exit(EXIT_SUCCESS);
}

int ExecuteFile(char *pname, char **ppar)
{
        int pid = fork();

        // Error
        if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
        }

        // Child process
        if (pid == 0) { 
		if (execvp(pname, ppar) < 0) {
			perror("execvp");
			exit(EXIT_FAILURE);
		}
        }

        // Otherwise, it is master thread
        int waitstatus;
        wait(&waitstatus);
	return WEXITSTATUS(waitstatus);
}

int PipedExecuteFile(char *pname1, char **ppar1, char *pname2, char **ppar2)
{
	int pid = fork();

	// Error
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	// Child process
        if (pid == 0) {
		int fd[2];

		pipe(fd);

		int pid2 = fork();

		// Error
		if (pid2 == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}

		// Child process
		if (pid2 == 0) {
			// stdout
			dup2(fd[1], 1); 
			close(fd[0]);

			if (execvp(pname1, ppar1) < 0) {
				perror("execvp");
				exit(EXIT_FAILURE);
			}
		}

		// Otherwise, it is master thread

		// stdin
		dup2(fd[0], 0);
		close(fd[1]);	
		
                if (execvp(pname2, ppar2) < 0) {
                        perror("execvp");
                        exit(EXIT_FAILURE);
                }
        }

	// Otherwise, it is master thread
	int waitstatus;
	wait(&waitstatus);
	return WEXITSTATUS(waitstatus);
}
