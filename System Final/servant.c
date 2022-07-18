#include "my_servant.h"


int main(int argc, char* argv[]) {

	struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handler;
    sigaction(SIGINT, &sa, NULL);

	/* VARIABLES FOR OPTARG */
	int opt,ip_flag=0, port_flag=0, dir_flag=0, range_flag=0;
	char dirPath[1024];
	char range[50];
	

    /* TAKING ARGUMENTS AND ASSIGNING TO VARIABLES */
    while((opt = getopt(argc, argv, ":d:c:r:p:")) != -1)  
    {  
        switch(opt)  
        {  
        	case 'd':
            	sprintf(dirPath,"%s",optarg);
            	dir_flag=1;
            	break;

            case 'c': 
            	sprintf(range,"%s",optarg);
            	range_flag=1;
            	break;

            case 'r': 
            	sprintf(ip_name,"%s",optarg);
            	ip_flag=1;
            	break;

            case 'p': 
            	port_num = atoi(optarg);
            	port_flag=1;
            	break;
        } 
    }
    
    /* CHECKING THE ARGUMENTS */
    if(dir_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(port_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(range_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	if(ip_flag==0){
		fprintf(stderr, "Usage : Missing argument\n");
		return -1;      
	}
	
	pid = pid_from_proc();

	read_directories(dirPath);

	find_range(range);

	responsible_dirs = end_index - start_index + 1;

	read_Records(dirPath);


	printf("Servant %d: loaded dataset, cities %s-%s\n", pid, directories[start_index-1], directories[end_index-1]);

	struct sockaddr_in sin;
	int port = 0;
	int server_fd, new_socket;
	int option = 1;
    int addrlen = sizeof(sin);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fprintf(stderr, "socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
        fprintf(stderr, "setsockopt");
        exit(EXIT_FAILURE);
    }

	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = 0;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_family = AF_INET;

	if (bind(server_fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "bind failed");
        exit(EXIT_FAILURE);
    }

	socklen_t len = sizeof(sin);
	if (getsockname(server_fd, (struct sockaddr *)&sin, &len) != -1)
		printf("Servant %d: listening at port %d\n", pid, ntohs(sin.sin_port));

	int dedicated_port = ntohs(sin.sin_port);
	char info_to_server[1024];
	sprintf(info_to_server, "S-%d-%d-%s-%s", pid, dedicated_port, directories[start_index-1], directories[end_index-1]);
	passInfoServer(info_to_server);

	if (listen(server_fd, 20) < 0) {
        fprintf(stderr, "listen");
        exit(EXIT_FAILURE);
    }

    while ((new_socket = accept(server_fd, (struct sockaddr*)&sin, (socklen_t*)&addrlen)) >= 0){
    	pthread_mutex_lock(&main_mutex);
		pthread_t th;
		int *newsock = malloc(sizeof(int));
    	*newsock = new_socket;

		if(pthread_create(&th, NULL, getResults, newsock)!=0){
			fprintf(stderr, "Error in thread creation\n");
			exit(EXIT_FAILURE);
		}
		handled++;

		pthread_mutex_unlock(&main_mutex);
    }
	
	free_allocated();

    return 0;
}

void init_map_item(map item){
	item.index = 0;
	strcpy(item.city, ".");
}

void init_Record(record item){
	item.id = 0;
	item.field_size = 0;
	item.price = 0;
	item.day = 1;
	item.month = 1;
	item.year = 2000;
}

int is_city_exist(char* citi){
	int i;
	for(i=0;i<recordCount;i++){
		if(strcmp(citi, recordsList[i].city) == 0){
			return 1;
		}
	}
	return -1;
}

void printRecords(){
	int i;
	for(i=0;i<mapCount;i++){
		printf("City:%s Index:%d\n", mapList[i].city, mapList[i].index);
	}
}

pid_t pid_from_proc(){
	char result[32];
	char result2[32];
    pid_t pid;
    pid_t pid2;
    ssize_t length;
    if((length = readlink ("/proc/self", result, sizeof (result))) != -1){
    	sscanf (result, "%d", &pid);
    }
    else{
    	pid = 0;
    }
    if((length = readlink ("/proc/self", result2, sizeof (result2))) != -1){
    	sscanf (result2, "%d", &pid2);
    }
    else{
    	pid2 = 0;
    }
    if(pid == pid2){
    	return pid;
    }
    else if(pid<pid2){
    	return pid;
    }
    else if(pid2<pid){
    	return pid2;
    }
    return 0;
}

int get_line_count(char filename[1024]){
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

    return line_count;
}



void allocate_space(int size){
	int i;
	directories = (char**)malloc(size * sizeof(char*));
    for (i = 0; i < size; i++)
        directories[i] = (char*)malloc(256 * sizeof(char));
}


void free_allocated(){
	int i;
	for (i = 0; i < dir_size; i++)
        free(directories[i]);
 
    free(directories);
}


void store_Records(char* filename, char* date, char* city){
	int lines = get_line_count(filename);
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    if(recordCount == 0){
		recordsList = (record*)malloc(lines * sizeof(record));
		recordCount = recordCount + lines;
	}
	else{
		recordCount = recordCount + lines;
		recordsList = (record*)realloc(recordsList, sizeof(record)*recordCount);
	}

    while ((read = getline(&line, &len, fp)) != -1) {
        if(read != 1){
        	/* CREATING ITEM FOR EACH LINE */
        	record item;
        	init_Record(item);

        	strcpy(item.city, city);
        	/* PARSING DAY MONTH YEAR */
        	const char *num;
			num = strtok(date, "-");
			item.day = atoi(num);
			int date_index = 0;
			while(num!=NULL){
				if(date_index == 1)
					item.month = atoi(num);
				else if(date_index == 2){
					item.year = atoi(num);
				}
				date_index++;
				num = strtok(NULL,"-");
			}
			/* PARSING OTHER FIELDS */
			const char *token;
			token = strtok(line, " ");
			item.id = atoi(token);
			int line_index = 0;
			while(token!=NULL){
				switch(line_index){
					case 1: strcpy(item.type, token); break;
					case 2: strcpy(item.province, token); break;
					case 3: item.field_size = atoi(token); break;
					case 4: item.price = atoi(token); break;
				}
				// printf("%s\n", token);
				line_index++;
				token = strtok(NULL, " ");
			}
			recordsList[recIndex] = item;
			recIndex++;
        } 
    }
    fclose(fp);
}


void read_Records(char* dirPath){
	int i;
	char subDir[1024];
	for(i=start_index-1;i<end_index;i++){
		strcpy(subDir,dirPath);
		strcat(subDir, "/");
		strcat(subDir, directories[i]);
		map item;
		init_map_item(item);
		strcpy(item.city, directories[i]);
		item.index = recordCount;
		mapList[mapCount] = item;
		mapCount++;
		struct dirent **file_list;
		int number_of_files, j;
		number_of_files = scandir(subDir, &file_list, 0, alphasort);
		for(j=0; j<number_of_files;j++){
			char recFile[1024];
			if(strcmp(file_list[j]->d_name,".") != 0 && strcmp(file_list[j]->d_name,"..") != 0){
				strcpy(recFile, subDir);
				strcat(recFile, "/");
				strcat(recFile, file_list[j]->d_name);
				store_Records(recFile, file_list[j]->d_name, directories[i]);
				memset(recFile, 0, 1024);
			}
		}
		memset(subDir, 0, 1024);
	}
}

void read_directories(char* dirPath){
	struct dirent **file_list;
	int number_of_files;
	int i;
	int index = 0;
	number_of_files = scandir(dirPath, &file_list, 0, alphasort);
	dir_size = number_of_files-2;

	allocate_space(number_of_files-2);
	if (number_of_files < 0)
	   fprintf(stderr, "scandir");
	else {
		for(i=0;i<number_of_files;i++){
			if(strcmp(file_list[i]->d_name,".") != 0 && strcmp(file_list[i]->d_name,"..") != 0){
				strcpy(directories[index], file_list[i]->d_name);
				index++;
			}
		}
		
		free(file_list);
	}
}


void find_range(char* rangestr){
	const char *num;
	num = strtok(rangestr, "-");
	start_index = atoi(num);
	while(num!=NULL){
		end_index = atoi(num);
		num = strtok(NULL,"-");
	}
}

void* passInfoServer(char* infoline){
	int socket_val = 0, client_fd;
    struct sockaddr_in address_of_server;

	if ((socket_val = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket creation is failed\n");
        return NULL;
    }

    address_of_server.sin_family = AF_INET;
    address_of_server.sin_port = htons(port_num);

	if (inet_pton(AF_INET, ip_name, &address_of_server.sin_addr) <= 0) {
        fprintf(stderr, "Address of socket is invalid\n");
        return NULL;
    }

    if ((client_fd = connect(socket_val, (struct sockaddr*)&address_of_server, sizeof(address_of_server))) < 0) {
        fprintf(stderr, "Connection to socket is failed\n");
        return NULL;
    }

    send(socket_val, infoline, strlen(infoline), 0);
    return NULL;
}

void splitDate(char* date, int* d, int* m, int* y){
	const char *num;
	num = strtok(date, "-");
	*d = atoi(num);
	int date_index = 0;
	while(num!=NULL){
		if(date_index == 1)
			*m = atoi(num);
		else if(date_index == 2){
			*y = atoi(num);
		}
		date_index++;
		num = strtok(NULL,"-");
	}
}


int find_index(char* citi){
	int i;
	for(i=0;i<mapCount;i++){
		if(strcmp(mapList[i].city, citi) == 0){
			return mapList[i].index;
		}
	}
	return -1;
}


void* getResults(void* requ){
	pthread_mutex_lock(&servant_mutex);
	int tj = *(int*)requ;
	int bytes = 0;
	char request[1025];
	if((bytes=read(tj, request, 1025)) < 0){
   		fprintf(stderr, "Receive failed!\n");
	}
	pthread_mutex_unlock(&servant_mutex);
	int d1,d2,m1,m2,y1,y2;
	int found_records = 0;
	char tip[256];
	char date1[256];
	char date2[256];
	char city[256];
	char line[1024];
	strcpy(city, ".");
	strcpy(line,request);
	const char *token;
	token = strtok(line, " ");
	int line_index = 0;
	while(token!=NULL){

		switch(line_index){
			case 1: strcpy(tip, token); break;
			case 2: strcpy(date1, token); break;
			case 3: strcpy(date2, token); break;
			case 4: strcpy(city, token); break;
		}
		line_index++;
		token = strtok(NULL, " ");
	}

	splitDate(date1, &d1,&m1,&y1);
	splitDate(date2, &d2,&m2,&y2);

	if(strlen(city) < 3){
		strcpy(city, ".");
	}

	if(city[0] == '.'){
		int i;
		
		for(i=0;i<recordCount;i++){
			if((strcmp(recordsList[i].type, tip) == 0)){
				if(recordsList[i].year > y1 && recordsList[i].year < y2){
					found_records++;
				}
				else if(recordsList[i].year == y1){
					if(recordsList[i].month > m1){
						found_records++;
					}
					else if(recordsList[i].month == m1){
						if(recordsList[i].day >= d1){
							found_records++;
						}
					}
				}
				else if(recordsList[i].year == y2){
					if(recordsList[i].month < m2){
						found_records++;
					}
					else if(recordsList[i].month == m2){
						if(recordsList[i].day <= d2){
							found_records++;
						}
					}
				}
			}
		}
		char result[1024];
		sprintf(result, "%d", found_records);
		write(tj, result, 1024);
			
		// closing the connected socket
	    close(tj);
	}
	else{
		/*Search City*/
		int s_index = find_index(city);
		if(s_index != -1){
			int i;
			for(i=s_index;i<recordCount;i++){
				if((strcmp(recordsList[i].city, city) == 0) && (strcmp(recordsList[i].type, tip) == 0)){
					if(recordsList[i].year > y1 && recordsList[i].year < y2){
						found_records++;
					}
					else if(recordsList[i].year == y1){
						if(recordsList[i].month > m1){
							found_records++;
						}
						else if(recordsList[i].month == m1){
							if(recordsList[i].day >= d1){
								found_records++;
							}
						}
					}
					else if(recordsList[i].year == y2){
						if(recordsList[i].month < m2){
							found_records++;
						}
						else if(recordsList[i].month == m2){
							if(recordsList[i].day <= d2){
								found_records++;
							}
						}
					}
				}
			}
			char result[1024];
			sprintf(result, "%d", found_records);
			write(tj, result, 1024);
				
			// closing the connected socket
		    close(tj);

		}
		else{
			char result[1024];
			sprintf(result, "%d", -1);
			write(tj, result, 1024);
				
			// closing the connected socket
		    close(tj);
		}

	}
	return NULL;
}


void signalTerminate(){
	free_allocated();
	free(recordsList);
	printf("Servant %d: termination message has been received. I handled a total of %d requests.Goodbye.\n", pid, handled);
}

void handler(int signal_number){
  
  if(signal_number == SIGINT){
  	signalTerminate();
    exit(0);
  }
}

