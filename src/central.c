#include <dirent.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "central.h"
#include "univ.h"

/* Static function prototypes */

/*
 * The function the archive thread should run. Listens for messages from other
 * processors.
 * Params: nothing - should always be NULL.
 * Returns: NULL every time
 */
static void* archive_thread_func(void *nothing);

/*
 * Helper function to find the next best processor if we're using cyclic
 * distribution.
 * Params: nothing
 * Returns: the rank of the next best processor to send to according to cyclic
 *          distribution.
 */
static int get_best_proc_cyclic();

/*
 * Helper function to find the next best processor if we're using block
 * distribution.
 * Params: nothing
 * Returns: the rank of the next best processor to send to according to block
 *          distribution.
 */
static int get_best_proc_block();

/*
 * Helper function to find the next best processor if we're using random
 * distribution. Inline because it's just a one-liner.
 * Params: nothing
 * Returns: the rank of the next best processor to send to according to
 *          random distribution.
 */
static inline int get_best_proc_random();

/*
 * Helper function to find the next best processor if we're using a scheduling
 * method that requires node data.
 * Params: nothing
 * Returns: the rank of the next best processor to send to according to the
 *          scheduling method we're using that requires queue data.
 */
static int get_best_proc_queue_data();

/* central.h extern variables */
int *node_stats;
file_queue_t *all_files;
int file_count;
pthread_t archive_thread;
int files_per_proc = 1;

/* Static variables */
static int stop_counter = 1;	//Counts the number of STOP signals we receive from nodes

void init_central()
{
	//Initialize the file queue
	all_files = malloc(sizeof(file_queue_t));
	init_queue(all_files);
	
	pthread_create(&archive_thread, NULL, archive_thread_func, NULL);	//Create the archive thread
	
	//If we're using a scheduling algorithm that requires node stats, initialize the node stats array
	if(sched_type == QUEUE_SIZE || sched_type == QUEUE_LENGTH)
	{
		node_stats = malloc(sizeof(int) * proc_count);
		memset(node_stats, 0, sizeof(int) * proc_count);
	}
}

//This returns void* and takes in void* because pthread needs it to
static void* archive_thread_func(void *nothing)
{
	//We don't actually use the parameter for anything
	
	//Do this until we receive a number of STOPs equal to the number of nodes this program was initialized with
	while(stop_counter < proc_count)
	{
		//Check to see if we got a message
		MPI_Status status;
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		
		switch(status.MPI_TAG)
		{
			case ARCHIVE_TAG:	//We got a signal to archive
			{
				//Get the file we should archive...
				char file_path[FILE_NAME_LEN];
				memset(file_path, 0, FILE_NAME_LEN);
				MPI_Recv(file_path, FILE_NAME_LEN, MPI_CHAR, status.MPI_SOURCE, ARCHIVE_TAG, MPI_COMM_WORLD, &status);
				move_file(file_path);	//...and archive it
			} break;
			case STOP_TAG:	//We got a signal to increment the stop counter
			{
				int plusone;
				MPI_Recv(&plusone, 1, MPI_INT, status.MPI_SOURCE, STOP_TAG, MPI_COMM_WORLD, &status);
				stop_counter += plusone;	//Increment the stop counter by 1
			} break;
			case QUEUE_DATA_TAG:	//We received queue data
			{
				int dat;
				MPI_Recv(&dat, 1, MPI_INT, status.MPI_SOURCE, QUEUE_DATA_TAG, MPI_COMM_WORLD, &status);
				node_stats[status.MPI_SOURCE] = dat;	//Update node data array
			} break;
		}
	}
	
	return NULL;	//And we actually don't return anything useful
}

