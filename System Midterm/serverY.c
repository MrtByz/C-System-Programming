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
#include <sys/mman.h>
#include <sys/shm.h>
#include "fifo_seqnum.h"
#include "become_daemon.h"
#include <time.h>

sig_atomic_t child_exit_status;
sig_atomic_t SIGINT_FLAG = 0;

#define SHM_NAME "/shmqueue"
#define MAX_SIZE (50)
#define PERM (0666)

typedef struct QNode QNode;
typedef struct Queue Queue;

#define REQ_SIZE 200
#define QUEUE_SIZE 64
#define QNULL (QUEUE_SIZE + 1)

struct QNode {
    char request[500];
    size_t next;
};

struct Queue {
    QNode nodes[REQ_SIZE];
    size_t used;
    size_t empty;
    size_t head;                
};


void serverY(char* fifo_path, char* log_path, int pool_y, int pool_z, int sleep_time);
void serverZ(char* log_path, int pool_z, int sleep_time);
void workerY(int t, char *log, int index);
int is_Invertible(int size, char * test_path);
void parse_coordinates(char **lines, int size, int **result);
ssize_t readline (char *buf, size_t sz, char *fn, off_t *offset);
int findInvertibility(int **mat, int n, int size);
void findcofact(int **mat, int **temp, int p, int q, int n, int size);
void create_sem(int index);
QNode *create_node();
void delete_node(QNode *node);
QNode *qnode(size_t index);
QNode *next_node(const QNode *node);
QNode *add_node(size_t *head, const char *str);
void remove_node(size_t *head);
void workerZ(int t, char *log, int index);
void create_semZ(int index);

int *Z_pids;
int *Y_pids;
struct Queue *queue;
int **pipe_to_pool_y;
int pipe_to_Z[2];
int pool_Z;
int pool_Y;
char logum[200];
char serv_fifo[200];
int forward_count;

void SIGEXIT_Y(){
    int i;
    char pid[1000];
    for(i=0;i<pool_Y;i++){
      char sem_name[10];
      sprintf(sem_name, "sem_%d", i);
      sem_t *sem_prod1 = sem_open(sem_name,0);
      sem_close(sem_prod1);
      sem_unlink(sem_name);
      close(pipe_to_pool_y[i][0]);
      close(pipe_to_pool_y[i][1]);
      kill(Y_pids[i], SIGUSR1);
    }

    int Z_pid;
    int Zpid ;
    Z_pid = shm_open("Z_pid", O_CREAT | O_RDWR, 0666);
    ftruncate(Z_pid, sizeof(int));
    int* Z_ptr = (int*)mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, Z_pid, 0);
    Zpid= Z_ptr[0];
    kill(Zpid, SIGUSR1);
    shm_unlink("Z_pid"); 
    free(Y_pids);
    time_t ltime;
    struct tm * timeinfo;
    time(&ltime);
    timeinfo = localtime ( &ltime );
    int fd2 = open(logum, O_WRONLY|O_APPEND, 0644);
    int I_shm;
    I_shm = shm_open("I_shrd", O_CREAT | O_RDWR, 0666);
    ftruncate(I_shm, 2*sizeof(int));
    int* I_ptr = (int*)mmap(0, 2*sizeof(int), PROT_WRITE, MAP_SHARED, I_shm, 0);
    sprintf(pid, "%s SIGINT received exiting server Y. Total requests: %d, %d invertible, %d not. %d forwarded to Z\n", asctime(timeinfo), I_ptr[0]+I_ptr[1], I_ptr[1], I_ptr[0], forward_count);
    close(I_shm);
    shm_unlink("I_shrd");
    write(fd2, pid, strlen(pid));
    close(fd2);
    unlink(serv_fifo);
    sem_unlink("sem_to_z");
    sem_unlink("sem_to_y");
    sem_unlink("sem_of_y");
    exit(0);
}

