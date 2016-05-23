#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char * argv[]){
	
	if(argc>2)
		printf("Error!!");
	
	int fd = open(argv[1], O_RDONLY);
		
	sleep(3);
		
	close(fd);
	
	return 0;
}
