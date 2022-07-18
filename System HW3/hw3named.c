#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>

typedef struct ingredients{
	char arr[2];
	int final;
	int chef_control;
}ing_table;


char named_sem[256];

sem_t *sem1;
sem_t *sem2;
sem_t *sem3;
sem_t *semp;


void chefWaits(char chefprod1, char chefprod2, int chefVal){
	char needs[2][10];

	if(chefprod1=='M' && chefprod2=='W'){
		sprintf(needs[0],"%s","sugar");
		sprintf(needs[1],"%s","flour");
	}
	else if(chefprod1=='M' && chefprod2=='S'){
		sprintf(needs[0],"%s","flour");
		sprintf(needs[1],"%s","walnuts");
	}
	else if(chefprod1=='M' && chefprod2=='F'){
		sprintf(needs[0],"%s","sugar");
		sprintf(needs[1],"%s","walnuts");	
	}
	else if(chefprod1=='F' && chefprod2=='S'){
		sprintf(needs[0],"%s","milk");
		sprintf(needs[1],"%s","walnuts");	
	}
	else if(chefprod1=='F' && chefprod2=='W'){
		sprintf(needs[0],"%s","milk");
		sprintf(needs[1],"%s","sugar");
	}
	else if(chefprod1=='S' && chefprod2=='W'){
		sprintf(needs[0],"%s","milk");
		sprintf(needs[1],"%s","flour");	
	}

	printf("chef%d (PID %d) is waiting for %s and %s\n",chefVal,getpid(),needs[0],needs[1]);

}

void ingredientTaker(int index,int chefVal){

	int shm_fd;
	shm_fd = shm_open("I_shrd", O_RDWR, 0666);
	ing_table* ptr = (ing_table*)mmap(0, sizeof(ing_table), PROT_WRITE, MAP_SHARED, shm_fd, 0);
	printf("chef%d (PID %d) Ingredient Table: %c - %c\n", chefVal, getpid(), ptr->arr[0], ptr->arr[1]);
	char ingre = ptr->arr[index];
	ptr->arr[index] = '.';
	close(shm_fd);

	char taken[10];

	switch(ingre){
		case 'M': sprintf(taken,"%s","milk"); break;
		case 'S': sprintf(taken,"%s","sugar"); break;
		case 'W': sprintf(taken,"%s","walnut"); break;
		case 'F': sprintf(taken,"%s","flour"); break;
	}

	printf("chef%d (PID %d) has taken the %s.\n",chefVal,getpid(), taken);
	
	printf("chef%d (PID %d) Ingredient Table: %c - %c\n", chefVal, getpid(), ptr->arr[0], ptr->arr[1]);
}



void chef(char ingredient1, char ingredient2, int chef_value){
	sem_t *sem1 = sem_open(named_sem,0);
	sem_t *sem2 = sem_open("sem2",0);
	sem_t *sem3 = sem_open("sem3",0);

	chefWaits(ingredient1, ingredient2,chef_value);
	sem_post(sem1);

	int t=0,flag=1;
	int shm_fd;
	shm_fd = shm_open("I_shrd", O_RDWR, 0666);
	ing_table* ptr = (ing_table*)mmap(0, sizeof(ing_table), PROT_WRITE, MAP_SHARED, shm_fd, 0);

    while(ptr->final==1)
    {
    	if(ptr->final==1){
    		sem_wait(sem2);
    	}

   		if(t<1){
	    	ptr->chef_control++;
   		}
    	if(ingredient1!=ptr->arr[0] && ingredient1!=ptr->arr[1] && ingredient2!=ptr->arr[0] && ingredient2!=ptr->arr[1] && ptr->final==1){
			ingredientTaker(0,chef_value);
  			ingredientTaker(1,chef_value);
    		ptr->chef_control=0;
    		printf("chef%d (PID %d) is preparing the dessert\n",chef_value, getpid());
    		printf("chef%d (PID %d) has delivered the dessert to the wholesaler\n",chef_value, getpid());
	    	sem_post(sem3);
    		chefWaits(ingredient1, ingredient2, chef_value);
    		t=0;
    		sem_post(sem1);
    		flag=0;
    	}
    	t++;

	    if(ptr->chef_control==6)
	    	sem_post(sem1);
	    if(flag==1)
  	    	sem_post(sem2);
  	    flag=1;
    }
    close(shm_fd);
    printf("chef%d (PID %d) is exiting\n",chef_value, getpid());
    exit(0);
}

