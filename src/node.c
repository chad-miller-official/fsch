#include <mpi.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"
#include "univ.h"

/* Static function prototypes */

/*
 * Read a line from an MPI file.
 * Params: file - the MPI file to read from.
 *         from - the position at the file to start reading from.
 *         buffer - a char array with enough space allocated to read a line
 *         from the file.
 *         status - a status struct to record status to.
 * Returns: the new position of the file pointer, or -1 if we reached the end of
 *          the file.
 */
static int read_line(MPI_File file, int from, char *buffer, MPI_Status status);

/*
 * The function the process thread should run. Dequeues files from the file
 * queue and calls process() on them.
 * Params: nothing - should always be NULL.
 * Returns: NULL every time.
 */
static void* process_thread_func(void *nothing);

/*
 * Takes a line with a key/value pair and forms a key/value pair struct
 * out of it.
 * Params: line - the line containing the key/value pair.
 * Returns: a key/value pair containing the key and value in the line.
 */
static kv_pair_t get_kv_pair(char *line);

/*
 * Burns a specified number of processor cycles. Essentially just a for-loop
 * that runs for a specific number of iterations and does nothing else.
 * Params: num_cycles - the number of cycles that should be burned.
 * Returns: nothing.
 */
static void burn_cycles(int num_cycles);

/* node.h extern variables */
file_queue_t *file_queue;
pthread_t process_thread;

/* Static variables */
static int do_process = 1;	//Boolean value that tells us when to stop waiting for more files to process

void init_node()
{	
	//Initialize our queue
	file_queue = malloc(sizeof(file_queue_t));
	file_queue = init_queue(file_queue);
	
	pthread_create(&process_thread, NULL, process_thread_func, NULL);	//Create our process() thread
}

//This returns void* and takes in void* because pthread needs it to
static void* process_thread_func(void *nothing)
{
	//We don't actually use the parameter for anything
	
	//We want to keep doing this while we can still expect files, or while there are still files to process
	while(do_process || queue_size(file_queue) > 0)
	{
		int file_size, priority;
		char *file = dequeue(file_queue, &file_size, &priority);	//Dequeue a file
		
		//If the file's not NULL, process it
		if(file != NULL)
			process(file);
	}
	
	return NULL;	//We actually don't return anything useful
}

void process(char *filename)
{
	//Open the file for reading
	MPI_File file;
	MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &file);
    
    char line[LINE_NUM_CHARS];
    int start = 0;
    MPI_Status status;
	
	//Keep going until we reach the end of the file
    while(start != -1)
    {
    	//Read a line
    	memset(line, 0, LINE_NUM_CHARS);
    	start = read_line(file, start, line, status);
    	
    	//If we didn't encounter an EOF while reading the line...
    	if(start != -1)
    	{
			kv_pair_t kv_pair = get_kv_pair(line);	//Get a key/value pair from it
			
			//If the key matches the key we want...
			if(!strcmp(kv_pair.key, search_key))
			{
				printf("\n%d found value from %s! Original: %s, Key=%s, Value=%s\n", proc_id, filename, line, kv_pair.key, kv_pair.value);
				MPI_Send(filename, strlen(filename), MPI_CHAR, CENTRAL, ARCHIVE_TAG, MPI_COMM_WORLD);	//Archive it
				burn_cycles(500);	//Instead of doing actual database stuff, just burn 500 cycles to simulate writing
				
				//And get us out of here, because we've finished our job
				free(kv_pair.key);
				free(kv_pair.value);
				break;
			}
				
			free(kv_pair.key);
			free(kv_pair.value);
    	}
    }
    
    printf("\n");
    MPI_File_close(&file);	//Close the file
}

void node_cleanup()
{
	do_process = 0;	//Tell us to stop expecting new files
	pthread_join(process_thread, NULL);	//Join the process thraed
	free_queue(file_queue);	//Free our file queue
	
	//Tell the central machine we've stopped
	int stop = 1;
	MPI_Send(&stop, 1, MPI_INT, CENTRAL, STOP_TAG, MPI_COMM_WORLD);
}

static int read_line(MPI_File file, int from, char *buffer, MPI_Status status)
{
	int count = 0;	//Counting variable
	
	//Character to append and read status
	char append;
	int num_read;

	//Read a character and get the number of characters read	
	MPI_File_read(file, &append, 1, MPI_CHAR, &status);
	MPI_Get_count(&status, MPI_CHAR, &num_read);
	
	//Build string as long as we're getting successful reads and stop when we hit a newline
	while(num_read >= 1 && append != '\n')
	{
		buffer[count++] = append;	//Add it to the buffer
		
		//Read a character and get the number of characters read
		MPI_File_read(file, &append, 1, MPI_CHAR, &status);
		MPI_Get_count(&status, MPI_CHAR, &num_read);
	}
	
	//If we reached the end of the file, return -1; otherwise, return the position of the file pointer
	return (num_read < 1) ? -1 : (from + count);
}

static kv_pair_t get_kv_pair(char *line)
{
	int line_len = strlen(line);	//Get the length of the line
	int equals_index = strcspn(line, "=");	//Get the index of the '=' separating the key and value
	int value_len = line_len - equals_index;	//Get the length of the value
	
	kv_pair_t retval;
	
	//Copy the key into the key/value pair we'll return
	retval.key = malloc(equals_index + 1);
	strncpy(retval.key, line, equals_index);
	retval.key[equals_index] = '\0';
	
	//Copy the value into the key/value pair and we'll return
	retval.value = malloc(value_len + 1);
	strncpy(retval.value, line + equals_index + 1, value_len);
	retval.value[value_len] = '\0';
	
	return retval;
}

static void burn_cycles(int num_cycles)
{
	volatile int i;	//Volatile means GCC won't optimize this function away
	printf("Inserting into database!");
	
	for(i = 0; i < num_cycles; i++);	//Just waste iterations
}

