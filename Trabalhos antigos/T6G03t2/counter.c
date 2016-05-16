#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h> 

#include "shm.h"
#include "log.h"

#define MAX_COUNTERS 10
#define FIFO_LENGTH 20
#define PERMISSIONS 0666
#define globalSem "/sem0"

//global vars

Store *s1;
int nrCounters, end=0, n;
char *shmName,*fifoId, *closeStore;
char *counterFifo;

//check close counters
int endLast(){
	int i;

	for(i=0; i < s1->numCounters; i++){
  		if (s1->counters[i].duration == -1 || s1->counters[i].inService > 0)
    		return 1;
	}
	
	return 0;
}

void alarmHandler(int signum)
{
	signal(SIGALRM, SIG_IGN);

	end=1;

s1->counters[nrCounters-1].duration=-2;

	sleep(1);

	close(n);

  	signal(SIGALRM, alarmHandler);
}

//thread function
void * thrfunc(void * arg)
{
	if (pthread_mutex_lock(&(s1->mut0)) != 0){
		perror("Locking mutex.\n");
		exit(1);
	}
	
	//number of clients plus 1, max=10
	int waitTime=s1->counters[nrCounters-1].inService+1;

	if (waitTime > 10)
		waitTime=10;
	
	s1->counters[nrCounters-1].inService++;

if (pthread_mutex_unlock(&(s1->mut0)) != 0){
		perror("Unlocking mutex.\n");
		exit(1);
	}

 	writeLog("Balcao", nrCounters, "inicia_atend_cli", (char *) arg);

	sleep(waitTime);

    writeLog("Balcao", nrCounters, "fim_atend_cli", (char *) arg);

	int fifoFd;
	
	if ((fifoFd=open((char *)arg,O_WRONLY)) == -1){
		perror("Open FIFO\n");
		exit(1);
	}

	const char msg[] = "fim_atendimento";

	//writes to fifo
	if (write(fifoFd,msg,strlen(msg)+1) == -1){
		perror("Writing to FIFO\n");
		exit(1);
	}

	if (pthread_mutex_lock(&(s1->mut0)) != 0){
		perror("Locking mutex.\n");
		exit(1);
	}

	s1->counters[nrCounters-1].inService--;
	s1->counters[nrCounters-1].attended++;
	s1->counters[nrCounters-1].avg+=waitTime;

	if (pthread_mutex_unlock(&(s1->mut0)) != 0){
		perror("Unlocking mutex.\n");
		exit(1);
	}

	close(fifoFd);
	unlink((char *) arg);

	return NULL;
}