void SIGEXIT_Z(){
    int i;
    char pid[1000];
    for(i=0;i<pool_Z;i++){
      char sem_name[10];
      sprintf(sem_name, "semZ_%d", i);
      sem_t *sem_prod1 = sem_open(sem_name,0);
      sem_close(sem_prod1);
      sem_unlink(sem_name);
      kill(Z_pids[i], SIGUSR1);
      free(Z_pids);
    }
    shm_unlink("/shmqueue");
    time_t ltime;
    struct tm * timeinfo;
    time(&ltime);
    timeinfo = localtime ( &ltime );
    int fd2 = open(logum, O_WRONLY|O_APPEND, 0644);
    sprintf(pid, "%s SIGINT received exiting server Z.\n", asctime(timeinfo));
    write(fd2, pid, strlen(pid));
    close(fd2);
    exit(0);
}

void handler(int signal_number){
  
  if(signal_number == SIGINT){
    SIGEXIT_Y();
  }
  else if(signal_number == SIGUSR1){
    SIGINT_FLAG = 1;
    SIGEXIT_Z();
  }
}



void handlerZworker(int signal_number){
  if(signal_number == SIGINT){
    SIGINT_FLAG = 1;
  }
  else if(signal_number == SIGUSR1){
    exit(0);
  }
}


