#ifndef SHM_H
#define SHM_H

//counter struct
typedef struct{
  int number;
  unsigned long openingTime;
  long duration;
  char fifoName[20];
  int inService;
  int attended;
  double avg;
}Counter;

//store struct
typedef struct{
  unsigned long openingTime;
  int numCounters;
  pthread_mutex_t mut0;
  Counter counters[10];
}Store;


#endif
