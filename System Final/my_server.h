#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>

/*
	Thread job object that holds the socket fd of the requester client thread.
*/
struct jobQueue{				
	int socketFd;
};


/*
	Servant object. Holds informations about servants.
	pid : PID of the servant process.
	port : Port number that given servant process listens.
	start : Starting city which the servant process is responsible for.
	end : Ending city which the servant process is responsible for.
*/
struct Servant{
	int pid;
	int port;
	char start[256];
	char end[256];
};

/*
	Holds the number of servants created.
	Incremented with each record of servant. 
*/
int servantIndex = 0;

/*
	List of servants.
	Servant objects are saved in here.
*/
struct Servant servantsArr[256];

/*
	Queues of thread jobs.
	Requests from clients are kept in here.
*/
struct jobQueue tasksArr[256];

/*
	Thread pool that holds the threads in it.
	Allocated later according to number of threads given from command line.
*/
pthread_t *threadPool;

/*
	Number of requests in queue is kept in here.
*/
int reqCounter = 0;

/*
	Number of currently working threads are hold in here.
*/
int instantWorkerCount=0;

/*
	Mutex to protect request queue from race condition between threads in the pool.
*/
pthread_mutex_t Listmutex;

/*
	Condition variable to make threads wait until a request arrives.
*/
pthread_cond_t Listcond;

/*
	Protecting request pushing to queue operation. 
*/
pthread_mutex_t main_mutex=PTHREAD_MUTEX_INITIALIZER;	

/*
	Servant mutex protects the servant list from race condition of threads 
	while threads add servant objects to servant list.
*/
pthread_mutex_t servant_mutex=PTHREAD_MUTEX_INITIALIZER;

/*
	Synchronizes threads while sending requests.
*/
pthread_mutex_t ask_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
	Flag to detect SIGINT arrived or not.
*/	
static volatile sig_atomic_t gotSIGINTSig = 0;

/*
	Keeps the number of handled requests.
*/		
int handled = 0;

/*
	Port number that server listens.
*/
int port_num;

/*
	Number of threads in the pool.
*/
int thread_num;

/*
	Adds new request to thred job queue

	_param_ : req -> Request object contains request's socket fd
	_return_ : None 
*/
void ListPush(struct jobQueue req);

/*
	Comparing strings to determine if the requested city is between responsible cities of a servant.

	_param_ : 	city1 -> Servant's starting city
				city2 -> Servant's ending city
				city3 -> City in request query
	_return_ : index of servant object in servant list. -1 if any servant cares about the city. 
*/
int compareStrings(char* city1, char* city2, char* city3);

/*
	Frees allocated memories before exiting.

	_param_ : None
	_return_ : None 
*/
void exit_gracefully();

/*
	Sending the request query to servant.

	_param_ : 	pid -> PID of the servant
				port -> port number that servant listens
				query -> query to send servant
	_return_ : found records in dataset for query
*/
int askServant(int pid, int port, char* query);

/*
	Function that thread pool threads run.
	Parses queries sends to servants and returns results to client thread.

	_param_ : tnum -> Thread number
	_return_ : None
*/
void* clientReqHandler(void* tnum);

/*
	Sending SIGINT to servant processes.

	_param_ : None
	_return_ : None 
*/
void sigTerminate();


/*
	Signal handler.

	_param_ : None
	_return_ : None 
*/
void handler(int signal_number);