int findSize(int fd) {
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
  char *log_path = NULL;
  int pool_y;
  int pool_z;
  int sleep_time;
	int index;
	int c;

	opterr = 0;
	while ((c = getopt (argc, argv,"s:o:p:r:t:")) != -1)
    switch (c)
      {
      case 's':
        fifo_path = optarg;
        break;
      case 'o':
        log_path = optarg;
        break;
      case 'p':
        pool_y = atoi(optarg);
        break;
      case 'r':
        pool_z = atoi(optarg);
        break;
      case 't':
        sleep_time = atoi(optarg);
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

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handler;
    sigaction(SIGINT, &sa, NULL);

    if(pipe(pipe_to_Z) == -1)
      perror("Pipe to Z couldnt open");

    sem_t *sem_prod = sem_open("sem_to_z", O_CREAT, 0660,0);
    sem_t *sem_prodY = sem_open("sem_to_y", O_CREAT, 0660,0);

    int Z_pid;
    Z_pid = shm_open("Z_pid", O_CREAT | O_RDWR, 0666);
    ftruncate(Z_pid, sizeof(int));
    int* Z_ptr = (int*)mmap(0, (pool_z+1)*sizeof(int), PROT_WRITE, MAP_SHARED, Z_pid, 0);
    close(Z_pid);

    if(sem_prod == -1){
      perror("sem failed");
      exit(0);
    }
    if(sem_prodY == -1){
      perror("sem failed");
      exit(0);
    }
    int pidim = fork();
    if(pidim == -1){
      return -1;
    }
    else if(pidim == 0){
      serverZ(log_path, pool_z, sleep_time);
    }
    else{
      serverY(fifo_path, log_path, pool_y, pool_z, sleep_time);
    }
}

void serverY(char* fifo_path, char* log_path, int pool_y, int pool_z, int sleep_time){
  int serverFd, dummyFd;
  struct request req;
  int i;
  forward_count = 0;
  //sem_t *sem_server = sem_open("sem_server", O_CREAT, 0660,1);
  pipe_to_pool_y = (int**)malloc(pool_y * sizeof(int*));
  for (i = 0; i < pool_y; i++)
    pipe_to_pool_y[i] = (int*)malloc(2 * sizeof(int));

  for(i=0;i<pool_y;i++){
    if(pipe(pipe_to_pool_y[i]) == -1)
      perror("Pipe problem");
  }

  int I_shm;
  I_shm = shm_open("I_shrd", O_CREAT | O_RDWR, 0666);
  ftruncate(I_shm, 2*sizeof(int));
  int* I_ptr = (int*)mmap(0, 2*sizeof(int), PROT_WRITE, MAP_SHARED, I_shm, 0);
  I_ptr[0] = 0;
  I_ptr[1] = 0;
  close(I_shm);

  Y_pids = (int*)malloc(pool_y * sizeof(int));
  pool_Y = pool_y;
  int ch_pid;
  for(i=0;i<pool_y;i++){
      switch(ch_pid =fork()){
        case -1: fprintf(stderr, "Couldn't fork\n");
        case 0: workerY(sleep_time, log_path,i);
        default: Y_pids[i] = ch_pid; create_sem(i);
      }
    }

  for(i=0;i<pool_y;i++){
    if(close(pipe_to_pool_y[i][0]) == -1)
      perror("Read end can not closed");
  }
  if(close(pipe_to_Z[0]) == -1)
    perror("Pipe to Z Read end can not closed");
  becomeDaemon(0);
  umask(0);
  strcpy(logum, log_path);
  strcpy(serv_fifo, fifo_path);
  char pid[500];
  time_t ltime;
  struct tm * timeinfo;
  time(&ltime);
  timeinfo = localtime ( &ltime );
  int fd = open(log_path, O_CREAT|O_WRONLY|O_APPEND, 0644);
  sprintf(pid, "%s Server Y (%s %s p=%d t=%d) started (%d)\n", asctime(timeinfo), fifo_path,log_path, pool_y,sleep_time, getpid());
  write(fd, pid, strlen(pid));
  sprintf(pid, "%s Instantiated server Z\n", asctime(timeinfo));
  write(fd, pid, strlen(pid));
  close(fd);

  if(mkfifo(fifo_path, S_IRUSR | S_IWUSR | S_IWGRP) == -1 && errno != EEXIST)
    fprintf(stderr,"mkfifo %s ", fifo_path);
  
  serverFd = open(fifo_path, O_RDONLY);
  if(serverFd == -1)
    fprintf(stderr,"open %s", fifo_path);

  dummyFd = open(fifo_path, O_WRONLY);

  if(dummyFd == -1)
    fprintf(stderr,"open %s", fifo_path);



  char sem_name[20];
  sprintf(sem_name, "sem_%d", getpid());
  sem_t *sem_of_y = sem_open(sem_name, O_CREAT, 0660,1);
  if(&sem_of_y == -1){
    perror("sem failed");
    exit(0);
  }
  //sem_unlink("sem_of_y");

  for(;;){
    sem_wait(sem_of_y);
    if(SIGINT_FLAG == 1){
      for(i=0;i<pool_y;i++){
        kill(Y_pids[i], SIGUSR1);
        char sem_name[10];
        sprintf(sem_name, "sem_%d", i);
        //printf("SEM NAME: %s\n", sem_name);
        sem_unlink(sem_name);
      }
      int Z_pid;
      int Zpid ;
      Z_pid = shm_open("Z_pid", O_CREAT | O_RDWR, 0666);
      ftruncate(Z_pid, sizeof(int));
      int* Z_ptr = (int*)mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, Z_pid, 0);
      Zpid= Z_ptr[0];
      kill(Zpid, SIGUSR1);        
      int fd_Z = open(log_path, O_WRONLY|O_APPEND, 0644);
      sprintf(pid, "ZPID: %d\n", Zpid);
      write(fd_Z, pid, strlen(pid));
      close(fd_Z);
      close(Z_pid);
      unlink(fifo_path);
      time(&ltime);
      timeinfo = localtime ( &ltime );
      int fd2 = open(log_path, O_WRONLY|O_APPEND, 0644);
      int I_shm;
      I_shm = shm_open("I_shrd", O_CREAT | O_RDWR, 0666);
      ftruncate(I_shm, 2*sizeof(int));
      int* I_ptr = (int*)mmap(0, 2*sizeof(int), PROT_WRITE, MAP_SHARED, I_shm, 0);
      sprintf(pid, "%s SIGINT received exiting server Y. Total requests: %d, %d invertible, %d not.\n", asctime(timeinfo), I_ptr[0]+I_ptr[1], I_ptr[1], I_ptr[0]);
      close(I_shm);
      shm_unlink("I_shrd");
      write(fd2, pid, strlen(pid));
      close(fd2);
      exit(0);
    }
    
    if(read(serverFd, &req, sizeof(struct request)) != sizeof(struct request)){
      fprintf(stderr, "Error reading request; discarding\n");
      continue;
    }
    else{
      char str_to_pipe[700];
      sprintf(str_to_pipe, "%s&PID%d\n", req.reqtxt, req.pid);
      /*int fd2 = open(log_path, O_WRONLY|O_APPEND, 0644);
      sprintf(pid, str_to_pipe, strlen(str_to_pipe));
      write(fd2, pid, strlen(pid));
      close(fd2);*/
      str_to_pipe[strlen(str_to_pipe)+1] = '\0';
      int busy_child_count = 0;
      for(i=0;i<pool_y;i++){
        char sem_name[10];
        int sval;
        sprintf(sem_name, "sem_%d", i);
        sem_t *sem_prod1 = sem_open(sem_name,0);
        sem_getvalue(sem_prod1, &sval);
        if(sval > 0){
          /*write(pipe_to_pool_y[i][1],str_to_pipe,strlen(str_to_pipe)+1);
          sem_post(sem_prod1);
          break;*/
          busy_child_count++;
        }
      }
      if(busy_child_count == pool_y){
        if(write(pipe_to_Z[1], str_to_pipe, strlen(str_to_pipe)+1) == -1){
        }
        forward_count++;
        sem_t *sem_prod_to_z = sem_open("sem_to_z",0);
        sem_t *sem_prod_to_y = sem_open("sem_to_y",0);
        sem_post(sem_prod_to_z);
        sem_wait(sem_prod_to_y);
      }
      else{
        for(i=0;i<pool_y;i++){
          char sem_name[10];
          int sval;
          sprintf(sem_name, "sem_%d", i);
          sem_t *sem_prod1 = sem_open(sem_name,0);
          sem_getvalue(sem_prod1, &sval);
          if(sval <= 0){
            write(pipe_to_pool_y[i][1],str_to_pipe,strlen(str_to_pipe)+1);
            sem_post(sem_prod1);
            break;
          }
        }
      }
    }
    sem_post(sem_of_y);
  }

  for(i=0;i<pool_y;i++){
    char sem_name[10];
    sprintf(sem_name, "sem_%d", i);
    sem_t *sem_prod1 = sem_open(sem_name,0);
    sem_close(sem_prod1);
    sem_unlink(sem_name);
  }
}

