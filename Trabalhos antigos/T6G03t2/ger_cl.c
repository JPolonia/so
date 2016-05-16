#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "shm.h"
#include "log.h"

#define MAX_COUNTERS 10
#define FIFO_LENGTH 20
#define PERMISSIONS 0666
#define MAX_VALUE 999

//global var
 Store * s1;

//Reading function from slides
int readline(int fd,char *str){

	int n;

	do{
		n=read(fd,str,1);
	}
	while (n>0 && *str++ != '\0');

	return (n>0);
}

void chooseCounter(){

  //creates private FIFO
  char *privateFifo = malloc(FIFO_LENGTH);

  //fills clientFifo
  sprintf(privateFifo,"/tmp/fc_%d",getpid());

  //creates fifo
  if (mkfifo(privateFifo,PERMISSIONS) == -1){
    perror("Creating FIFO.\n");
    exit(1);
  }

  //name of the counter fifo which is open and with less clients
  char *counterFifo = malloc(FIFO_LENGTH);
  
  int numClients=MAX_VALUE;

 if (pthread_mutex_lock(&(s1->mut0)) != 0){
    perror("Locking mutex.\n");
    exit(1);
  }
  
  int i, fifoFd, counter=0;

  for(i = 0; i < s1->numCounters; i++)
    if(s1->counters[i].duration == -1){
      if(s1->counters[i].inService < numClients){
        numClients = s1->counters[i].inService;
        counter=i+1;
        strcpy(counterFifo,s1->counters[i].fifoName);
       
    }
    }

    if (pthread_mutex_unlock(&(s1->mut0)) != 0){
      perror("Unocking mutex.\n");
      exit(1);
  }
  
    //open fifo for write
   if ((fifoFd=open(counterFifo,O_WRONLY)) == -1){
        perror("Opening FIFO.\n");
        exit(1); 
    }

    //writes to fifo
    if (write(fifoFd,privateFifo,FIFO_LENGTH) == -1){
      perror("Writing FIFO.\n");
      exit(1);
    }

    writeLog("Client", counter, "pede_atendimento", privateFifo);

    close(fifoFd);



    int fifoFd1;
    char str[FIFO_LENGTH];

    //open fifo for read
    if ((fifoFd1=open(privateFifo,O_RDONLY)) == -1){
     perror("Opening FIFO.\n");
     exit(1); 
    }


    while(readline(fifoFd1,str))

    close(fifoFd1);

    writeLog("Client", counter, "fim_atendimento", privateFifo);

    unlink(privateFifo);

  return;
}


int main(int argc, char* argv[]) {

  //verify the number of argv arguments
  if (argc != 3){
    perror("Invalid number of arguments.\n");
     printf("Usage: %s <nome_mempartilhada> <num_clientes> \n",argv[0]);
    exit(1);
  }

  int fd;

  //open shared memory
  if ((fd = shm_open(argv[1], O_RDWR, PERMISSIONS)) ==  -1) {
    perror("Opening shared memory.\n");
    exit(1);
  }

  s1=(Store *)mmap(NULL,sizeof(Store),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

  int pid, i;

  for(i = 0; i < atoi(argv[2]); i++){

      pid = fork();

      if(pid == 0){
          chooseCounter();
        return 0;

      }else if(pid < 0){
          perror("Clients fork.\n");
          return 1;
      }
  }

  return 0;
}
