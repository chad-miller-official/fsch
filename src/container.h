#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include <pthread.h>
#include <stdio.h>

/* Defines a file node for a file queue */
typedef struct _file_node_t {
	char *file;					//File name
	int file_size;				//File size
	int priority;				//File priority
	struct _file_node_t *next;	//Pointer to next file in queue
} file_node_t;

/* Defines a thread-safe file priority queue */
typedef struct _file_queue_t {
	file_node_t *head;				//Head file node
	int size;						//Number of files in queue
	int sum_file_size;				//Sum of file sizes of all files in queue
	int modifying;					//1 if this queue is being modified; 0 otherwise
	pthread_mutex_t queue_mutex;	//Mutex for thread safety
	pthread_cond_t enqueue_cond;	//Enqueue condition variable
	pthread_cond_t dequeue_cond;	//Dequeue condition variable
	pthread_cond_t read_cond;		//General purpose read condition variable
} file_queue_t;

/* QUEUE STUFF */

/*
 * Initializes a queue.
 * Params: queue - a file queue that has already been allocated via malloc().
 * Returns: queue, after it's been initialized.
 */
file_queue_t* init_queue(file_queue_t *queue);

/*
 * Enqueues a file based on its priority. The queue is always sorted by
 * priority. When there are two or more files with the same priority level,
 * the file that was enqueued first has highest priority.
 * Params: queue - the queue to enqueue to.
 *         filename - the name of the file to enqueue.
 *         file_size - the size of the file to enqueue.
 *         priority - the priority of the file to enqueue.
 * Returns: 0 if enqueueing was successful; a nonzero value otherwise.
 */
int enqueue(file_queue_t *queue, char *filename, int file_size, int priority);

/*
 * Dequeues a file. Returns the file with the highest priority that was
 * enqueued the longest time ago.
 * Params: queue - the file to dequeue from.
 *         file_size - a single int buffer that will contain the size of the
 *         file dequeued on return.
 *         priority - a single int buffer that will contain the priority of
 *         the file dequeued on return.
 * Returns: the name of the file that was dequeued.
 */
char* dequeue(file_queue_t *queue, int *file_size, int *priority);

/*
 * Gets the number of files in the queue.
 * Params: queue - the queue whose number of files should be returned.
 * Returns: the number of files in the queue.
 */
int queue_size(file_queue_t *queue);

/*
 * Gets the sum of all file sizes in the queue.
 * Params: queue - the queue whose sum of file sizes should be returned.
 * Returns: the sum of all file sizes in the queue.
 */
int queue_sum_file_size(file_queue_t *queue);

/*
 * Finalizes a queue.
 * Params: queue - the queue that should be finalized.
 * Returns: 0 if finalization was successful; a nonzero value otherwise.
 */
int free_queue(file_queue_t *queue);

#endif //QUEUE_H_INCLUDED