void serverZ(char* log_path, int pool_z, int sleep_time){
  becomeDaemon(0);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &handler;
  sigaction(SIGUSR1, &sa, NULL);

  char req_str[850];
  pool_Z = pool_z;

  
  int req_count=0;
  int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT,PERM);

  ftruncate(shmfd, sizeof *queue);
  queue = mmap(NULL, sizeof *queue,
  PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  if (queue == MAP_FAILED) {
      (void) fprintf(stderr,"MAP FAILED");
      exit(1);
  } /* error */
  close(shmfd);

  Z_pids = (int*)malloc(pool_z * sizeof(int));

  /*int I_shm;
  I_shm = shm_open("I_shrd2", O_CREAT | O_RDWR, 0666);
  ftruncate(I_shm, 2*sizeof(int));
  int* I_ptr = (int*)mmap(0, 2*sizeof(int), PROT_WRITE, MAP_SHARED, I_shm, 0);
  I_ptr[0] = 0;
  I_ptr[1] = 0;
  close(I_shm);*/

  queue->head = QNULL;
  queue->empty = QNULL;
  queue->used = 0;
  int i;
  int ch_pid;
  for(i=0;i<pool_z;i++){
      switch(ch_pid = fork()){
          case -1: exit(0);
          case 0: workerZ(sleep_time, log_path, i);
          default: Z_pids[i] = ch_pid; create_semZ(i); break;
      }
  }

  int Z_pid;
  Z_pid = shm_open("Z_pid", O_CREAT | O_RDWR, 0666);
  ftruncate(Z_pid, sizeof(int));
  int* Z_ptr = (int*)mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, Z_pid, 0);
  Z_ptr[0] = getpid();
  close(Z_pid);
  strcpy(logum, log_path);

  if(close(pipe_to_Z[1]) == -1)
    perror("Pipe to Z Write end can not closed");
  sem_t *sem_prod = sem_open("sem_to_z",0);
  sem_t *sem_prodY = sem_open("sem_to_y",0);
  while(1){
    if(SIGINT_FLAG == 1){
      for(i=0;i<pool_z;i++){
        kill(Z_pids[i], SIGUSR1);
      }
      exit(0);
    }
    if(req_count != 0){
      for(i=0;i<pool_z;i++){
        char sem_name[10];
        int sval;
        sprintf(sem_name, "semZ_%d", i);
        sem_t *sem_prod1 = sem_open(sem_name,0);
        sem_getvalue(sem_prod1, &sval);
        if(sval <= 0){
          req_count--;
          sem_post(sem_prod1);
          break;
        }
      }
    }
    else{
      sem_wait(sem_prod);
      int read_check;
      read_check = read(pipe_to_Z[0], req_str, 701);
      if (read_check == -1) {
        perror("read");
        continue;
      }
      sem_post(sem_prodY);
      add_node(&queue->head, req_str);
      req_count++;
    }
  }
}

