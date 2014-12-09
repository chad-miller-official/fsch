#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "central.h"
#include "node.h"
#include "univ.h"

#define PRINT_USAGE() fprintf(stderr, \
	  "************************************** \
	 \n** MATH 4777 Project File Scheduler ** \
	 \n************************************** \
	 \nUsage: ./fsch <file directory> <archive directory> <search key> [scheduling algorithm] [priority options] \
	 \n Scheduling algorithms: \
	 \n   -c  = Cyclic distribution (default) \
	 \n   -b  = Block distribution \
	 \n   -r  = Random distribution \
	 \n   -qs = Queue size distribution \
	 \n   -ql = Queue length distribution \
	 \n Priority options: \
	 \n   -n  = No priority (default) \
	 \n   -op = Oldest files given priority\n")

/* Static unction prototypes */
static void central_work();
static void node_work();

/* univ.h extern variables */
char *file_dir_str;
char *archive_dir_str;
char *search_key;
int proc_count;
int proc_id;
int sched_type;
int priority_option;

int main(int argc, char *argv[])
{
	//Make sure we have file and archive directories and search key to work with
    if(argc < 4)
    {
    	PRINT_USAGE();
        return -1;
    }
    
    //Initialize MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &proc_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
	
	srand(time(NULL));	//Seed the generator
    
	//Set the file directory to work with
	int file_len = strlen(argv[1]);
	file_dir_str = malloc(file_len);
	strcpy(file_dir_str, argv[1]);
	
	//If the directory doesn't end with '/', we need to append it
	if(file_dir_str[file_len - 1] != '/')
	{
		//So allocate an additional spot for it
		file_dir_str = realloc(file_dir_str, file_len + 1);
		strcat(file_dir_str, "/");
	}
	
	//Set the archive directory to work with
	int archive_len = strlen(argv[2]);
	archive_dir_str = malloc(archive_len);
	strcpy(archive_dir_str, argv[2]);
	
	//If the directory doesn't end with '/', we need to append it it
	if(archive_dir_str[archive_len - 1] != '/')
	{
		//So allocate an additional spot for it
		archive_dir_str = realloc(archive_dir_str, archive_len + 1);
		strcat(archive_dir_str, "/");
	}
	
	//Get the word to search for
	search_key = malloc(strlen(argv[3]));
	strcpy(search_key, argv[3]);
	
	//First, set the default scheduling algorithm and priority option
	sched_type = CYCLIC;
	priority_option = NO_PRIORITY;

	//If the user specified a priority option, use it
	if(argc >= 6)
	{
		switch(argv[5][1])
		{
			case 'n':
				priority_option = NO_PRIORITY;
				break;
			case 'o':
				switch(argv[5][2])
				{
					case 'p':
						priority_option = OLDEST_FILE_PRIORITY;
						break;
					default:
						PRINT_USAGE();
						return -1;
				}
				
				break;
			default:
				PRINT_USAGE();
				return -1;
		}
	}
	
	//If the user specified a scheduling algorithm, use it
	if(argc >= 5)
	{
		switch(argv[4][1])
		{
			case 'c':
				sched_type = CYCLIC;
				break;
			case 'b':
				sched_type = BLOCK;
				break;
			case 'r':
				sched_type = RANDOM;
				break;
			case 'q':
				switch(argv[4][2])
				{
					case 's':
						sched_type = QUEUE_SIZE;
						break;
					case 'l':
						sched_type = QUEUE_LENGTH;
						break;
					default:
						PRINT_USAGE();
						return -1;
				}
				
				break;
			default:
				PRINT_USAGE();
				return -1;
		}
	}
	
	clock_t start = clock();	//Get the start time
	
	if(proc_id == CENTRAL)
		init_central();	//If we're the central machine, initialize us as the central machine
	else
		init_node();	//Otherwise, initialize us as a node
	
	MPI_Barrier(MPI_COMM_WORLD);	//Wait for everyone to finish initializing before continuing
	
    if(proc_id == CENTRAL)
        central_work();	//If we're the central machine, do central machine work
    else
        node_work();	//Otherwise, do node work
    
    MPI_Barrier(MPI_COMM_WORLD);	//Wait for everyone to finish doing what they're doing
    
    if(proc_id == CENTRAL)
    	central_cleanup();	//If we're the central machine, finalize us as the central machine
    else
    	node_cleanup();	//Otherwise, finalize us as a node
    
    //Free all strings we malloc()'d
    free(file_dir_str);
    free(archive_dir_str);
    free(search_key);
    
    MPI_Barrier(MPI_COMM_WORLD);	//And wait for everyone to finish cleaning up
    
    //If we're the central machine, we should be the last to exit, so get the total time this ran for
    if(proc_id == CENTRAL)
    {
    	clock_t now = clock();
    	int diff = (int) (now - start);
    	float seconds = (float) diff / CLOCKS_PER_SEC;
    	printf("TOTAL RUNTIME: %f seconds!\n", seconds);
    }
    
    MPI_Finalize();
    return 0;
}

