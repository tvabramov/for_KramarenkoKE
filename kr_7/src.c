#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

// This is stock size, or number of resources types
#define STOCK_SIZE 5

// Downloaders count
#define DOWNLOADERS_COUNT 3

// This is the stock type, contains cells, cells count and mutex
typedef struct stock_t_struct {
        int *cells;
        int size;
	pthread_mutex_t *mutexes;		// mutex to read/write every cell
	pthread_mutex_t write_mutex;		// mutex to print info to console. TODO: It shoud be isolated mutex, not in this structure
} stock_t;

// This structure is sent to the stock uploader thread. It contains the pointer to the stock
typedef struct uploader_in_data_t_struct {
        stock_t *stock;
} uploader_in_data_t;

// This structure is sent to the stock downloader thread. It contains the pointer to the stock
typedef struct downloader_in_data_t_struct {
        stock_t *stock;
	int id;
	int *need;
	int *have;
} downloader_in_data_t;

void *uploader_work(void *args);
void *downloader_work(void *args);

int main(int argc, char **argv)
{
	printf("MASTER: Stock simulating, size = %d\n", STOCK_SIZE);


	// Creating the stock
	stock_t stock;
	stock.size = STOCK_SIZE;
	stock.cells = (int *)malloc(sizeof(int) * stock.size);
	stock.mutexes = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * stock.size);
	// Init the stock
	printf("MASTER: Stock deposits: \t\t\t\t");
	srand(time(0));
	int i;
	for (i = 0; i < stock.size; i++) {
		if (pthread_mutex_init(&stock.mutexes[i], NULL) != 0) {
			fprintf(stderr, "MASTER: Error on pthread_mutex_init (stock %d)\n", i);
			exit(EXIT_FAILURE);
		}

		if (pthread_mutex_init(&stock.write_mutex, NULL) != 0) {
                        fprintf(stderr, "MASTER: Error on pthread_mutex_init (stock)\n");
                        exit(EXIT_FAILURE);
                }

		stock.cells[i] = 100 + rand() % 1901; // 100 - 2000;
		printf("\t%d", stock.cells[i]);
	}
	printf("\n");
	
	// Creating the uploader
	pthread_t uploader_thread;
	uploader_in_data_t uploader_in_data;
	pthread_attr_t uploader_attr;

	uploader_in_data.stock = &stock;
	
	if (pthread_attr_init(&uploader_attr) != 0) {
        	fprintf(stderr, "MASTER: Error on pthread_attr_init (uploader)\n");
                exit(EXIT_FAILURE);
        }
	pthread_attr_setdetachstate(&uploader_attr, PTHREAD_CREATE_JOINABLE);

	if (pthread_create(&uploader_thread, &uploader_attr, &uploader_work, &uploader_in_data) != 0) {
		fprintf(stderr, "MASTER: Error on pthread_create (uploader)\n");
		exit(EXIT_FAILURE);
	}

	// Creating the downloaders
        pthread_t downloader_threads[DOWNLOADERS_COUNT];
        downloader_in_data_t downloader_in_data[DOWNLOADERS_COUNT];
        pthread_attr_t downloader_attr[DOWNLOADERS_COUNT];

	for (i = 0; i < DOWNLOADERS_COUNT; i++) {
	        downloader_in_data[i].stock = &stock;
		downloader_in_data[i].id = i;
		downloader_in_data[i].need = (int *)malloc(sizeof(int) * stock.size);
		downloader_in_data[i].have = (int *)malloc(sizeof(int) * stock.size);

		int j;
		for (j = 0; j < stock.size; j++) {
			downloader_in_data[i].need[j] = 100 + rand() % 901; // 100 ... 1000
			downloader_in_data[i].have[j] = 0;
		}
	        if (pthread_attr_init(&downloader_attr[i]) != 0) {
			fprintf(stderr, "MASTER: Error on pthread_attr_init (downloader %d)\n", i);
	                exit(EXIT_FAILURE);
	        }
        	pthread_attr_setdetachstate(&downloader_attr[i], PTHREAD_CREATE_JOINABLE);

	        if (pthread_create(&downloader_threads[i], &downloader_attr[i], &downloader_work, &downloader_in_data[i]) != 0) {
	                fprintf(stderr, "MASTER: Error on pthread_create (downloader %d)\n", i);
                	exit(EXIT_FAILURE);
	        }
	}

	// Waiting the threads
