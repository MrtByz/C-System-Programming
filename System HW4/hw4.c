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

int semaphores;
int numreader = 0;
int iteration_count;
int consumer_count;
char filepath[256];

void handler(int signal_number){
  
  if(signal_number == SIGINT){
    semctl(semaphores, 0, IPC_RMID);
    exit(0);
  }
}


void *writer(void *wno){
	int flag=1;
	char value;
	int bytes;
	int fd = open(filepath, O_RDONLY);
	struct sembuf vr = { 0, 0, 0};
	struct sembuf vs = { 1, 0, 0};
	union semun  {
		int val;
		struct semid_ds *buf;
		ushort *array;
	} arg;
	time_t ltime;
    struct tm * timeinfo;
	while(flag==1){

		bytes = read(fd, &value, 1);
		if(bytes==0)
			flag=0;
		else{
			if(value == '1' || value == '2'){
				
				int val_1;
				int val_2;
				val_1 = semctl(semaphores, 0, GETVAL, arg);
				val_2 = semctl(semaphores, 1, GETVAL, arg);
				time(&ltime);
				timeinfo = localtime ( &ltime );
				printf("%s Supplier: read from input a '%c'. Current amounts %d x '1', %d x '2'.\n", asctime(timeinfo),value, val_1, val_2);
				fflush(stdout);
				if(value == '1'){
					vr.sem_op = 1;
					semop(semaphores,&vr,1);
					time(&ltime);
					timeinfo = localtime ( &ltime );
					printf("%s Supplier: delivered a '%c'. Post delivery amounts %d x '1', %d x '2'.\n", asctime(timeinfo),value, val_1+1, val_2);
					fflush(stdout);
				}
				else if(value == '2'){
					vs.sem_op = 1;
					semop(semaphores,&vs,1);
					time(&ltime);
					timeinfo = localtime ( &ltime );
					printf("%s Supplier: delivered a '%c'. Post delivery amounts %d x '1', %d x '2'.\n", asctime(timeinfo),value, val_1, val_2+1);
					fflush(stdout);
				}
			}
		}
	}
	time(&ltime);
	timeinfo = localtime ( &ltime );
	printf("%s The supplier has left.\n", asctime(timeinfo));
	fflush(stdout);
	return NULL;
}

void* reader(void* rno){

	int iter_count=0;
	ushort semvals[2];
	union semun  {
		int val;
		struct semid_ds *buf;
		ushort *array;
	} arg;
	arg.array = semvals;
	struct sembuf ops[2];
	int i;
	for(i=0;i<2;i++){
		ops[i].sem_num = i;
		ops[i].sem_op = -1;
		ops[i].sem_flg = 0;
	}

	time_t ltime;
    struct tm * timeinfo;
    
	while(iter_count<iteration_count){
		semop(semaphores,ops,2);
		int val_1;
		int val_2;
    	val_1 = semctl(semaphores, 0, GETVAL, arg);
		val_2 = semctl(semaphores, 1, GETVAL, arg);

		iter_count++;
		time(&ltime);
		timeinfo = localtime ( &ltime );
		printf("%s Consumer %d: at iteration %d(waiting). Current amounts %d x '1', %d x '2'\n", asctime(timeinfo), *((int *)rno),iter_count,val_1+1,val_2+1);
		fflush(stdout);
		time(&ltime);
		timeinfo = localtime ( &ltime );
		printf("%s Consumer %d: at iteration %d(consumed). Post-consumption amounts %d x '1', %d x '2'\n", asctime(timeinfo), *((int *)rno),iter_count,val_1,val_2);
		fflush(stdout);
	}
	time(&ltime);
	timeinfo = localtime ( &ltime );
	printf("%s Consumer %d has left.\n", asctime(timeinfo), *((int *)rno));
	fflush(stdout);
	return NULL;
}


int main(int argc, char **argv) {
	ushort semvals[2];
	union semun  {
		int val;
		struct semid_ds *buf;
		ushort *array;
	} arg;

	int opt,consumer_flag=0, iter_flag=0, file_flag=0;    
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handler;
    sigaction(SIGINT, &sa, NULL);
    
    while((opt = getopt(argc, argv, ":c:n:f:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'c': 
            	consumer_count = atoi(optarg); 
            	consumer_flag=1;
            	break;
            case 'n':
            	iteration_count = atoi(optarg);  
            	iter_flag=1;
            	break;
            case 'f':
            	sprintf(filepath,"%s",optarg); 
            	file_flag=1;
            	break;
        }  
    }
    if(consumer_flag==0){
    	fprintf(stderr, "Usage : For argument (consumer number) not given\n");
		return -1;      
    }
    if(iter_flag==0){
    	fprintf(stderr, "Usage : For argument (iteration number) not given\n");
		return -1;      
    }
    if(file_flag==0){
    	fprintf(stderr, "Usage : For argument (file path) not given\n");
		return -1;      
    }

    if(consumer_count <= 4){
    	fprintf(stderr, "Consumer count is invalid\n");
		return -1;   
    }
    if(iteration_count <= 1){
    	fprintf(stderr, "Iteration count is invalid\n");
		return -1;   
    }
	semvals[0] = 0;
	semvals[1] = 0;

    semaphores = semget(IPC_PRIVATE,  2, IPC_CREAT | 0777);
    
    arg.array = semvals;
    semctl(semaphores,0,SETALL,arg);

    pthread_t readers[consumer_count];
    pthread_t supplier;
    pthread_attr_t attr;
    int s;

    s = pthread_attr_init(&attr);
    if(s != 0){
    	fprintf(stderr, "Attribute initialization fault\n");
		return -1;  
    }
    s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(s != 0){
    	fprintf(stderr, "Detach fault\n");
		return -1;  
    }

    pthread_create(&supplier, &attr, (void *)writer, (void *)1);

    s = pthread_attr_destroy(&attr);
	if(s != 0){
    	fprintf(stderr, "Destroy attribute fault\n");
		return -1;  
    }

    int i;
    int a[consumer_count];
    for(i = 0; i < consumer_count; i++) {
        a[i] = i;
    }

	for(i = 0; i < consumer_count; i++) {
        pthread_create(&readers[i], NULL, (void *)reader, (void *)&a[i]);
    }

    for(i = 0; i < consumer_count; i++) {
        pthread_join(readers[i], NULL);
    }
    semctl(semaphores, 0, IPC_RMID);
    return 0;
}
