#include "client.h"


int main(int argc, char **argv){
	char filename1[1024];
	int i;
	int opt,f1_flag=0, port_flag=0, ip_flag=0;
    
    while((opt = getopt(argc, argv, ":r:q:s:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'r': 
            	sprintf(filename1,"%s",optarg);
            	f1_flag=1;
            	break;
            case 'q':
            	port_num = atoi(optarg); 
            	port_flag=1;
            	break;
            case 's':
            	sprintf(ip_name,"%s",optarg); 
            	ip_flag=1;
            	break;
        } 
    }
    
    if(f1_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(port_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(ip_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}


	read_file(filename1);
	printf("I have loaded %d requests and I'm creating %d threads.\n", lines, lines);

	int a[lines];
	pthread_t calculators[lines];
	for(i = 0; i < lines; i++) {
        a[i] = i;
    }
    for(i = 0; i < lines; i++) {
        pthread_create(&calculators[i], NULL, (void *)requester_thread, (void *)&a[i]);
    }


	for(i = 0; i < lines; i++) {
        pthread_join(calculators[i], NULL);
    }

    printf("Client: All threads are terminated, goodbye.\n");

	free_allocated();
}

void allocate_space(){
	int i;
	read_file_array = (char**)malloc(lines * sizeof(char*));
    for (i = 0; i < lines; i++)
        read_file_array[i] = (char*)malloc(1024 * sizeof(char));
}


void get_line_count(char filename[1024]){
	int line_count = 0;

	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        if(read != 1){
        	line_count++;	
        } 
    }

    fclose(fp);
    free(line);

    lines = line_count;
}


void free_allocated(){
	int i;
	for (i = 0; i < lines; i++)
        free(read_file_array[i]);
 
    free(read_file_array);
}


void print_lines(){
	int i;
	for(i=0;i<lines;i++){
		printf("%s\n", read_file_array[i]);
	}
}

void read_file(char filename[1024]){
	
	get_line_count(filename);

	allocate_space(lines);

	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int index = 0;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        if(read != 1){
        	line[strlen(line)-1] = '\0';
        	strcpy(read_file_array[index], line);
        	index++;
        } 
    }
}


void* requester_thread(void* line){
	int i;
	int thread_index = *(int*) line;
	char buffer[1024];
	pthread_mutex_lock(&lock);
	++arrived;
	
	printf("Client-Thread-%d: Thread-%d has been created.\n", thread_index, thread_index);
	if(arrived < lines){
		pthread_cond_wait(&cond1, &lock);
		printf("Client-Thread-%d: I am requesting %s\n", thread_index, read_file_array[thread_index]);
	}
	else{
		for(i=0;i<lines-1;i++)
			pthread_cond_signal(&cond1);
		printf("Client-Thread-%d: I am requesting %s\n", thread_index, read_file_array[thread_index]);
	}
	pthread_mutex_unlock(&lock);

	int socket_val = 0, client_fd;
    struct sockaddr_in address_of_server;

	if ((socket_val = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket creation failed\n");
        return NULL;
    }

    address_of_server.sin_family = AF_INET;
    address_of_server.sin_port = htons(port_num);

	if (inet_pton(AF_INET, ip_name, &address_of_server.sin_addr) <= 0) {
        fprintf(stderr, "Socket address is invalid\n");
        return NULL;
    }

    if ((client_fd = connect(socket_val, (struct sockaddr*)&address_of_server, sizeof(address_of_server))) < 0) {
        fprintf(stderr, "Connection to socket failed\n");
        return NULL;
    }

    send(socket_val, read_file_array[thread_index], strlen(read_file_array[thread_index]), 0);

    read(socket_val, buffer, 1024);

    if(atoi(buffer) == -1){
    	printf("Client-Thread-%d: The server's response to %s is ERROR\n", thread_index, read_file_array[thread_index]);
    }
    else{
    	printf("Client-Thread-%d: The server's response to %s is %d\n", thread_index, read_file_array[thread_index], atoi(buffer));
    }

    printf("Client-Thread-%d: Terminating.\n", thread_index);
	return NULL;
}