/*	void *uploader_out_data;
	if (pthread_join(uploader_thread, &uploader_out_data) != 0) {
                fprintf(stderr, "MASTER: Error on pthread_join (uploader)\n");
                exit(EXIT_FAILURE);
        }
*/
	for (i = 0; i < DOWNLOADERS_COUNT; i++) {
		void *downloader_out_data;
		if (pthread_join(downloader_threads[i], &downloader_out_data) != 0) {
                	fprintf(stderr, "MASTER: Error on pthread_join (downloader %d)\n", i);
                	exit(EXIT_FAILURE);
		}
	}

	if (pthread_cancel(uploader_thread) != 0) {
                fprintf(stderr, "MASTER: Error on pthread_join (uploader)\n");
                exit(EXIT_FAILURE);
        }

	printf("The work is done!\n");
	for (i = 0; i < DOWNLOADERS_COUNT; i++) {
		printf("DOWNLOADER (%d): have/need deposits: ", i + 1);
	
		int j;
                for (j = 0; j < stock.size; j++)
			printf("\t\t%d/%d", downloader_in_data[i].have[j], downloader_in_data[i].need[j]);
		printf("\n");		
	}

	free(stock.cells);

	exit(EXIT_SUCCESS);
}

void *uploader_work(void *args)
{
	uploader_in_data_t *data = args;

	for ( ; ; )
	{
		int c = rand() % data->stock->size; // cell number
		int k = 100 + rand() % 401; // increment 100 .. 500

		pthread_mutex_lock(&data->stock->mutexes[c]);

		data->stock->cells[c] += k;

		pthread_mutex_lock(&data->stock->write_mutex);

		printf("UPLOADER: %d is uploaded into %d cell, deposits: \t", k, c + 1);
		int j;
		for (j = 0; j < data->stock->size; j++)
			if (j == c) printf("\t%d", data->stock->cells[j]);
			else printf("\t-");
		printf("\n");

		pthread_mutex_unlock(&data->stock->write_mutex);

		pthread_mutex_unlock(&data->stock->mutexes[c]);
	}

	return NULL;
}

void *downloader_work(void *args)
{
	downloader_in_data_t *data = args;

	for ( ; ; )
	{
		int c = rand() % data->stock->size; // cell number
                int k = 100 + rand() % 401; // increment 100 .. 500
		int getit = 0;

		pthread_mutex_lock(&data->stock->mutexes[c]);

		// If we need this resource and it is in the store, we can download it
		if ((data->have[c] < data->need[c]) && (data->stock->cells[c] >= k)) {
			data->stock->cells[c] -= k;
			data->have[c] += k;
			getit = 1;
		}
		
		pthread_mutex_lock(&data->stock->write_mutex);

		if (getit == 1) {
			printf("DOWNLOADER (%d): %d is downloaded from %d cell, deposits: ", data->id + 1, k, c + 1);
                	int j;
                	for (j = 0; j < data->stock->size; j++)
				if (j == c) printf("\t%d", data->stock->cells[j]);
				else printf("\t-");
                	printf("\n");
		}

		pthread_mutex_unlock(&data->stock->write_mutex);

		pthread_mutex_unlock(&data->stock->mutexes[c]);

		int j;
		int getall = 1;
		for (j = 0; j < data->stock->size; j++)
			if (data->have[j] < data->need[j]) {
				getall = 0;
				break;
			}

		if (getall == 1) break;
	}

	return NULL;
}
