#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "container.h"

#define PRINT_USAGE() fprintf(stderr, \
	  "************************************** \
	 \n** MATH 4777 Project File Scheduler ** \
	 \n************************************** \
	 \nUsage: ./fsch_serial <file directory> <archive directory> <search key>\n")
	 
#define FILE_NAME_LEN 80
#define LINE_NUM_CHARS 80

/* Represents a key/value pair */
typedef struct _kv_pair_t {
	char *key;
	char *value;
} kv_pair_t;

/* Function prototypes */

/*
 * Iterates through all files in the file directory and enqueues them in
 * all_files.
 * Params: nothing
 * Returns: the number of files inserted into the queue.
 */
int enqueue_all_files();

/*
 * Checks if a file name is valid. A file name is considered valid if it does
 * not begin with "." and ends with ".sen."
 * Params: filename - the full path to the file.
 * Returns: 1 if the file name is valid; 0 otherwise.
 */
int file_name_valid(char *filename);

/*
 * Process a file. Search for a specific key, and if the file contains a
 * key/value pair with that key, insert it into a database and archive it.
 * Params: filename - full path to the file that should be processed
 * Returns: nothing
 */
void process(char *filename);

/*
 * Takes a line with a key/value pair and forms a key/value pair struct
 * out of it.
 * Params: line - the line containing the key/value pair.
 * Returns: a key/value pair containing the key and value in the line.
 */
kv_pair_t get_kv_pair(char *line);

/*
 * Moves a file from the file directory to the archive directory.
 * Params: filepath - the full path to the file.
 * Returns: nothing
 */
void move_file();

/*
 * Burns a specified number of processor cycles. Essentially just a for-loop
 * that runs for a specific number of iterations and does nothing else.
 * Params: num_cycles - the number of cycles that should be burned.
 * Returns: nothing.
 */
void burn_cycles(int num_cycles);

/* Variables */
char *file_dir_str;			//File directory
char *archive_dir_str;		//Archive directory
char *search_key;			//Key to look for in each file
file_queue_t *all_files;	//Queue of all files we found

int main(int argc, char *argv[])
{
	//Make sure we have file and archive directories to work with
    if(argc < 3)
    {
    	PRINT_USAGE();
        return -1;
    }
	
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

	clock_t start = clock();	//Get the start time
	
	//Initialize the file queue and enqueue all files
    all_files = malloc(sizeof(file_queue_t));
	init_queue(all_files);
    enqueue_all_files();
    
    //Dequeue every file we have and process it
    while(queue_size(all_files) > 0)
    {
    	int file_size, priority;
    	char *filename = dequeue(all_files, &file_size, &priority);
	    process(filename);
    }
    
    free_queue(all_files);	//Free the file queue
    
    //Get the total time this ran for
    clock_t now = clock();
	int diff = (int) (now - start);
	float seconds = (float) diff / CLOCKS_PER_SEC;
	printf("TOTAL RUNTIME: %f seconds!\n", seconds);
	
    return 0;
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
        	
		    enqueue(all_files, filename, file_size, 1);	//Enqueue the file
		}
    }
    
    closedir(file_dir);	//Close the work directory stream
    return all_files->size;	//Return the number of files we read
}

int file_name_valid(char *filename)
{
	//If the filename is a directory, it's definitely not valid
	if(filename[0] == '.')
		return 0;
	
	return !strcmp((filename + strlen(filename) - 4), ".sen");	//Check if filename ends with ".sen"
}

void process(char *filename)
{
	//Open the file for reading
	FILE *file = fopen(filename, "r");
    char line[LINE_NUM_CHARS];
	
	//Keep going until we reach the end of the file
    while(!feof(file))
    {
    	//Read a line
    	memset(line, 0, LINE_NUM_CHARS);
    	fgets(line, LINE_NUM_CHARS, file);
    	int len = strlen(line);
        line[len - 2] = '\0';	//Cut off the newline at the end
	
		kv_pair_t kv_pair = get_kv_pair(line);	//Get a key/value pair from it
		
		//If the key matches the key we want...
		if(!strcmp(kv_pair.key, search_key))
		{
			printf("\nFound value from %s! Original: %s, Key=%s, Value=%s\n", filename, line, kv_pair.key, kv_pair.value);
			move_file(filename);	//Archive
			burn_cycles(500);	//Instead of doing actual database stuff, just burn 500 cycles to simulate writing
			
			//And get us out of here, because we've finished our job
			free(kv_pair.key);
			free(kv_pair.value);
			break;
		}
			
		free(kv_pair.key);
		free(kv_pair.value);
    }
    
    printf("\n");
	fclose(file);	//Close the file
}

kv_pair_t get_kv_pair(char *line)
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

void burn_cycles(int num_cycles)
{
	volatile int i;	//Volatile means GCC won't optimize this function away
	printf("Inserting into database!");
	
	for(i = 0; i < num_cycles; i++);	//Just waste iterations
}

