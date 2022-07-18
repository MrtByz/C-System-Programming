#include <sys/stat.h>
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
#include "become_daemon.h"
//#include "tlpi_hdr.h"

int becomeDaemon(int flags){
	int fd;
	switch(fork()){
		case -1:return -1;
		case 0: break;
		default:_exit(EXIT_SUCCESS);
	}

	if(setsid() == -1)
		return -1;
	
	switch(fork()){
		case -1:return -1;
		case 0: break;
		default:_exit(EXIT_SUCCESS);	
	}


    if (!(flags & BD_NO_UMASK0))
        umask(0);                       


    if (!(flags & BD_NO_REOPEN_STD_FDS)) {
        close(STDIN_FILENO);            

        fd = open("/dev/null", O_RDWR);

        if (fd != STDIN_FILENO)         
            return -1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }

    return 0;
}
