#include "my_server.h"


int main(int argc, char* argv[]) {

	struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handler;
    sigaction(SIGINT, &sa, NULL);

	/* VARIABLES FOR OPTARG */
	int opt,th_flag=0, port_flag=0;


    /* TAKING ARGUMENTS AND ASSIGNING TO VARIABLES */
    while((opt = getopt(argc, argv, ":p:t:")) != -1)  
    {  
        switch(opt)  
        {  
        	case 'p':
            	port_num = atoi(optarg); 
            	port_flag=1;
            	break;

            case 't': 
            	thread_num = atoi(optarg);
            	th_flag=1;
            	break;
        } 
    }
    
    /* CHECKING THE ARGUMENTS */
    if(th_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(port_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}

	if(thread_num < 5){
		fprintf(stderr, "Invalid Thread Number\n");
		return -1; 
	}


	/* VARIABLES FOR THREADS AND CREATION OF THREADS */
	int i;
    pthread_t th[thread_num];
    int a[thread_num];
    for(i = 0; i < thread_num; i++) {
        a[i] = i;
    }

    pthread_mutex_init(&Listmutex, NULL);
    pthread_cond_init(&Listcond, NULL);

    threadPool=(pthread_t*)malloc(sizeof(pthread_t)*thread_num);
    
    for(i=0;i<thread_num;i++){
		if(pthread_create(&threadPool[i], NULL, clientReqHandler,(void *)&a[i])!=0){
			fprintf(stderr, "Error in thread creation\n");
			exit(EXIT_FAILURE);
		}
	}
    /* VARIABLES FOR SOCKET */

    int server_fd, new_socket;
    struct sockaddr_in address;
    int option = 1;
    int address_size = sizeof(address);

    memset(&address, '0', sizeof(address));
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fprintf(stderr, "Socket creation failed.");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
        fprintf(stderr, "Setsockopt failed.");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_num);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        fprintf(stderr, "Bind to socket failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 20) < 0) {
        fprintf(stderr, "Listen to socket failed");
        exit(EXIT_FAILURE);
    }

    while((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&address_size)) >= 0){
    	
	    pthread_mutex_lock(&main_mutex);
	    struct jobQueue thread_job;
	    thread_job.socketFd = new_socket;
	    ListPush(thread_job);
	    pthread_mutex_unlock(&main_mutex);
    }

    for (i = 0; i < thread_num; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            fprintf(stderr, "Failed to join the thread");
        }
    }
    pthread_mutex_destroy(&Listmutex);
    pthread_cond_destroy(&Listcond);
    return 0;
}

void sigTerminate(){
	time_t ltime;
	struct tm * timeinfo;
	time(&ltime);
	timeinfo = localtime ( &ltime );
	printf("%s SIGINT has been reveived. I handled a total of %d requests. Goodbye.\n", asctime(timeinfo),handled);
	int i;
	for(i=0;i<servantIndex;i++){
		kill(servantsArr[i].pid, SIGINT);
	}
}



void handler(int signal_number){
  
  if(signal_number == SIGINT){
  	gotSIGINTSig++;
    sigTerminate();
    exit(0);
  }
}


void ListPush(struct jobQueue req){

	/* Mutex locked*/
	pthread_mutex_lock(&Listmutex);
	tasksArr[reqCounter] = req;
	reqCounter = reqCounter + 1;
	/* Mutex unlocked*/
	pthread_mutex_unlock(&Listmutex);
	/*Signaling to inform a request has arrived*/
	pthread_cond_signal(&Listcond);

}


int compareStrings(char* city1, char* city2, char* city3){
	int low = strcmp(city1, city3);
	int high = strcmp(city2, city3);
	if(low <= 0 && high>=0){
		return 1;
	}
	else{
		return 0;
	}
}

void exit_gracefully(){
	int i;
	for(i=0;i<servantIndex;i++){
		kill(servantsArr[i].pid, SIGINT);
	}
	free(threadPool);
	exit(0);
}