static void central_work()
{
	int total_files = enqueue_all_files();	//Get the total number of files we found
	files_per_proc = (proc_count > 1) ? total_files / (proc_count - 1) : 1;	//Get the number of files per node for block scheduling
	
	//Adjust so we don't accidentally leave any nodes out
	if(total_files % (proc_count - 1) != 0)
		files_per_proc--;
	
	//For each file we found...
    while(queue_size(all_files) > 0)
    {	
    	int file_size, priority;
    	char *filename = dequeue(all_files, &file_size, &priority);	//Get the file...
	    int best_proc = get_best_proc();	//...and get the best node to send this to
	    
	    //And send the node all of its information
	    MPI_Send(filename, FILE_NAME_LEN, MPI_CHAR, best_proc, FILE_NAME_TAG, MPI_COMM_WORLD);
	    MPI_Send(&file_size, 1, MPI_INT, best_proc, FILE_SIZE_TAG, MPI_COMM_WORLD);
	    MPI_Send(&priority, 1, MPI_INT, best_proc, FILE_PRIORITY_TAG, MPI_COMM_WORLD);
    }
    
    //Tell everyone else there's no more files left
    int stop = 1;
    
    for(int i = 1; i < proc_count; i++)
    	MPI_Send(&stop, 1, MPI_INT, i, STOP_TAG, MPI_COMM_WORLD);
}

static void node_work()
{
	int out_of_files = 0;
	
	//While the central machine is still sending us files...
    while(!out_of_files)
    {
    	//See what kind of message we're getting
    	MPI_Status status;
		MPI_Probe(CENTRAL, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		
		switch(status.MPI_TAG)
		{
			case FILE_NAME_TAG:	//Get a file
			{
				//Get the file's information
				char filename[FILE_NAME_LEN];
				int file_size, priority;
				MPI_Recv(filename, FILE_NAME_LEN, MPI_CHAR, CENTRAL, FILE_NAME_TAG, MPI_COMM_WORLD, &status);
				MPI_Recv(&file_size, 1, MPI_INT, CENTRAL, FILE_SIZE_TAG, MPI_COMM_WORLD, &status);
				MPI_Recv(&priority, 1, MPI_INT, CENTRAL, FILE_PRIORITY_TAG, MPI_COMM_WORLD, &status);
				
				enqueue(file_queue, filename, file_size, priority);	//And then enqueue it
				
				//If we're using a scheduling algorithm that depends on node data, send it to the central machine
				if(sched_type == QUEUE_SIZE || sched_type == QUEUE_LENGTH)
				{
					int data;
					
					switch(sched_type)
					{
						case QUEUE_SIZE:
							data = file_queue->sum_file_size;
							break;
						case QUEUE_LENGTH:
							data = file_queue->size;
							break;
						default:
							data = -1;	//Something went really wrong
							break;
					}

					printf("%d is sending data...\n", proc_id);
					MPI_Send(&data, 1, MPI_INT, CENTRAL, QUEUE_DATA_TAG, MPI_COMM_WORLD);
					printf("%d sent data!\n", proc_id);
				}
			} break;
			case STOP_TAG:	//Break us out of this loop
				MPI_Recv(&out_of_files, 1, MPI_INT, CENTRAL, STOP_TAG, MPI_COMM_WORLD, &status);
				break;
		}
    }
}

