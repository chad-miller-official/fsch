#ifndef CENTRAL_H_INCLUDED
#define CENTRAL_H_INCLUDED

#include <pthread.h>
#include "container.h"

/* Central machine variables */
extern int *node_stats;				//Array of node stats for certain scheduling algorithms

extern file_queue_t *all_files;		//Queue of all files we found
extern pthread_t archive_thread;	//Thread that performs all receives (particularly signals to archive)
extern int file_count;				//Number of files we found
extern int files_per_proc;			//Number of files each processor should get for block scheduling

/* Central machine functions */

/*
 * Initializes us as the central machine.
 * Params: nothing
 * Returns: nothing
 */
void init_central();

/*
 * Iterates through all files in the file directory and enqueues them in
 * all_files.
 * Params: nothing
 * Returns: the number of files inserted into the queue.
 */
int enqueue_all_files();

/*
 * Moves a file from the file directory to the archive directory.
 * Params: filepath - the full path to the file.
 * Returns: nothing
 */
void move_file(char *filepath);

/*
 * Checks if a file name is valid. A file name is considered valid if it does
 * not begin with "." and ends with ".sen."
 * Params: filename - the full path to the file.
 * Returns: 1 if the file name is valid; 0 otherwise.
 */
int file_name_valid(char *filename);

/*
 * Returns the best node to send the next file to.
 * Params: nothing
 * Returns: the rank of the best node to send the next file to.
 */
int get_best_proc();

/*
 * Finalizes us as the central machine.
 */
void central_cleanup();

#endif //CENTRAL_H_INCLUDED

