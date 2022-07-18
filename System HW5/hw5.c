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



char filename1[256];
char filename2[256];
char outputfile[256];
int** matrix1;
int** matrix2;
int** matrixC;
double** matrixReel;
double** matrixImag;
int dimension;
int size,thread_count;
int arrived = 0;
clock_t begin;
int sigint_Flag = 0;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;



void handler(int signal_number){
  
  if(signal_number == SIGINT){
  	printf("SIGINT received exiting.\n");
    sigint_Flag = 1;
  }
  
}

int read_file_to_array(int dimension, int **matrix, char* filepath){
	char value[dimension];
	int bytes;
	int fd = open(filepath, O_RDONLY);
	int i,j;

	for(i=0;i<dimension;i++){
		bytes = read(fd, &value, dimension);
		if(bytes != dimension || (bytes == dimension && (value[dimension-1] == EOF))){
			fprintf(stderr, "File size is invalid\n");
			return -1;
		}
		for(j=0;j<dimension;j++){
			matrix[i][j] = value[j];
		}
	}
	return 1;
}

void write_array_to_file(int dimension){
	int i,j;
	int fd = open(outputfile, O_CREAT|O_WRONLY|O_APPEND, 0777);
	char line[1024];
	for(i=0;i<dimension;i++){
		for(j=0;j<dimension;j++){
			sprintf(line, "%.3f+%.3fi", matrixReel[i][j], matrixImag[i][j]);
			write(fd, line, strlen(line));
			if(j != dimension-1){
				write(fd,",",strlen(","));
			}
		}
		write(fd,"\n",strlen("\n"));
	}	
	close(fd);
}

int power(int times){

	int i;
	int result = 1;
	for(i=0;i<times;i++){
		result = result * 2;
	}

	return result;
}

void print_matrix(int **matrix, int size){
	int i,j;
	for(i=0;i<size;i++){
		for(j=0;j<size;j++){
			printf("%d, ", matrix[j][i]);
		}
		printf("\n");
	}
}

void print_matrix_double(double **matrix, int size){
	int i,j;
	for(i=0;i<size;i++){
		for(j=0;j<size;j++){
			printf("%.3f, ", matrix[i][j]);
		}
		printf("\n");
	}
}

void free_matrix(int** matrix, int size){
	int i;
	for (i = 0; i < size; i++)
        free(matrix[i]);
 
    free(matrix);
}

void free_all(){
	int i;
	free_matrix(matrix1, dimension);
    free_matrix(matrix2, dimension);
    free_matrix(matrixC, dimension);
    for (i = 0; i < size; i++)
        free(matrixReel[i]);
    free(matrixReel);
    for (i = 0; i < size; i++)
        free(matrixImag[i]);
    free(matrixImag);

}


void* calculator(void* tnum){
	time_t ltime;
    struct tm * timeinfo;
	int thread_index = *(int*) tnum;
	int start_index = round((double)dimension/(double)thread_count)*thread_index;
	int end_index = ((thread_index+1)*round((double)dimension/(double)thread_count))-1;
	
	if(start_index > dimension -1){
		start_index = dimension-1;
	}
	if(end_index >= dimension){
		end_index = dimension - 1;
	}
	if(thread_index == thread_count-1 && end_index < dimension-1){
		end_index = dimension - 1;
	}
	if(thread_count > dimension){
		if(thread_index >= dimension){
			start_index = dimension-1;
			end_index = dimension-1;
		}
		else{
			start_index = thread_index;
			end_index = thread_index;
		}
	}
	clock_t begen = clock();
	int i=0, j=0,z;
	int total = 0;
	double reel = 0.0;
	double imaginer = 0.0;
	for(z=start_index;z<=end_index;z++){
		for(i=0;i<dimension;i++){
			for(j=0;j<dimension;j++){
				total = total + (matrix1[i][j]*matrix2[j][z]);
				if(sigint_Flag == 1){
					printf("Thread %d is received sigint exiting.\n", thread_index);
					return NULL;
				}
			}
			matrixC[i][z] = total;
			total = 0;
		}
	}

	pthread_mutex_lock(&lock);
	++arrived;
	clock_t end = clock();
	double time_sp = 0.0;
	time_sp = (double)(end - begen) / CLOCKS_PER_SEC;
	time(&ltime);
	timeinfo = localtime ( &ltime );
	printf("%s Thread %d has reached the rendezvous point in %f seconds.\n", asctime(timeinfo),thread_index, time_sp);
	if(arrived < thread_count){
		pthread_cond_wait(&cond1, &lock);
		time(&ltime);
		timeinfo = localtime ( &ltime );
		printf("%s Thread %d is advancing to the second part.\n", asctime(timeinfo),thread_index);
	}
	else{
		for(i=0;i<thread_count-1;i++)
			pthread_cond_signal(&cond1);
		time(&ltime);
		timeinfo = localtime ( &ltime );
		printf("%s Thread %d is advancing to the second part.\n", asctime(timeinfo),thread_index);
	}
	pthread_mutex_unlock(&lock);
	begen = clock();

	int k,l,m,n;
	for(k=0;k<dimension;k++){ // k
		for(l=start_index;l<=end_index;l++){ // l
			reel = 0.0;
			imaginer = 0.0;
			for(m=0;m<dimension;m++){ // m
				for(n=0;n<dimension;n++){ // n
					reel = reel + matrixC[n][m]*cos(2 * 3.141592 * (((double)(k*n)/(double)dimension) + ((double)(m*l)/(double)dimension)));
					imaginer = imaginer + matrixC[n][m]*sin(-2 * 3.141592 * (((double)(k*n)/(double)dimension) + ((double)(m*l)/(double)dimension)));
					if(sigint_Flag == 1){
						free_all();
						printf("Thread %d is received sigint exiting.\n", thread_index);
						return NULL;
					}
				}
			}
			matrixReel[k][l] = reel;
			matrixImag[k][l] = imaginer;
		}
	}

	end = clock();
	time_sp = (double)(end - begen) / CLOCKS_PER_SEC;
	time(&ltime);
	timeinfo = localtime ( &ltime );
	printf("%s Thread %d has finished the second part in %f seconds.\n", asctime(timeinfo),thread_index, time_sp);
	return NULL;
}


