#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#define THREADS_COUNT 10

void *threads_work(void *args);
void test(int* ret_values);

int main(int argc, char **argv)
{
	int i;
	int ret_values[THREADS_COUNT];

	for (i = 0; i < 10; i++) {
		test(ret_values);

		printf("Test %d:", i);
		int j;
		for (j = 0; j < THREADS_COUNT; j++)
			printf(" %d", ret_values[j]);
		printf("\n");
	}
	exit(EXIT_SUCCESS);
}

void test(int* ret_values)
{
	int i;

	pthread_attr_t attrs[THREADS_COUNT];

	for (i = 0; i < THREADS_COUNT; i++)
		if (pthread_attr_init(&attrs[i]) != 0) {
			fprintf(stderr, "MASTER: Error on pthread_attr_init (thread %d)\n", i);
			exit(EXIT_FAILURE);
		}

	for (i = 0; i < THREADS_COUNT; i++)
		pthread_attr_setdetachstate(&attrs[i], PTHREAD_CREATE_JOINABLE);

	pthread_t threads[THREADS_COUNT];

	// Here is error simulating, we give &i to each thread
	for (i = 0; i < THREADS_COUNT; i++)
		if (pthread_create(&threads[i], &attrs[i], &threads_work, &i) != 0) {
			fprintf(stderr, "MASTER: Error on pthread_create (thread %d)\n", i);
			exit(EXIT_FAILURE);
		}
	i = 1000; // if this value is shown, the loop is finished before the thread is started

	for (i = 0; i < THREADS_COUNT; i++)
		pthread_attr_destroy(&attrs[i]);

	//Waiting for the threads
	for (i = 0; i < THREADS_COUNT; i++)
	{
		void* rv;
		if (pthread_join(threads[i], &rv) != 0) {
			fprintf(stderr, "MASTER: Error on pthread_join (thread %d)\n", i);
			exit(EXIT_FAILURE);
		}
		ret_values[i] = *(int *)rv;
		free(rv);
	}
}

void *threads_work(void *args)
{
	int myval = *(int *)args;

	int* result = (int *)malloc(sizeof(int));
	*result = myval;

	return result;
}
