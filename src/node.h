#ifndef NODE_H_INCLUDED
#define NODE_H_INCLUDED

#include "container.h"

/* Node variables */
extern file_queue_t *file_queue;	//This node's file queue
extern pthread_t process_thread;	//This node's thread to run process() independently of enqueueing

/* Node functions */

/*
 * Initialize us as a node.
 * Params: nothing
 * Returns: nothing
 */
void init_node();

/*
 * Process a file. Search for a specific key, and if the file contains a
 * key/value pair with that key, insert it into a database and archive it.
 * Params: filename - full path to the file that should be processed
 * Returns: nothing
 */
void process(char *filename);

/*
 * Finalize us as a node.
 */
void node_cleanup();

#endif //NODE_H_INCLUDED