int enqueue_all_files()
{
	DIR *file_dir = opendir(file_dir_str);	//Open the work directory stream
    struct dirent *subdir = NULL;	//Pointer to a directory entry
    
    //Iterate through all filenames in the directory
    while((subdir = readdir(file_dir)) != NULL)
    {
    	//If the name is valid, we should do something with it
        if(file_name_valid(subdir->d_name))
        {
        	//Get the full path to the file
        	char filename[FILE_NAME_LEN];
        	strcpy(filename, file_dir_str);
        	strcat(filename, subdir->d_name);
        	
        	//Get the size of the file
        	FILE *file = fopen(filename, "r");
        	fseek(file, 0, SEEK_END);
        	int file_size = ftell(file);
        	fclose(file);
        	
        	//Set the file priority (1 unless we specified a priority option)
        	int priority = 1;
        	
        	if(priority_option == OLDEST_FILE_PRIORITY)
        	{
		    	char *priority_string = strchr(subdir->d_name, (int) '_') + 1;
		    	priority = -atoi(priority_string);
        	}
        	
		    enqueue(all_files, filename, file_size, priority);	//Enqueue the file
		}
    }
    
    closedir(file_dir);	//Close the work directory stream
    return (file_count = all_files->size);	//Set file count while returning it
}

void move_file(char *filepath)
{
	char *file_name = strrchr(filepath, (int) '/') + 1;	//Get the file name only
	
	//Get the full new path to the file
	char new_path[FILE_NAME_LEN];
	memset(new_path, 0, FILE_NAME_LEN);
	strcpy(new_path, archive_dir_str);
	strcat(new_path, file_name);
	
	printf("Moving from %s to %s\n", filepath, new_path);
	rename(filepath, new_path);	//Rename the file, i.e. move it
}

int file_name_valid(char *filename)
{
	//If the filename is a directory, it's definitely not valid
	if(filename[0] == '.')
		return 0;
	
	return !strcmp((filename + strlen(filename) - 4), ".sen");	//Check if filename ends with ".sen"
}

int get_best_proc()
{
	//Depending on the scheduling type, return the value a helper function returns
	switch(sched_type)
	{
		case CYCLIC:
			return get_best_proc_cyclic();
		case BLOCK:
			return get_best_proc_block();
		case RANDOM:
			return get_best_proc_random();
		case QUEUE_SIZE:
			return get_best_proc_queue_data();
		case QUEUE_LENGTH:
			return get_best_proc_queue_data();
		default:
			return -1;	//Something went really wrong
	}
}

static int get_best_proc_cyclic()
{
	static int proc_counter = 0;	//Keep track of which processor we last left off at
	
	int retval = (proc_counter % (proc_count - 1)) + 1;	//Get the processor we left off at
	proc_counter++;	//Increment so next time, we get the next cyclic processor
	return retval;
}

static int get_best_proc_block()
{
	static int proc_counter = 0;	//Keep track of which processor we last left off at
	static int block_counter = 0;	//Keep track of how many files we've sent to it so far
	
	int retval = (proc_counter % (proc_count - 1)) + 1;	//Get the processor we left off at
	block_counter++;	//Increment the number of files we've sent to it by 1
	
	//If it's greater than the number of files it should be getting, increment so we get the next processor next time
	if(block_counter >= files_per_proc)
	{
		proc_counter++;
		block_counter = 0;
	}
	
	return retval;
}

static inline int get_best_proc_random()
{
	return (rand() % (proc_count - 1)) + 1;	//Just get a random processor between [1, # of nodes)
}

static int get_best_proc_queue_data()
{
	//Find the node with minimum "x", where x is some metric
	int min = 1;
	
	for(int i = 1; i < proc_count; i++)
		if(node_stats[i] < node_stats[min])
			min = i;
	
	return min;
}

void central_cleanup()
{
	pthread_join(archive_thread, NULL);	//Join the archive thread
	
	//Free the node stats array if we initialized it
	if(sched_type == QUEUE_SIZE || sched_type == QUEUE_LENGTH)
		free(node_stats);
	
	//Free the file queue
	free_queue(all_files);
}

