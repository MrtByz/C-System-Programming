#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*
	2D char array saves the requests from request file.
*/
char **read_file_array;

/*
	Holds the ip address of server taken from command line as an argument
*/
char ip_name[1024];

/*
	Port number that server listens taken from command line as an argument
*/
int port_num;

/*
	Number of lines in request file.
*/
int lines;

/*
	Variable for waiting threads in the matter of synchronization barrier problem. 
	Each thread which reaches to randezvous point increases this variable.
*/
int arrived = 0;

/*
	Condition variable that used for the solution of synchronization barrier problem.
	Creates a monitor with the mutex below.
*/
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

/*
	Mutex that used for the solution of synchronization barrier problem.
	Creates a monitor with the condition varible above.
*/
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


/*
	This function allocates enough space to read_file_array.
	read_file_array is a 2d char array and holds the requests inside.
	
	_param_ : None
	_return_ : None
*/
void allocate_space();

/*
	This function takes the path of request file.
	Reads file and assigns the number of lines to "lines" variable.
	
	_param_ : filename -> Request file path
	_return_ : None
*/
void get_line_count(char filename[1024]);

/*
	Frees the allocated memories that allocated for read_file_array.
	_param_ : None
	_return_ : None
*/
void free_allocated();

/*
	Prints the lines that stored in read_file_array.
	Implemented for debug purposes.

	_param_ : None
	_return_ : None
*/
void print_lines();

/*
	This function reads request file content to read_file_array.

	_param_ : filename -> Request file path
	_return_ : None
*/
void read_file(char filename[1024]);

/*
	This function is the function used by created threads.
	Connects to server via socket and sends the request to server.

	_param_ : line -> Number of the request index that thread is responsible for.
	_return_ : None
*/
void* requester_thread(void* line);