void workerZ(int t, char *log, int index){

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handlerZworker;
    sigaction(SIGUSR1, &sa, NULL);
    char pid[500];
    struct response resp;
    char clientFifo[CLIENT_FIFO_NAME_LEN];
    char sem_name[10];
    sprintf(sem_name, "semZ_%d", index);

    sem_t *sem_prod = sem_open(sem_name,0);
    while(1){
        char test_path[500];
        char pid_to_int[20];
        if(SIGINT_FLAG == 1){
          exit(0);
        }

        sem_wait(sem_prod);
        
        if(queue->head != QNULL){
          char *req_str = qnode(queue->head)->request;
          int i=0,j=0;
          
          while(req_str[i] != '&'){
            test_path[i] = req_str[i];
            i++;
          }
          test_path[i] = '\0';
          i+=4;
          while(req_str[i] != '\n'){
            pid_to_int[j] = req_str[i];
            j++;
            i++;
          }
          pid_to_int[i] = '\0';
          //char* test_path = qnode(queue->head)->request;
          int pid_of_cli = atoi(pid_to_int);
          remove_node(&queue->head);
          int fd = open(test_path, O_RDONLY, 0644);
          int mat_size = findSize(fd);
          close(fd);
          //printf("%s - %d - %dx%d\n", test_path, getpid(), mat_size,mat_size);
          fd = open(log, O_WRONLY|O_APPEND, 0644);
          char text2[500];
          time_t ltime;
          struct tm * timeinfo;
          time(&ltime);
          timeinfo = localtime ( &ltime );
          sprintf(text2, "%s Forwarding request of client PID#%d to serverZ, matrix size %dx%d\n", asctime(timeinfo), pid_of_cli, mat_size, mat_size);
          write(fd, text2, strlen(text2));
          close(fd);
          int inv_or_not = is_Invertible(mat_size, test_path);
          time(&ltime);
          timeinfo = localtime ( &ltime );
          sprintf(pid, "%s Z: Worker PID#%d is handling client PID#%d matrix size %dx%d\n", asctime(timeinfo), getpid(), pid_of_cli, mat_size, mat_size);
          int fd2 = open(log, O_WRONLY|O_APPEND, 0644);
          write(fd2, pid, strlen(pid));
          close(fd2);

          snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) pid_of_cli);

          int fd3 = open(log, O_WRONLY|O_APPEND, 0644);
          if(inv_or_not == 0){
            strcpy(resp.yes_or_no, "no");
            resp.yes_or_no[2] = '\0';
            time(&ltime);
            timeinfo = localtime ( &ltime );
            sprintf(pid, "%s Z: Worker PID#%d is responding to client PID#%ld: the matrix is NOT invertible\n", asctime(timeinfo), getpid(), (long)pid_of_cli);
            write(fd3, pid, strlen(pid));
            /*int shm_fd;
            shm_fd = shm_open("I_shrd2", O_RDWR, 0666);
            int* ptr = (int*)mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);
            ptr[0]++;
            close(shm_fd);*/
          }
          else{
            strcpy(resp.yes_or_no, "yes");
            resp.yes_or_no[3] = '\0';
            time(&ltime);
            timeinfo = localtime ( &ltime );
            sprintf(pid, "%s Z: Worker PID#%d is responding to client PID#%ld: the matrix is invertible\n", asctime(timeinfo), getpid(), (long)pid_of_cli);
            write(fd3, pid, strlen(pid));
            /*int shm_fd;
            shm_fd = shm_open("I_shrd2", O_RDWR, 0666);
            int* ptr = (int*)mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);
            ptr[1]++;
            close(shm_fd);*/
          }
          sleep(t);
          close(fd3);
          int clientFd = open(clientFifo, O_WRONLY);
          if(clientFd == -1){
            fprintf(stderr,"open %s", clientFifo);
          }
          if(write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
            fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
          if(close(clientFd) == -1)
            fprintf(stderr,"close"); 
        }
    }
    exit(0);
}

