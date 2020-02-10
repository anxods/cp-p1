/*Exercise 2 (Protect the critical section with a mutex for each position of the array)
Adding a single mutex to protect the array prevents the corruption of the values. However, when
a thread locks the mutex to swap two positions the entire array is locked. We could increase
performance by allowing threads that want to swap different positions into the critical section
at the same time. In this exercise we will do so by protecting each position of the array with a
separate mutex.*/

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "options.h"

struct buffer {
	int *data;
	int size;
};

struct thread_info {
	pthread_t       thread_id;        // id returned by pthread_create()
	int             thread_num;       // application defined thread #
};

struct args {
	int 		thread_num;       // application defined thread #
	int 	        delay;			  // delay between operations
	int		iterations;
	struct buffer   *buffer;		  // Shared buffer
	pthread_mutex_t *mutexes;	//share mutex
};


void *swap(void *ptr)
{
	struct args *args =  ptr;

	while(args->iterations--) {
		int i,j, tmp;
		
		i=rand() % args->buffer->size;
		j=rand() % args->buffer->size;

		printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n",
		 	args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);
				
		if (i != j) {
			while(1) {
				pthread_mutex_lock(&(args->mutexes[j]));
				if (pthread_mutex_trylock(&args->mutexes[i]) == 0)
					break;
				pthread_mutex_unlock(&(args->mutexes[j]));
			}
		} else {
			while(1){
				if (pthread_mutex_trylock(&args->mutexes[i]) == 0)
					break;
				continue;
			}
		}

			tmp = args->buffer->data[i];
			if(args->delay) usleep(args->delay); // Force a context switch

			args->buffer->data[i] = args->buffer->data[j];
			if(args->delay) usleep(args->delay);

			args->buffer->data[j] = tmp;

			if(args->delay) usleep(args->delay);  //se destruye el thread al acabar la funcion

			pthread_mutex_unlock(&(args->mutexes[i]));

			if (i != j){
				pthread_mutex_unlock(&(args->mutexes[j]));
			}
	}

	return NULL;
}

void print_buffer(struct buffer buffer) {
	int i;

	for (i = 0; i < buffer.size; i++)
		printf("%i ", buffer.data[i]);

	printf("\n");
}

int checkBuffer(struct buffer buffer) {
	int i,aux = 0;

	for (i=0; i<buffer.size; i++)
		aux += buffer.data[i];

	return aux == 45;
}

void start_threads(struct options opt){

	int i;
	struct thread_info *threads;
	struct args *args;
	struct buffer buffer;
	pthread_mutex_t mutex[opt.buffer_size];
	clock_t t; 

	srand(time(NULL));

	if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {
		printf("Out of memory\n");
		exit(1);
	}

	buffer.size = opt.buffer_size;

	for(i=0; i<buffer.size; i++)
		buffer.data[i]=i;

	for (i = 0; i < buffer.size; i++) {
	    if  (pthread_mutex_init( &mutex[i], NULL) != 0) {
				printf("\n pthread_mutex_init operation failed\n");
		} else
			printf(" Mutex %d created\n",i );
	}

	printf("\n"); printf(" Creating %d threads\n", opt.num_threads); printf("\n");
	threads = malloc(sizeof(struct thread_info) * opt.num_threads);
	args = malloc(sizeof(struct args) * opt.num_threads);

	if (threads == NULL || args==NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	printf("Buffer before: "); 
	print_buffer(buffer); printf("\n");

	t = clock();

	// Create num_thread threads running swap() 
	for (i = 0; i < opt.num_threads; i++) {
		threads[i].thread_num = i;
		
		args[i].thread_num = i;
		args[i].buffer     = &buffer;
		args[i].delay      = opt.delay;
		args[i].iterations = opt.iterations;
		args[i].mutexes  = mutex;

		if ( 0 != pthread_create(&threads[i].thread_id, NULL, swap, &args[i])) {
			printf("Could not create thread #%d", i);
			exit(1);
		}
	}
	
	// Wait for the threads to finish
	for (i = 0; i < opt.num_threads; i++)
		pthread_join(threads[i].thread_id, NULL);

	t = clock() - t;

	// Print the buffer
	printf("\n");
	printf("Buffer after:  ");
	print_buffer(buffer);
	printf("\n");
		
	if (checkBuffer(buffer))
		printf("Correct buffer\n");
	else
		printf("Incorrect buffer\n");

	printf("\n");
	double time_taken = ((double)t)/CLOCKS_PER_SEC; 
	printf("start_threads() took %f seconds to execute \n", time_taken); 

	free(args);
	free(threads);
	free(buffer.data);
	for (i = 0; i < buffer.size; i++) {
		if  (pthread_mutex_destroy(&(mutex[i])) != 0) {
			printf("\n mutex destroy failed\n");
		} 
	}
        
	pthread_exit(NULL);
}

int main (int argc, char **argv){

	struct options opt;
	
	// Default values for the options
	opt.num_threads = 10;
	opt.buffer_size = 10;
	opt.iterations  = 100;
	opt.delay       = 10;
	
	read_options(argc, argv, &opt);

	start_threads(opt);

	exit(0);
}
