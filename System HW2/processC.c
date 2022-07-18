#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

extern char** environ;
int coordinates[3][10];
double calculate_covariance(int column1, int column2, double avg1, double avg2);
void set_coordinates(char* row, int row_num);
double calculate_mean(int column);
void SIGINT_EXIT();
char* cov_mat_line;
sig_atomic_t SIGUSR_FLAG = 0;

void handler(int signal_number){

	if(signal_number == SIGUSR1){
		SIGUSR_FLAG = 1;
	}

}


int main(int argc, char *argv[])
{	
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &handler;
	sigaction(SIGUSR1, &sa, NULL);

	char* outputfile = argv[0];
	int startfd =3;
	int newfd = open(outputfile, O_WRONLY | O_APPEND);
	char **ep;
	int row_num = 0;
	for(ep=environ;*ep != NULL;ep++){
		set_coordinates(*ep, row_num);
		row_num++;
		if(SIGUSR_FLAG == 1){
			SIGINT_EXIT();
		}
	}

	double avg_x = calculate_mean(0);
	double avg_y = calculate_mean(1);
	double avg_z = calculate_mean(2);

	double cov_mat[3][3];

	cov_mat[0][0] = calculate_covariance(0,0,avg_x,avg_x);
	cov_mat[0][1] = calculate_covariance(0,1,avg_x,avg_y);
	cov_mat[0][2] = calculate_covariance(0,2,avg_x,avg_z);
	cov_mat[1][0] = calculate_covariance(1,0,avg_y,avg_x);
	cov_mat[1][1] = calculate_covariance(1,1,avg_y,avg_y);
	cov_mat[1][2] = calculate_covariance(1,2,avg_y,avg_z);
	cov_mat[2][0] = calculate_covariance(2,0,avg_z,avg_x);
	cov_mat[2][1] = calculate_covariance(2,1,avg_z,avg_y);
	cov_mat[2][2] = calculate_covariance(2,2,avg_z,avg_z);
	if(SIGUSR_FLAG == 1){
		SIGINT_EXIT();
	}
	
	cov_mat_line = (char *) malloc(200);

	sprintf(cov_mat_line, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", cov_mat[0][0], cov_mat[0][1],  cov_mat[0][2], cov_mat[1][0], cov_mat[1][1], cov_mat[1][2], cov_mat[2][0],  cov_mat[2][1], cov_mat[2][2]);

	struct flock lock;
	memset(&lock,0,sizeof(lock));
	lock.l_type = F_WRLCK;
	fcntl(newfd, F_SETLKW, &lock);
	write(newfd,cov_mat_line,strlen(cov_mat_line));
	lock.l_type = F_UNLCK;
	fcntl(newfd, F_SETLKW, &lock);
	
	close(newfd);
	free(cov_mat_line);
	exit(0);
}

void SIGINT_EXIT(){
	printf("CHILD PROCESS RECEIVED SIGNAL\n");
	free(cov_mat_line);
	printf("ALLOCATED SPACES ARE FREED\n");
	exit(-1);
}


void set_coordinates(char* row, int row_num){
	char* token;
	char temp[100];
	int count = 0;
	strcpy(temp, row);
	token = strtok(temp, ",");
	while(token){
		if(count == 0){
			coordinates[0][row_num] = atoi(token);
		}
		else if(count == 1){
			coordinates[1][row_num] = atoi(token);
		}
		else{
			coordinates[2][row_num] = atoi(token);
		}
		count++;
		token = strtok(0, ",");
		if(SIGUSR_FLAG == 1){
			SIGINT_EXIT();
		}
	}
}

double calculate_mean(int column){
	int i = 0;
	int total = 0;
	for(i=0;i<10;i++){
		total = total + coordinates[column][i];
		if(SIGUSR_FLAG == 1){
			SIGINT_EXIT();
		}
	}
	return total/10.0;
}

double calculate_covariance(int column1, int column2, double avg1, double avg2){
	double total = 0.0;
	int i,j;

	for(i=0;i<10;i++){
		total = total + ((coordinates[column1][i]-avg1)*(coordinates[column2][i]-avg2));
		if(SIGUSR_FLAG == 1){
			SIGINT_EXIT();
		}
	}

	return total/10.0;
}

