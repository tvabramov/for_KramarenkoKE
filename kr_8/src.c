#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

typedef struct incrementer_data_t_struct {
        int a;
        pthread_mutex_t mutex_a;
        pthread_cond_t cond;
	int end_state;
} incrementer_data_t;

void *incrementer_work(void *args);
void *incrementer2_work(void *args);

int main(int argc, char **argv)
{
	// Creating the incrementer
	incrementer_data_t incr_in_data;
	incr_in_data.a = 0;
	incr_in_data.end_state = 0;
	if (pthread_mutex_init(&incr_in_data.mutex_a, NULL) != 0) {
		fprintf(stderr, "MASTER: Error on pthread_mutex_init\n");
		exit(EXIT_FAILURE);
        }

	if (pthread_cond_init(&incr_in_data.cond, NULL) != 0) {
                fprintf(stderr, "MASTER: Error on pthread_mutex_init\n");
                exit(EXIT_FAILURE);
        }

	// Creating 1st
	pthread_attr_t incrementer_attr;
	
	if (pthread_attr_init(&incrementer_attr) != 0) {
        	fprintf(stderr, "MASTER: Error on pthread_attr_init\n");
                exit(EXIT_FAILURE);
        }

	pthread_attr_setdetachstate(&incrementer_attr, PTHREAD_CREATE_JOINABLE);

	//Creating the 2nd incrementer
	pthread_attr_t incrementer2_attr;

        if (pthread_attr_init(&incrementer2_attr) != 0) {
                fprintf(stderr, "MASTER: Error on pthread_attr_init\n");
                exit(EXIT_FAILURE);
        }

        pthread_attr_setdetachstate(&incrementer2_attr, PTHREAD_CREATE_JOINABLE);


	// Starting both
	pthread_t incrementer_thread;
        if (pthread_create(&incrementer_thread, &incrementer_attr, &incrementer_work, &incr_in_data) != 0) {
                fprintf(stderr, "MASTER: Error on pthread_create\n");
                exit(EXIT_FAILURE);
        }

	pthread_t incrementer2_thread;
        if (pthread_create(&incrementer2_thread, &incrementer2_attr, &incrementer2_work, &incr_in_data) != 0) {
                fprintf(stderr, "MASTER: Error on pthread_create\n");
                exit(EXIT_FAILURE);
        }

        pthread_attr_destroy(&incrementer_attr);
	pthread_attr_destroy(&incrementer2_attr);

	//Waiting the threads
	void *incr_out_data = NULL;
	if (pthread_join(incrementer2_thread, &incr_out_data) != 0) {
                fprintf(stderr, "MASTER: Error on pthread_join (incrementer)\n");
                exit(EXIT_FAILURE);
        }
	
        if (pthread_join(incrementer_thread, &incr_out_data) != 0) {
                fprintf(stderr, "MASTER: Error on pthread_join (incrementer)\n");
                exit(EXIT_FAILURE);
        }

	pthread_mutex_destroy(&incr_in_data.mutex_a);
	pthread_cond_destroy(&incr_in_data.cond);

	printf("The work is done, a = %d\n", incr_in_data.a);

	exit(EXIT_SUCCESS);
}

void *incrementer_work(void *args)
{
	incrementer_data_t *data = args;

	for ( ; ; )
	{
		pthread_mutex_lock(&data->mutex_a);

		data->a++;

		if (data->a == 9) pthread_cond_signal(&data->cond);

		if (data->end_state == 1) {
			pthread_mutex_unlock(&data->mutex_a);
			break;
		}

		pthread_mutex_unlock(&data->mutex_a);
	}

	return NULL;
}

void *incrementer2_work(void *args)
{
        incrementer_data_t *data = args;

	pthread_mutex_lock(&data->mutex_a);

	pthread_cond_wait(&data->cond, &data->mutex_a);

printf("T2 a = %d\n", data->a);
	data->a++;
printf("T2 a++ = %d\n", data->a);
	data->end_state = 1;

	pthread_mutex_unlock(&data->mutex_a);
       
        return NULL;
}

