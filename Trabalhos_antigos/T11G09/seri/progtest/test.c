#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 100

int main(int argc, char **argv) {
  
  int fd, i;
  char buffer[BUF_SIZE];
  
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
	for(i=0;i<5;i++){
		read(fd, buffer, 30);
		buffer[30]='\0';
		printf("Recevido: %s \n",buffer);
	}/*
	write(fd,buffer,24);
	scanf("%d",&i);
	write(fd,buffer,24);
	 
	strcpy(buffer,"funcao de teste write com mais de 65 caracteres completa com sucesso! \n");
	write(fd,buffer,71);
	scanf("%d",&i);
	write(fd,buffer,71);
	scanf("%d",&i);
*/

  close(fd);
  
  return 0;
}