int main(int argc, char **argv){
	int opt,f1_flag=0, f2_flag=0, of_flag=0, size_flag=0, th_flag=0;
	
	double time_spent = 0.0;
	time_t ltime;
    struct tm * timeinfo;
	struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handler;
    sigaction(SIGINT, &sa, NULL);
    
    while((opt = getopt(argc, argv, ":i:j:o:n:m:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'i': 
            	sprintf(filename1,"%s",optarg);
            	f1_flag=1;
            	break;
            case 'j':
            	sprintf(filename2,"%s",optarg); 
            	f2_flag=1;
            	break;
            case 'o':
            	sprintf(outputfile,"%s",optarg); 
            	of_flag=1;
            	break;
            case 'n':
            	size = atoi(optarg); 
            	size_flag=1;
            	break;
            case 'm':
            	thread_count = atoi(optarg); 
            	th_flag=1;
            	break;
        } 
    }
    
    if(f1_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(f2_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(of_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
    if(size_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(th_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
    
    if(size <= 2){
    	fprintf(stderr, "Size is invalid\n");
		return -1;
    }
    if(thread_count <2 || thread_count%2==1){
    	fprintf(stderr, "Thread count is invalid\n");
		return -1;
    }
    
    dimension = power(size);
    begin = clock();


    int i;
	matrix1 = (int**)malloc(dimension * sizeof(int*));
	matrix2 = (int**)malloc(dimension * sizeof(int*));
	matrixC = (int**)malloc(dimension * sizeof(int*));
	matrixReel = (double**)malloc(dimension * sizeof(double*));
	matrixImag = (double**)malloc(dimension * sizeof(double*));
    for (i = 0; i < dimension; i++)
        matrix1[i] = (int*)malloc(dimension * sizeof(int));
    for (i = 0; i < dimension; i++)
        matrix2[i] = (int*)malloc(dimension * sizeof(int));
    for (i = 0; i < dimension; i++)
        matrixC[i] = (int*)malloc(dimension * sizeof(int));
    for (i = 0; i < dimension; i++)
        matrixReel[i] = (double*)malloc(dimension * sizeof(double));
    for (i = 0; i < dimension; i++)
        matrixImag[i] = (double*)malloc(dimension * sizeof(double));

    int is_Valid;
    is_Valid = read_file_to_array(dimension, matrix1, filename1);
    if(is_Valid == -1){
    	free_all();
    	fprintf(stderr, "One of the input file has invalid size!\n");
    	exit(0);
    }
    /*print_matrix(matrix1, dimension);
    printf("\n\n\n");*/
    is_Valid = read_file_to_array(dimension, matrix2, filename2);
    if(is_Valid == -1){
    	free_all();
    	fprintf(stderr, "One of the input file has invalid size!\n");
    	exit(0);
    }
    //print_matrix(matrix2, dimension);
    time(&ltime);
	timeinfo = localtime ( &ltime );
    printf("%s Two matrices of size %dx%d have been read. The number of threads is %d\n", asctime(timeinfo),dimension, dimension, thread_count);

	pthread_t calculators[thread_count];
	int a[thread_count];
	for(i = 0; i < thread_count; i++) {
        a[i] = i;
    }
    for(i = 0; i < thread_count; i++) {
        pthread_create(&calculators[i], NULL, (void *)calculator, (void *)&a[i]);
    }


	for(i = 0; i < thread_count; i++) {
        pthread_join(calculators[i], NULL);
    }


    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;



    write_array_to_file(dimension);
	time(&ltime);
	timeinfo = localtime ( &ltime );
    printf("%s The process has written the output file. The total time spent is %f second.\n", asctime(timeinfo),time_spent);

    free_all();
}
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

