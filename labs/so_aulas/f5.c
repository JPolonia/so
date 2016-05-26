#include <stdio.h>
#include "mythreads.h"
#include <stdlib.h>
#include <pthread.h>

#define DEBUG 0

typedef struct {
    long *cnt;	/* pointer to shared counter */
    int n;		/* no of times to increment */
    int id;		/* application-specific thread-id */
} targ_t;

void *tfun(void *arg) {

    targ_t *targ = (targ_t *) arg;
    int i;
  
    printf("Thread %d starting\n", targ->id);

    printf ("&cnt = %p \n", targ->cnt);

    for(i = 0; i < targ->n ; i++) {
		if( DEBUG ) 
		    printf("b4:  %ld ", *(targ->cnt)); 
	
		(*(targ->cnt))++; 
		/* If cnt is always N*MAX_ITER 
		   Then comment the previous line and 
		   uncomment the following 3 lines 
		   aux = *(targ->cnt);
		   aux++;
		   *(targ->cnt) = aux; 
		   */
		if (DEBUG )
		    printf("\t %ld \n", *(targ->cnt)); 
    }
    printf("Thread %d done\n", targ->id);
    
    return NULL;
}

int main(int argc, char *argv[]){
	pthread_t tid,t2,t3;
	long cnt = 0;
	
	targ_t targ1;
	targ1.cnt = &cnt;
	targ1.n = 10000000;
	targ1.id = 1; 
	
	targ_t targ2;
	targ2.cnt = &cnt;
	targ2.n = 10000000;
	targ2.id = 2;
	
	targ_t targ3;
	targ3.cnt = &cnt;
	targ3.n = 10000000;
	targ3.id = 3; 
	
  
  printf("hello world (pid:%d)\n", (int) getpid());
    
  pthread_create(&tid,NULL,(void *) tfun,&targ1);
  
  
  pthread_create(&t2,NULL,(void *) tfun,&targ2);
  
  
  pthread_create(&t3,NULL,(void *) tfun,&targ3);
  pthread_join(tid,NULL);
  pthread_join(t2,NULL);
  pthread_join(t3,NULL);
    
  printf ("cnt = %ld \n", cnt);
  pthread_exit(0);
  
  
  
}

