#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

int main(int x ,char *y[] ){

int f,i=0,l=0,k;
char write_buf[100];
char op;
char *cop;

f=open(y[1], O_RDWR);
if(f==-1)
    printf("Erro1\n");
printf("'w' to write, 'r' to read\n");
scanf("%c", &op);

switch (op) {


  case 'w':{
    printf ("What to write\n");
    scanf ("%s", write_buf);

    i=strlen(write_buf);

    cop=malloc(sizeof(char)*i);

    while (write_buf[l] != '\0'){
    cop[l]=write_buf[l];
    l++;
    }
  
    for (k=l;k<i;k++){
    cop[k]='\0';  
    }

    write(f, cop, i);
    free(cop);
    break;
  } 
  case 'r':  {
    i=read(f,write_buf,sizeof(write_buf));
    break;
  }  

} 

close(f);


return 0;
}
