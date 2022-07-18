#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <semaphore.h>
#include <time.h>
#include "fifo_seqnum.h"


static char clientFifo[CLIENT_FIFO_NAME_LEN];

static void removeFifo(){
	unlink(clientFifo);
}

int readFile(int fd) {
    char buffer[500];
    int k = 0;
    char temp = '-';
    do {
        
        read(fd, &temp, 1); 
        buffer[k++] = temp;    
        if(temp == '\n') {
            buffer[k-1] = '\0';
            //break;
        }
    }
    while (temp != '\n'); 

    //printf("%s\n", buffer);
    int i;
    int com_count = 0;
    for(i=0;i<strlen(buffer);i++){
        if(buffer[i] == ',')
            com_count++;
    }
    //printf("%d\n", com_count+1);
    return com_count+1;
}


int main(int argc, char **argv)
{
	char *fifo_path = NULL;
	char *data_path = NULL;
	struct request req;
	struct response resp;
	int serverFd, clientFd;
	int index;
	int c;

	opterr = 0;
	while ((c = getopt (argc, argv,"s:o:")) != -1)
    switch (c)
      {
      case 's':
        fifo_path = optarg;
        break;
      case 'o':
        data_path = optarg;
        break;
      case '?':
        if (optopt == 'c')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }

    for (index = optind; index < argc; index++)
    	fprintf (stderr,"Non-option argument %s\n", argv[index]);

    int fd = open(data_path, O_RDONLY, 0644);
    int mat_size = readFile(fd);
    close(fd);

    time_t ltime;
    struct tm * timeinfo;
    time(&ltime);
    timeinfo = localtime ( &ltime );

    fprintf(stderr,"%s Client PID#%d (%s) is submitting a %dx%d matrix\n", asctime(timeinfo), getpid(), data_path, mat_size, mat_size);

    umask(0);

    

    snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) getpid());
    if(mkfifo(clientFifo, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST)
    	fprintf(stderr,"mkfifo %s ", clientFifo);
    if(atexit(removeFifo) != 0)
    	fprintf(stderr,"atexit");

    req.pid = getpid();
    strcpy(req.reqtxt, data_path);
    req.seqLen = 4000;

    serverFd = open(fifo_path, O_WRONLY);
    if(serverFd == -1)
    	fprintf(stderr,"open %s", fifo_path);
    if(write(serverFd, &req, sizeof(struct request)) != sizeof(struct request))
    	fprintf(stderr,"Can't write to server");

    time_t begin = time(NULL);
    clientFd = open(clientFifo, O_RDONLY);
    if(clientFd == -1)
    	fprintf(stderr,"open %s", clientFifo);
    if(read(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
    	fprintf(stderr,"Can't read response from server");

    time_t end = time(NULL);
    time(&ltime);
    timeinfo = localtime ( &ltime );
    if(strcmp(resp.yes_or_no, "yes") == 0)
      fprintf(stderr,"%s CLient PID#%d: the matrix is invertible, total time %ld seconds, goodbye.\n", asctime(timeinfo),getpid(), (end-begin));
    else{
      fprintf(stderr,"%s CLient PID#%d: the matrix is NOT invertible, total time %ld seconds, goodbye.\n", asctime(timeinfo),getpid(), (end-begin));
    }
	return 0;
}