int main(int argc, char* argv[])
{
	//verify the number of argv arguments
	if (argc != 3){
		perror("Invalid number of arguments.\n");
		printf("Usage: %s <sharedmemory_name> <opening_time>\n", argv[0]);
		exit(1);
	}

	//verify user entry
	if (atoi(argv[2]) <= 0 || strchr(argv[1], '/') == NULL){
		perror("Invalid arguments.\n");
		exit(1);
	}

	shmName = malloc(100);
	strcpy(shmName,argv[1]);

	//inits alarm
	signal(SIGALRM, alarmHandler);
	alarm(atoi(argv[2]));
	int creator=0;
	//semaphore
	sem_t *sem;
	if ((sem=sem_open(globalSem,O_RDWR|O_CREAT|O_EXCL,PERMISSIONS,1)) == SEM_FAILED){
		sem=sem_open(globalSem,O_RDWR);
	}
	int shmd;

	sem_wait(sem);

	//verify if shared memory exist, if not, creates one
	if ((shmd = shm_open(argv[1],O_CREAT|O_RDWR|O_EXCL, PERMISSIONS)) == -1){
		shmd=shm_open(argv[1],O_RDWR,PERMISSIONS);
	} else{
		
		creator=1;

		initLog();
		writeLog("Balcao", 1, "inicia_mempart", "-");

		if(ftruncate(shmd,sizeof(Store)) != 0){
			perror("Truncating memory.\n");
			exit(1);
		}
		
	}


	//inits shared memory
	s1=(Store *)mmap(NULL,sizeof(Store),PROT_READ|PROT_WRITE,MAP_SHARED,shmd,0);
	
	if (creator){

		//init global vars
		s1->openingTime=(unsigned long) time(NULL);
		s1->numCounters=0;
		pthread_mutexattr_t mattr;
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(&(s1->mut0),&mattr);
	}	

	if (s1->numCounters < MAX_COUNTERS){
		nrCounters=s1->numCounters++;
		nrCounters++;
	}
	else{
		perror("Max number of counters [10]\n");
		exit(1);
	}

	//creates attendement FIFO
	counterFifo = malloc(FIFO_LENGTH);

	//fills counterFifo
	sprintf(counterFifo,"/tmp/fb_%d",getpid());

	//creates fifofo

	if (mkfifo(counterFifo,PERMISSIONS) == -1){
		perror("Creating FIFO\n");
		exit(1);
 	 }

	if (pthread_mutex_lock(&(s1->mut0)) != 0){
		perror("Locking mutex.\n");
		exit(1);
	}

	//inits counters[nrCounters]

	writeLog("Balcao", nrCounters, "cria_linh_mempart", counterFifo);

	s1->counters[nrCounters-1].number=nrCounters;
	s1->counters[nrCounters-1].openingTime=atoi(argv[2]);
	s1->counters[nrCounters-1].duration=-1;
	strcpy(s1->counters[nrCounters-1].fifoName,counterFifo);
	s1->counters[nrCounters-1].inService=0;
	s1->counters[nrCounters-1].attended=0;
	s1->counters[nrCounters-1].avg=0;

	if (pthread_mutex_unlock(&(s1->mut0)) != 0){
		perror("Unlocking mutex.\n");
		exit(1);
	}

	sem_post(sem);

	//main thread
	pthread_t core;

	int *ret=0, fifoFd;

	if ((fifoFd=open(counterFifo,O_RDONLY)) == -1){
			perror("Opening FIFO.\n");
			exit(1);
		}

closeStore= malloc(FIFO_LENGTH);

	while(!end){
		
		fifoId = malloc(FIFO_LENGTH);
		
		if ((n=read(fifoFd,fifoId,FIFO_LENGTH)) > 0){
			
		strcpy(closeStore,fifoId);

			//create thread
			if (pthread_create(&core, NULL, thrfunc, fifoId) != 0){
				perror("Creating thread.\n");
				exit(1);
			}
		
		}else if (n < 0) {
			perror("Reading FIFO.\n");
			exit(1);
		}
		}

			if (pthread_join(core,(void *) ret) != 0){
					perror("Waiting thread.\n");
					exit(1);
			}


			//close counter
  	//s1->counters[nrCounters-1].duration=-2;

  	//check if counter had clients
  	if (s1->counters[nrCounters-1].attended == 0){
  		s1->counters[nrCounters-1].avg=0;
  		writeLog("Balcao", nrCounters, "fecha_balcao", "-");
  	}
  	else {
  		s1->counters[nrCounters-1].avg/=s1->counters[nrCounters-1].attended;
  		writeLog("Balcao", nrCounters, "fecha_balcao", closeStore);
  		unlink(counterFifo);
  	}

  	if (!endLast()){

			int totalClients=0;
			double totalAvg=0;
			int totalTime=0;

			int i=0;

			printf("\n\nStore statistics:\n\n");

			for (i=0; i < s1->numCounters; i++){
				totalClients+=s1->counters[i].attended;
				totalAvg+=s1->counters[i].avg;
				totalTime+=s1->counters[i].openingTime;

				printf("Counter %d total clients: %d\n",i+1,s1->counters[i].attended);
				printf("Counter %d total time: %lu\n",i+1,s1->counters[i].openingTime);
				printf("Counter %d avg per client: %f\n\n",i+1,s1->counters[i].avg);
			}
				printf("\nStore total time: %d\n",totalTime);
				printf("Store total clients: %d\n",totalClients);
				printf("Store avg per client: %f\n",totalAvg/s1->numCounters);

				writeLog("Balcao", nrCounters, "fecha_loja", closeStore);

		}

    if (shm_unlink(shmName) < 0){
    	//perror("Unlink shared memmory\n");
   		exit(1);
	}
	unlink(counterFifo);

		//close(fifoFd);
		free(counterFifo);
		free(shmName);
		sem_unlink(globalSem);
		
		return 0;
}