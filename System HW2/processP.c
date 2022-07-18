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

int binary_to_ascii(char *filename, char* outputfile);
ssize_t readline (char *buf, size_t sz, char *fn, off_t *offset);
void parse_coordinates();
double calculate_frobenius(double matrix[3][3]);
void calculate_closest();
void SIGINT_EXIT();
int dead_childs = 0;
int child_count = 0;
int size = 1;
char **coordinates;
double *norms;
int* ptr;
char *output_filename = NULL;
sig_atomic_t child_exit_status;
sig_atomic_t SIGINT_FLAG = 0;

void handler(int signal_number){
	if(signal_number == SIGINT){
		SIGINT_FLAG = 1;
	}
}

void clean_up_child_process(int signal_number){
	pid_t pid;
	int status=0;
	while (dead_childs != child_count) {
		pid=waitpid(-1,&status,0);
		dead_childs++;
  		printf("Process %d terminated\n",pid);
	}
}


int spawn(char* program, char **arglist, char **envVec){
	
	pid_t child_pid;
	
	child_pid = fork();
	if(child_pid != 0){
		return child_pid;
	}
	else{
		execve(program, arglist, envVec);
		fprintf(stderr, "An error occured\n");
		abort();
	}
}


int main(int argc, char **argv)
{
	char *input_filename = NULL;
	
	char** envArgs;
	int index;
	int c;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &handler;
	sigaction(SIGINT, &sa, NULL);
	dead_childs = 0;

	opterr = 0;
	while ((c = getopt (argc, argv,"i:o:")) != -1)
    switch (c)
      {
      case 'i':
        input_filename = optarg;
        break;
      case 'o':
        output_filename = optarg;
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
    	printf ("Non-option argument %s\n", argv[index]);

    if(output_filename == NULL || input_filename == NULL){
    	printf("An argument is missing!\n");
    	printf("Example Usage Is:  ./test -i inputfilepath -o outputfilepath \n");
    	exit(1);
    }

    binary_to_ascii(input_filename, output_filename);

    char buf[200];
  	off_t offset = 0;

	int len;
	int i=0;
	while ((len = readline (buf, 200, output_filename, &offset)) != -1){
		strcpy(coordinates[i], buf);
		i++;
		if(SIGINT_FLAG == 1){
			SIGINT_EXIT();
		}
	}
	parse_coordinates();

	/*for(i=0;i<child_count;i++){
		printf("Norm %d : %lf\n", i, norms[i]);
		if(SIGINT_FLAG == 1){
			SIGINT_EXIT();
		}
	}*/
	calculate_closest();

	return 0;
}

void SIGINT_EXIT(){
	int i;
	printf("\nRECEIVED SIGINT EXITING GRACEFULLY\n");
	for(i=0;i<child_count;i++){
		free(coordinates[i]);
	}
	free(coordinates);
	free(norms);
	for(i=0;i<child_count;i++){
		kill(ptr[i], SIGUSR1);
	}
	free(ptr);
	printf("ALLOCATED SPACES ARE FREED\n");
	printf("CHILD PROCESS KILLED\n");
	remove(output_filename);
	printf("Output file deleted\n");
	exit(-1);
}

void calculate_closest(){
	int i,j;
	int l=0, m=1;
	for(i=0;i<child_count-1;i++){
		for(j=i+1;j<child_count;j++){
			if(abs(norms[i]-norms[j]) < abs(norms[l]-norms[m])){
				l=i;
				m=j;
			}
		}
		if(SIGINT_FLAG == 1){
			SIGINT_EXIT();
		}
	}
	printf("The closest 2 matrices are %s and %s, their distance is %lf\n", coordinates[l], coordinates[m], (double)abs((double)norms[l]-(double)norms[m]));
}

int binary_to_ascii(char *filename, char* outputfile){

	char allline[30];
	int coordinatlar[3][10];
	int i,j;
	int fd = open(outputfile, O_CREAT | O_WRONLY | O_APPEND, 0644);
	char fdstr[5];
	sprintf(fdstr, "%d", fd);
	int fp = open(filename, O_RDONLY);
	if(fp < 0){
		printf("File not found process terminated. \n");
		exit(1);
	}
	else{
		printf("Process P reading %s\n", filename);
		while(read(fp,allline,30) == 30){
			//printf("%s\n", allline);
			for(i=0;i<30;i+=3){
				coordinatlar[0][i/3] = (int)allline[i];
				coordinatlar[1][i/3] = (int)allline[i+1];
				coordinatlar[2][i/3] = (int)allline[i+2];
			}
			int lnVar=0;
		    char **keysChar=(char**)malloc(sizeof(char*)*10);
		    for(lnVar=0;lnVar<10;lnVar++)
		    {
		        keysChar[lnVar]=(char*)malloc(10);
		    }
		    for(i=0;i<10;i++)
					sprintf(keysChar[i], "%d,%d,%d", coordinatlar[0][i], coordinatlar[1][i],coordinatlar[2][i]);

			char* arglist[] = {outputfile, fdstr, NULL};
	
			char* envVec[] = {keysChar[0],keysChar[1],keysChar[2],keysChar[3],keysChar[4],keysChar[5],keysChar[6],keysChar[7],keysChar[8],keysChar[9], NULL};
			int child_pid = spawn("./processC", arglist, envVec);
			/*******************************************/
			if(child_count == 0){
				ptr = (int *)malloc(sizeof(int));
				ptr[0] = child_pid;
			}
			else{
			 	int *new_ptr;
			 	new_ptr = (int *)realloc(ptr, (child_count+1)*sizeof(int));
			 	ptr = new_ptr;
				ptr[child_count] = child_pid; 
			}
			/*******************************************/
			child_count = child_count + 1;
			printf("Created R_%d with (%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s),(%s)\n", child_count, keysChar[0],keysChar[1],keysChar[2],keysChar[3],keysChar[4],keysChar[5],keysChar[6],keysChar[7],keysChar[8],keysChar[9]);
			free(keysChar);
		}
		printf("Reached EOF, collecting outputs from %s\n", outputfile);
	}
	
	norms = (double*)malloc(child_count);

	coordinates = malloc(child_count*sizeof(char*));
  for (i = 0; i < child_count; i++)
      coordinates[i] = (char*)malloc(300 * sizeof(char));
	pid_t pid;
	int status=0;
	while (pid=waitpid(-1,&status,0) != -1) {
  	if(SIGINT_FLAG == 1){
			SIGINT_EXIT();
		}
	}
	if(SIGINT_FLAG == 1){
		SIGINT_EXIT();
	}
	//printf("%d\n", child_count);
	return 0;
}


void parse_coordinates(){
  int i,j,z;
  double covmat1[3][3];
  char temp1[200];

  for(z=0;z<child_count;z++){
    strcpy(temp1, coordinates[z]);
    i=0; 
    j=0;
    char *token1 = strtok(temp1, ",");
    while(token1){
      double d_test;
      sscanf(token1, "%lf", &d_test);
      covmat1[i][j] = d_test;
      j++;
      if(j==3){
        j=0;
        i++;
      }
      token1 = strtok(0, ",");
    }
    double result = calculate_frobenius(covmat1);
    norms[z] = result;
    if(SIGINT_FLAG == 1){
		SIGINT_EXIT();
	}
  }
}

double calculate_frobenius(double matrix[3][3]){
  int i,j;
  double total = 0.0;
  for(i=0;i<3;i++){
    for(j=0;j<3;j++){
      total = total + (matrix[i][j]*matrix[i][j]);
    }
    if(SIGINT_FLAG == 1){
		SIGINT_EXIT();
	}
  }

  return sqrt(total);
}

ssize_t readline (char *buf, size_t sz, char *fn, off_t *offset)
{
    
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