void workerY(int t, char *log, int index){
  struct response resp;
  char clientFifo[CLIENT_FIFO_NAME_LEN];

  char pid[1000];
  char req_str[850];

  if(close(pipe_to_pool_y[index][1]) == -1)
    perror("Write end can not closed");

  char sem_name[10];

  sprintf(sem_name, "sem_%d", index);
  sem_t *sem_prod = sem_open(sem_name,0);

  if(&sem_prod == -1){
    perror("sem failed");
    exit(0);
  }


  while(1){
    sem_wait(sem_prod);
    
    int read_check;

    read_check = read(pipe_to_pool_y[index][0], req_str, sizeof(req_str));

    if (read_check == -1) {
      perror("read");
      exit(0);
    }
    
    int i=0,j=0;
    char test_path[500];
    char pid_to_int[20];
    while(req_str[i] != '&'){
      test_path[i] = req_str[i];
      i++;
    }
    test_path[i] = '\0';
    i+=4;
    while(req_str[i] != '\n'){
      pid_to_int[j] = req_str[i];
      j++;
      i++;
    }
    pid_to_int[j] = '\0';
    
    int fd = open(test_path, O_RDONLY, 0644);
    int mat_size = findSize(fd);
    close(fd);
    int inv_or_not = is_Invertible(mat_size, test_path);
    time_t ltime;
    struct tm * timeinfo;
    time(&ltime);
    timeinfo = localtime ( &ltime );
    sprintf(pid, "%s Worker PID#%d is handling client PID#%s matrix size %dx%d\n", asctime(timeinfo), getpid(), pid_to_int, mat_size, mat_size);
    int fd2 = open(log, O_WRONLY|O_APPEND, 0644);
    write(fd2, pid, strlen(pid));
    close(fd2);

    int pid_of_cli = atoi(pid_to_int);
    snprintf(clientFifo, CLIENT_FIFO_NAME_LEN, CLIENT_FIFO_TEMPLATE, (long) pid_of_cli);

    
    int fd3 = open(log, O_WRONLY|O_APPEND, 0644);
    if(inv_or_not == 0){
      strcpy(resp.yes_or_no, "no");
      time(&ltime);
      timeinfo = localtime ( &ltime );
      sprintf(pid, "%s Worker PID#%d is responding to client PID#%ld: the matrix is NOT invertible\n", asctime(timeinfo),getpid(), (long)pid_of_cli);
      write(fd3, pid, strlen(pid));
      int shm_fd;
      shm_fd = shm_open("I_shrd", O_RDWR, 0666);
      int* ptr = (int*)mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);
      ptr[0]++;
      close(shm_fd);
    }
    else{
      strcpy(resp.yes_or_no, "yes");
      time(&ltime);
      timeinfo = localtime ( &ltime );
      sprintf(pid, "%s Worker PID#%d is responding to client PID#%ld: the matrix is invertible\n", asctime(timeinfo), getpid(), (long)pid_of_cli);
      write(fd3, pid, strlen(pid));
      int shm_fd;
      shm_fd = shm_open("I_shrd", O_RDWR, 0666);
      int* ptr = (int*)mmap(0, sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0);
      ptr[1]++;
      close(shm_fd);
    }
    sleep(t);
    int clientFd = open(clientFifo, O_WRONLY);
    if(clientFd == -1){
      fprintf(stderr,"open %s", clientFifo);
    }
    if(write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
      fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
    if(close(clientFd) == -1)
      fprintf(stderr,"close");
    close(fd3);
  }
  exit(0);
}


void create_sem(int index){
  char sem_name[10];
  sprintf(sem_name, "sem_%d", index);
  //printf("SEM NAME: %s\n", sem_name);
  sem_t *sem_prod = sem_open(sem_name, O_CREAT, 0660,0);
  if(&sem_prod == -1){
    perror("sem failed");
    exit(0);
  }
}

void create_semZ(int index){
  char sem_name[10];
  sprintf(sem_name, "semZ_%d", index);
  sem_t *sem_prod = sem_open(sem_name, O_CREAT, 0660,0);
  if(&sem_prod == -1){
    perror("sem failed");
    exit(0);
  }
}

