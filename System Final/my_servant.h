#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <errno.h>
#include <arpa/inet.h>


/*
	Start index of the servant.
	Defines the start position of servant in city list.
*/
int start_index;

/*
	End index of the servant.
	Defines the end position of servant in city list.
*/
int end_index;

/*
	2D char array to keep cities in dataset directory.
*/
char** directories;

/*
	Keeps the number of cities in dataset.
*/
int dir_size;

/*
	IP address of the server.
*/
char ip_name[50];

/*
	Port number that server listens.
*/
int port_num;

/*
	Number of cities that servant is responsible.
*/
int responsible_dirs=0;

/*
	Keeps the number of requests that servant handled.
*/
int handled = 0;

/*
	PID of the servant.
*/
int pid;

/*
	Protects the request assignment to created thread.
*/
pthread_mutex_t main_mutex=PTHREAD_MUTEX_INITIALIZER;

/*
	Protects the request read from socket.
*/
pthread_mutex_t servant_mutex=PTHREAD_MUTEX_INITIALIZER;


/*
	Flag to determine SIGINT arrival.
*/
int sigintflag = 0;

/*
	record object that saves the informations of the record lines.
	city : City of the record.
	type : Type of the record. TARLA, DUKKAN... etc.
	id : ID of the record.
	province : Street of the record.
	field_size : Size of the field in meter square type.
	day : Day of record
	month : Month of record
	year : Year of record 
*/
typedef struct record{
	char city[256];
	char type[256];
	int id;
	char province[256];
	int field_size;
	int price;
	int day;
	int month;
	int year;
} record;


/*
	city : City name in records.
	index : Index of the city starts in records data structure.
*/
typedef struct map{
	char city[256];
	int index;
} map;

/*
	Map's record holder part.
*/
record* recordsList;

/*
	List to keep map objects.
	Holds the index of each city starts in data structure.
*/
map mapList[256];

/*
	Keeps the number of map objects recorded in map list.
*/
int mapCount = 0;

/*
	Number of records in map.
*/
int recordCount = 0;

/*
	Variable to increase when adding new record to map.
*/
int recIndex = 0;


/*
	Initializes the map item.
	Kind of a constructor for map objects.

	_param_ : item -> map item to set fields
	_return_ : None
*/
void init_map_item(map item);

/*
	Initializes the record item.
	Kind of a constructor for record objects.

	_param_ : item -> record item to set fields
	_return_ : None
*/
void init_Record(record item);

/*
	Function to check if the requested city exists in read records from dataset.

	_param_ : citi -> name of the requested city
	_return_ : 1 if city found else -1
*/
int is_city_exist(char* citi);

/*
	Prints the records read from dataset.

	_param_ : None
	_return_ : None
*/
void printRecords();

/*
	Finds the PID of the process from /proc and returns.

	_param_ : None
	_return_ : PID of the process.	
*/
pid_t pid_from_proc();

/*
	This function takes the path of record file.
	Reads file and returns the number of lines.
	
	_param_ : filename -> Request file path
	_return_ : number of lines.
*/
int get_line_count(char filename[1024]);

/*
	Allocating space for city directories' names.

	_param_ : size -> Number of cities in dataset. Determines the size of directories array.
	_return_ : None
*/
void allocate_space(int size);

/*
	Frees allocated space for directories array.

	_param_ : None
	_return_ : None
*/
void free_allocated();

/*
	Reads the record file line by line.
	Creates record object for each line.
	Saves record objects in map.

	_param_ : 	filename -> path of the record file
				date -> Date of the record file
				city -> City of the records belong
	_return_ : None
*/
void store_Records(char* filename, char* date, char* city);

/*
	Reading record file names in a city directory.

	_param_ : dirPath -> Path of city directory
	_return_ : None	
*/
void read_Records(char* dirPath);

/*
	Reading city names(directories) in dataset.

	_param_ : dirPath -> Path of dataset directory.
	_return_ : None	
*/
void read_directories(char* dirPath);

/*
	Extracting start and end index of the servant responsibility from command line argument.

	_param_ : rangestr -> Range of the servant. (1-9, 10-19... etc.)
	_return_ : None		
*/
void find_range(char* rangestr);

/*
	Passing found record number to server.

	_param_ : infoline -> Found record number.
	_return_ : None			
*/
void* passInfoServer(char* infoline);

/*
	Splitting the date string to day month and year.

	_param_ : 	date -> Date in string type.
				d -> integer pointer for day number
				m -> integer pointer for month number
				y -> integer pointer for year number
	_return_ : None			
*/
void splitDate(char* date, int* d, int* m, int* y);

/*
	Function of the threads in servant process. Checks the request in recorded data.

	_param_ : requ -> request socket fd taken as parameter
	_return_ : None	
*/
void* getResults(void* requ);

/*
	Function to free memories allocated before exiting because of SIGINT

	_param_ : None
	_return_ : None	
*/
void signalTerminate();

/*
	Signal handler function.

	_param_ : signal_number -> type of the signal arrived.
	_return_ : None		
*/
void handler(int signal_number);