int askServant(int pid, int port, char* query){
    int socket_val = 0, client_fd;
    struct sockaddr_in address_of_server;
    char buffer[1024] = { 0 };
    if ((socket_val = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket creation is failed\n");
        exit_gracefully();
    }
 
    address_of_server.sin_family = AF_INET;
    address_of_server.sin_port = htons(port);
 

    if (inet_pton(AF_INET, "127.0.0.1", &address_of_server.sin_addr) <= 0) {
        fprintf(stderr, "Socket address is invalid\n");
        exit_gracefully();
    }
 
    if ((client_fd = connect(socket_val, (struct sockaddr*)&address_of_server, sizeof(address_of_server))) < 0) {
        fprintf(stderr, "Connection to socket failed\n");
        exit_gracefully();
    }
    time_t ltime;
	struct tm * timeinfo;
	time(&ltime);
	timeinfo = localtime ( &ltime );
    printf("%s Contacting servant %d\n", asctime(timeinfo),pid);
    pthread_mutex_lock(&ask_mutex);
    send(socket_val, query, strlen(query), 0);
    pthread_mutex_unlock(&ask_mutex);
    read(socket_val, buffer, 1024);
    time(&ltime);
	timeinfo = localtime ( &ltime );
    printf("%s Response received: %s forwarded to client.\n", asctime(timeinfo),buffer);
 	
    return atoi(buffer);
}


void* clientReqHandler(void* tnum){
	int bytes=0;

	while(1){
		char buff[1025];
		pthread_mutex_lock(&Listmutex);
		while(reqCounter == 0){
			pthread_cond_wait(&Listcond, &Listmutex);
		}
		
		if(gotSIGINTSig==0){
			struct jobQueue taski = tasksArr[0];
			int i;
	        for (i = 0; i < reqCounter - 1; i++) {
	            tasksArr[i] = tasksArr[i + 1];
	        }
	        reqCounter--;
	        pthread_mutex_unlock(&Listmutex);
			if((bytes=read(taski.socketFd, buff, 1025)) < 0){
	       		fprintf(stderr, "Receive failed!\n");
	    	}
	    	buff[bytes]='\0';
	    	if(buff[0] == 'S'){
	    		const char *token;
				token = strtok(buff, "-");
				struct Servant serva;
				int line_index = 0;
				while(token!=NULL){
					switch(line_index){
						case 1: serva.pid = atoi(token); break;
						case 2: serva.port = atoi(token); break;
						case 3: strcpy(serva.start, token); break;
						case 4: strcpy(serva.end, token); break;
					}
					line_index++;
					token = strtok(NULL, "-");
				}
				pthread_mutex_lock(&servant_mutex);
				servantsArr[servantIndex] = serva;
				servantIndex++;
				pthread_mutex_unlock(&servant_mutex);
				time_t ltime;
    			struct tm * timeinfo;
    			time(&ltime);
				timeinfo = localtime ( &ltime );
				printf("%s Servant %d: present at port %d handling cities %s-%s\n", asctime(timeinfo),serva.pid, serva.port, serva.start, serva.end);
	    	}
	    	else{
	    		handled++;
				time_t ltime;
    			struct tm * timeinfo;
    			time(&ltime);
				timeinfo = localtime ( &ltime );	    		
	    		printf("%s Request arrived %s\n", asctime(timeinfo),buff);
	    		const char *token;
	    		char city[256];
	    		char seperate[1025];
	    		strcpy(seperate, buff);
	    		strcpy(city, ".");
				token = strtok(seperate, " ");
				int line_index = 0;
				while(token!=NULL){
					if(line_index == 4){
						strcpy(city, token);
					}
					line_index++;
					token = strtok(NULL, " ");
				}

				if(city[0] == '.'){
					time(&ltime);
					timeinfo = localtime ( &ltime );	
					printf("%s Contacting ALL servants\n", asctime(timeinfo));
					char result[1024];
					
					int z=0, found=0;
					for(z=0;z<servantIndex;z++){
						found = found + askServant(servantsArr[z].pid,servantsArr[z].port, buff);
					}
					sprintf(result, "%d", found);
					write(taski.socketFd, result, 1024);

				}
				else{
					int servant_to_ask;
					int z;
					int strFlag = 0;
					for(z=0;z<servantIndex;z++){
						int compRes = compareStrings(servantsArr[z].start, servantsArr[z].end, city);
						if(compRes == 1){
							strFlag = 1;
							servant_to_ask = z;
							break;
						}
					}
					if(strFlag == 1){
						int found = askServant(servantsArr[servant_to_ask].pid,servantsArr[servant_to_ask].port, buff);
						char result[1024];
						sprintf(result, "%d", found);
						write(taski.socketFd, result, 1024);
					}
					else{
						char result[1024];
						sprintf(result, "%d", -1);
						write(taski.socketFd, result, 1024);
					}

				}
				memset(city, 0, 256);
	    	}
	    	close(taski.socketFd);	
	        memset(buff, 0, 1025);
	    }
	    else{
	    	pthread_mutex_unlock(&Listmutex);
	    	exit_gracefully();
	    }
	}
}



