#include <stdlib.h>
#include <string.h>

#include "container.h"

file_queue_t* init_queue(file_queue_t *queue)
{
	//Initialize queue contents to their default values
	queue->head = NULL;
	queue->size = 0;
	queue->sum_file_size = 0;
	queue->modifying = 0;
	
	pthread_mutex_init(&(queue->queue_mutex), NULL);	//Initialize the queue mutex
	
	//Initialize the queue condition variables
	pthread_cond_init(&(queue->enqueue_cond), NULL);
	pthread_cond_init(&(queue->dequeue_cond), NULL);
	pthread_cond_init(&(queue->read_cond), NULL);
	
	return queue;
}

int enqueue(file_queue_t *queue, char *filename, int file_size, int priority)
{
	//Do nothing if the queue is NULL
	if(queue == NULL)
		return 1;
	
	//Wait for our turn to access the queue
	pthread_mutex_lock(&(queue->queue_mutex));
	
	while(queue->modifying)
		pthread_cond_wait(&(queue->enqueue_cond), &(queue->queue_mutex));
	
	queue->modifying = 1;	//Make sure other threads know we're modifying this
	
	//Initialize the node to be added
	file_node_t *add = malloc(sizeof(file_node_t));
	add->file = malloc(strlen(filename));
	strcpy(add->file, filename);
	add->file_size = file_size;
	add->priority = priority;
	add->next = NULL;
	
	//Then add the node to the queue
	if(queue->size == 0)
		queue->head = add;	//If there's no files, just make it the head
	else if(add->priority > queue->head->priority)
	{
		//If the priority of this file is greater than the head's priority, add it behind the head
		add->next = queue->head;
		queue->head = add;
	}
	else
	{	
		//Otherwise, find the proper spot for this file
		file_node_t *temp = queue->head;
		
		while(temp->next != NULL)
		{
			if(temp->next->priority < add->priority)
			{
				add->next = temp->next;
				break;
			}
			
			temp = temp->next;
		}
		
		temp->next = add;
	}
	
	//Increment the queue sizes
	queue->size++;
	queue->sum_file_size += file_size;
	
	queue->modifying = 0;	//We're no longer modifying this

	//Signal the condition variables and unlock the mutex
	pthread_cond_signal(&(queue->dequeue_cond));
	pthread_cond_signal(&(queue->read_cond));
	pthread_mutex_unlock(&(queue->queue_mutex));

	return 0;
}

//filename and file_size are the return values
char* dequeue(file_queue_t *queue, int *file_size, int *priority)
{
	//If the queue is NULL or there's nothing to dequeue, return NULL
	if(queue == NULL || queue->size == 0)
		return NULL;
	
	//Wait for our turn to access the queue
	pthread_mutex_lock(&(queue->queue_mutex));
	
	while(queue->modifying)
		pthread_cond_wait(&(queue->dequeue_cond), &(queue->queue_mutex));
	
	queue->modifying = 1;	//Make sure other threads know we're modifying this
	
	//Dequeue the head, because the head always has highest priority
	file_node_t *oldhead = queue->head;
	
	char *filename = oldhead->file;
	*file_size = oldhead->file_size;
	*priority = oldhead->priority;
	
	//Then make the next node the head
	queue->head = queue->head->next;
	queue->size--;
	queue->sum_file_size -= oldhead->file_size;
	free(oldhead);
	
	queue->modifying = 0;	//We're no longer modifying this
	
	//Signal the condition variables and unlock the mutex
	pthread_cond_signal(&(queue->enqueue_cond));
	pthread_cond_signal(&(queue->read_cond));
	pthread_mutex_unlock(&(queue->queue_mutex));
	
	return filename;
}

int queue_size(file_queue_t *queue)
{
	//If the queue is NULL, return a size of 0
	if(queue == NULL)
		return 0;
	
	//Wait for our turn to access the queue
	pthread_mutex_lock(&(queue->queue_mutex));
	
	while(queue->modifying)
		pthread_cond_wait(&(queue->read_cond), &(queue->queue_mutex));
	
	//Quickly "modify" the queue by getting its size
	queue->modifying = 1;
	int retval = queue->size;
	queue->modifying = 0;
	
	//Signal the condition variables and unlock the mutex
	pthread_cond_signal(&(queue->enqueue_cond));
	pthread_cond_signal(&(queue->dequeue_cond));
	pthread_mutex_unlock(&(queue->queue_mutex));
	
	return retval;
}

int queue_sum_file_size(file_queue_t *queue)
{
	//If the queue is NULL, return a size of 0
	if(queue == NULL)
		return 0;
	
	//Wait for our turn to access the queue
	pthread_mutex_lock(&(queue->queue_mutex));
	
	while(queue->modifying)
		pthread_cond_wait(&(queue->read_cond), &(queue->queue_mutex));
	
	//Quickly "modify" the queue by getting the sum of its file sizes
	queue->modifying = 1;
	int retval = queue->sum_file_size;
	queue->modifying = 0;
	
	//Signal the condition variables and unlock the mutex
	pthread_cond_signal(&(queue->enqueue_cond));
	pthread_cond_signal(&(queue->dequeue_cond));
	pthread_mutex_unlock(&(queue->queue_mutex));
	
	return retval;
}

int free_queue(file_queue_t *queue)
{
	//Free every node in the queue
	file_node_t *node = queue->head;
	
	while(node != NULL)
	{
		file_node_t *nextnode = node->next;
		free(node->file);
		free(node);
		node = nextnode;
	}
	
	//Destroy all thread-related variables
	pthread_mutex_destroy(&(queue->queue_mutex));
	pthread_cond_destroy(&(queue->enqueue_cond));
	pthread_cond_destroy(&(queue->dequeue_cond));
	pthread_cond_destroy(&(queue->read_cond));
	
	free(queue);	//And free the queue
	return 0;
}