void pusher(){
	semp = sem_open("semp",0);
	sem2 = sem_open("sem2",0);

	int shm_fd;
	shm_fd = shm_open("I_shrd", O_RDWR, 0666);
	ing_table* ptr = (ing_table*)mmap(0, sizeof(ing_table), PROT_WRITE, MAP_SHARED, shm_fd, 0);

	
	while(ptr->final==1){
		sem_wait(semp);
		sem_post(sem2);
	}
	close(shm_fd);
}



int main(int argc, char **argv){
    int opt,input_flag=0, sem_name_flag=0;    
    char filepath[256];

    while((opt = getopt(argc, argv, ":i:n:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'i': 
            	sprintf(filepath,"%s",optarg); 
            	input_flag=1;
            	break;
            case 'n':
            	sprintf(named_sem,"%s",optarg); 
            	sem_name_flag=1;
            	break;
        }  
    }
    if(input_flag==0){
    	fprintf(stderr, "Usage : For argument (input file argument) not given\n");
		return -1;      
    }
    if(sem_name_flag==0){
    	fprintf(stderr, "Usage : For argument (semaphore name) not given\n");
		return -1;      
    }


    int I_shm;
	I_shm = shm_open("I_shrd", O_CREAT | O_RDWR, 0666);
	ftruncate(I_shm, sizeof(ing_table));
	ing_table* I_ptr = (ing_table*)mmap(0, sizeof(ing_table), PROT_WRITE, MAP_SHARED, I_shm, 0);
    I_ptr->final=1;

    sem1 = sem_open(named_sem, O_CREAT, 0660,0);
    sem2 = sem_open("sem2", O_CREAT, 0660,0);
    sem3 = sem_open("sem3", O_CREAT, 0660,0);
    semp = sem_open("semp", O_CREAT, 0660,0);

    int pidim = fork();
    if(pidim == -1){
      return -1;
    }
    else if(pidim == 0){
      pusher();
      exit(0);
    }

    int i=0;
    char ingred1;
    char ingred2;
    for(i=0;i<6;i++){
		switch(i){
    		case 0: ingred1='M'; ingred2='W'; break;
    		case 1: ingred1='M'; ingred2='S'; break;
    		case 2: ingred1='M'; ingred2='F'; break;
    		case 3: ingred1='F'; ingred2='S'; break;
    		case 4: ingred1='F'; ingred2='W'; break;
    		case 5: ingred1='S'; ingred2='W'; break;
    	}

		switch(fork()){
			case -1: fprintf(stderr, "Couldn't fork\n");
			case 0: chef(ingred1, ingred2, i+1);
			default: break;
		}
		sem_wait(sem1);
    }

    char *filePath;
    int flag=1,bytes;
    filePath = filepath;
    char ingredient1,ingredient2,newLine;
   	int fd = open(filePath, O_RDONLY);
   	int des_count=0;
   	

    while(flag==1){
	    bytes = read(fd, &ingredient1, 1);
	    bytes = read(fd, &ingredient2, 1);
		bytes = read(fd, &newLine, 1);
		I_ptr->arr[0]=ingredient1;
    	I_ptr->arr[1]=ingredient2;
    	I_ptr->chef_control=0;
		if(bytes==0)
			flag=0;
		printf("the wholesaler (PID %d) delivers %c and %c\n", getpid(), ingredient1, ingredient2);
	    sem_post(semp);
   		printf("the wholesaler (PID %d) is waiting for the dessert\n", getpid());
	    sem_wait(sem3);
	    des_count++;
   		printf("the wholesaler (PID %d) has obtained the dessert and left to sell it\n", getpid());
	    sem_wait(sem1);

    }
  	I_ptr->final=0;
  	sem_post(semp);
    

    pid_t pid;
	int status=0;
    while ((pid=waitpid(-1,&status,0)) != -1) {}

    close(fd);
    sem_unlink(named_sem);
    sem_unlink("sem2");
    sem_unlink("sem3");
    sem_unlink("semp");
    close(I_shm);
    shm_unlink("I_shrd");
    
    printf("the wholesaler (PID %d) is done (total desserts: %d)\n", getpid(), des_count);
    return 0;
}
