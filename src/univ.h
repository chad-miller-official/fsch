#ifndef UNIV_H_INCLUDED
#define UNIV_H_INCLUDED

//This is basically a header for main.c

#define CENTRAL 0
#define FILE_NAME_LEN 80
#define LINE_NUM_CHARS 80

/* Scheduling algorithms */
enum {
	CYCLIC,
	BLOCK,
	RANDOM,
	QUEUE_SIZE,
	QUEUE_LENGTH
};

/* Priority options */
enum {
	NO_PRIORITY,
	OLDEST_FILE_PRIORITY
};

/* MPI tags */
enum {
	FILE_NAME_TAG,
	FILE_SIZE_TAG,
	FILE_PRIORITY_TAG,
	STOP_TAG,
	ARCHIVE_TAG,
	QUEUE_DATA_TAG
};

/* Represents a key/value pair */
typedef struct _kv_pair_t {
	char *key;
	char *value;
} kv_pair_t;

/* Variables universal to all processors */
extern char *file_dir_str;		//File directory
extern char *archive_dir_str;	//Archive directory
extern char *search_key;		//Key to look for in each file

extern int proc_count;			//Number of nodes (including central node)
extern int proc_id;				//Processor ID of this specific instance
extern int sched_type;			//Scheduling algorithm to use
extern int priority_option;		//Prority option to use

#endif //UNIV_H_INCLUDED

