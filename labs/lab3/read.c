#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 30

#define READ_SIZE 10
int main(int argc, char **argv) {
  
  int fd, i;
  char buffer[BUF_SIZE];
  int index = 1; 
  
  setbuf(stdout, NULL);
  
  if (argc != 2) {
    
    printf("Error passing arguments\n");
    exit(1);
  }
  
  fd = open(argv[1], O_RDWR);
  strcpy(buffer,"A funcao write funciona\n");
  if (fd < 0) {
    printf("Error opening file\n");
    exit(2);
  }
	while(index){
		index = read(fd, buffer, READ_SIZE);
		buffer[index]='\0'; 
		printf("%s",buffer);
	}
  printf("\nA funcao read funciona\n");
  close(fd);
  
  return 0;
}
