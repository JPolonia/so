#include <stdio.h>
#include <pthread.h>

int global_1,global_2;

void *global(void *arg){
	printf("%d + %d = %d\n",global_1,global_2,global_1+global_2);
	return NULL;
}

void *arg(void *arg){
	printf("%d + %d = %d\n",global_1,global_2,global_1+global_2);
	return NULL;
}

int main(int argc, char *argv[]){
	pthread_t tid_global;
	pthread_t tid_arg;
  global_1 = 3;
  global_2 = 7;
  printf("hello world (pid:%d)\n", (int) getpid());
    
  pthread_create(&tid_global,NULL,(void *) global,NULL);
  pthread_join(tid_global,NULL);
  
  pthread_create(&tid_arg,NULL,(void *) arg,NULL);
  pthread_join(tid_arg,NULL);
  
  pthread_exit(0);
}