void parse_coordinates(char **lines, int size, int **result){
    int j,z;
    char temp1[200];

    for(z=0;z<size;z++){
        strcpy(temp1, lines[z]);
        j=0;
        char *token1 = strtok(temp1, ",");
        while(token1){
          int d_test;
          sscanf(token1, "%d", &d_test);
          result[z][j] = d_test;
          j++;
          if(j==size){
            j=0;
          }
          token1 = strtok(0, ",");
        }
    }
}

int is_Invertible(int size, char * test_path){
    char *lines[500];
    int i;
    for (i = 0; i < size; i++)
        lines[i] = (char*)malloc(500 * sizeof(char));
    int **covmat1 = (int**)malloc(size * sizeof(int*));
    for (i = 0; i < size; i++)
        covmat1[i] = (int*)malloc(size * sizeof(int));
    char buf[200];
    off_t offset = 0;

    int len;
    i=0;
    while ((len = readline (buf, 200, test_path, &offset)) != -1){
        strcpy(lines[i], buf);
        i++;
    }
    parse_coordinates(lines, size, covmat1);
    int resultie = findInvertibility(covmat1, size, size);
    free(*lines);
    free(covmat1);
    return resultie;
}

void findcofact(int **mat, int **temp, int p, int q, int n, int size){
    int i=0, j=0;
    int row;
    for (row = 0; row < n; row++)
    {
        for (int col = 0; col < n; col++){
            if (row != p && col != q){
                temp[i][j++] = mat[row][col];
                if (j == n - 1){
                    j = 0;
                    i++;
                }
            }
        }
    }
}


int findInvertibility(int **mat, int n, int size){
    int det = 0;

    if (n == 1)
        return mat[0][0];
    int i,j;
    int **temp = (int**)malloc(n * sizeof(int*));
    for (i = 0; i < n; i++)
        temp[i] = (int*)malloc(n * sizeof(int));

    int sign = 1;
    for (j = 0; j<n; j++){
        findcofact(mat, temp, 0, j, n, size);
        det += sign * mat[0][j]* findInvertibility(temp, n-1, size);
        sign = -sign;
    }
    free(temp);
    return det;
}

ssize_t readline (char *buf, size_t sz, char *fn, off_t *offset){
    
    int fd = open (fn, O_RDONLY);
    if (fd == -1){
        fprintf (stderr, "%s() error: file open failed '%s'.\n",
                __func__, fn);
        return -1;
    }

    ssize_t nchr = 0;
    ssize_t idx = 0;
    char *p = NULL;

    if ((nchr = lseek (fd, *offset, SEEK_SET)) != -1)
        nchr = read (fd, buf, sz);

    if (nchr == -1) { 
        fprintf (stderr, "%s() error: read failure in '%s'.\n",
                __func__, fn);
        return nchr;
    }

    if (nchr == 0) return -1;

    p = buf; 
    while (idx < nchr && *p != '\n') p++, idx++;
    *p = 0;

    if (idx == nchr) { 
        *offset += nchr;

        return nchr < (ssize_t)sz ? nchr : 0;
    }

    *offset += idx + 1;
    return idx;
}


QNode *create_node(){
    if (queue->empty != QNULL) {
        QNode *node = queue->nodes + queue->empty;

        queue->empty = queue->nodes[queue->empty].next;
        return node;
    } else {
        if (queue->used < REQ_SIZE) return &queue->nodes[queue->used++];
    }
    return NULL;
}

void delete_node(QNode *node){
    if (node) {
        node->next = queue->empty;
        queue->empty = node - queue->nodes;
    }
}

QNode *qnode(size_t index){
    return (index == QNULL) ? NULL : queue->nodes + index;
}

QNode *next_node(const QNode *node){
    return qnode(node->next);
}

QNode *add_node(size_t *head, const char *str){
    QNode *node = create_node();
    if (node) {
        strncpy(node->request, str, sizeof(node->request));
        node->next = *head;
        *head = node - queue->nodes;
    }
    return node;
}

void remove_node(size_t *head){
    if (*head != QNULL) {
        size_t next = queue->nodes[*head].next;
        delete_node(&queue->nodes[*head]);
        *head = next;
    }
}
