#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int work_child_1(); // 1st master's child's work
int work_child_2(); // 2nd master's child's work
int work_child_1_1(); // 1st master's child 1st child's work
int work_child_2_1(); // 2nd master's child 1st child's work
int work_child_2_2(); // 2nd master's child 2nd child's work



int main(int argc, char **argv)
{
	printf("MASTER: My pid = %d, my ppid = %d\n", getpid(), getppid());

	// 1st forking
	int pid1 = fork();

	// Error
	if (pid1 == -1) {
		perror("MASTER proc, 1st fork");
		exit(1);
	}

	// Child process
	if (pid1 == 0) {
		exit(work_child_1());
	}

	// Otherwise, it is master thread
	printf("MASTER: My 1st child's pid = %d\n", pid1);
        
	// 2nd forking
	int pid2 = fork();

        // Error
        if (pid2 == -1) {
                perror("MASTER proc, 2nd fork");
                exit(1);
        }

        // Child process
        if (pid2 == 0) {
                exit(work_child_2());
        }

        // Otherwise, it is master thread
	printf("MASTER: My 2nd child's pid = %d\n", pid2);

	// Waiting for the child processes
	int wstat = 0;
	waitpid(pid1, &wstat, 0);
	printf("MASTER: My 1st child process finished with code = %d\n", WEXITSTATUS(wstat));

	waitpid(pid2, &wstat, 0);
	printf("MASTER: My 2nd child process finished with code = %d\n", WEXITSTATUS(wstat));

	exit(0);
}

int work_child_1() {
	printf("  MASTER'S 1-ST CHILD: My pid = %d, my ppid = %d\n", getpid(), getppid());
//	sleep(3);

	// 1st forking
        int pid1 = fork();

        // Error
        if (pid1 == -1) {
                perror("MASTER'S 1-ST CHILD proc, 1st fork");
                exit(1);
        }

        // Child process
        if (pid1 == 0) {
                exit(work_child_1_1());
        }

        // Otherwise, it is master thread
	printf("  MASTER'S 1-ST CHILD: My 1st child's pid = %d\n", pid1);

	// Waiting for the child processes
	int wstat = 0;
        waitpid(pid1, &wstat, 0);
        printf("  MASTER'S 1-ST CHILD: My 1st child process finished with code = %d\n", WEXITSTATUS(wstat));
	
	exit(10);
}

int work_child_2() {
        printf("  MASTER'S 2-ND CHILD: My pid = %d, my ppid = %d\n", getpid(), getppid());
//	sleep(2);

	// 1st forking
        int pid1 = fork();

        // Error
        if (pid1 == -1) {
                perror("MASTER'S 2-ND CHILD proc, 1st fork");
                exit(1);
        }

        // Child process
        if (pid1 == 0) {
                exit(work_child_2_1());
        }

	// Otherwise, it is master thread
        printf("  MASTER'S 2-ND CHILD: My 1st child's pid = %d\n", pid1);

	// 2nd forking
        int pid2 = fork();

        // Error
        if (pid2 == -1) {
                perror("MASTER'S 2-ND CHILD proc, 2nd fork");
                exit(1);
        }

        // Child process
        if (pid2 == 0) {
                exit(work_child_2_2());
        }

        // Otherwise, it is master thread
        printf("  MASTER'S 2-ND CHILD: My 2nd child's pid = %d\n", pid2);

        // Waiting for the child processes
        int wstat = 0;
        waitpid(pid1, &wstat, 0);
        printf("  MASTER'S 2-ND CHILD:: My 1st child process finished with code = %d\n", WEXITSTATUS(wstat));

        waitpid(pid2, &wstat, 0);
        printf("  MASTER'S 2-ND CHILD:: My 2nd child process finished with code = %d\n", WEXITSTATUS(wstat));

        exit(20);
}

int work_child_1_1() {
	printf("    MASTER'S 1-ST CHILD'S 1-ST CHILD: My pid = %d, my ppid = %d\n", getpid(), getppid());
        sleep(1);
        return 11;
}

int work_child_2_1() {
	printf("    MASTER'S 2-ND CHILD'S 1-ST CHILD: My pid = %d, my ppid = %d\n", getpid(), getppid());
        sleep(1);
        return 21;
}

int work_child_2_2() {
	printf("    MASTER'S 2-ND CHILD'S 2-ND CHILD: My pid = %d, my ppid = %d\n", getpid(), getppid());
        sleep(1);
        return 22;
}

