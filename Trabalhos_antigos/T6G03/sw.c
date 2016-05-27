#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUF_LENGTH 4096

#define READ 0

#define WRITE 1

int main(int argc, char* argv[])
{
	//verify the number of argv arguments
	if (argc != 3) 
	{
		printf("Invalid number of arguments.\n");
		printf("Usage: %s <directory> <filename>\n", argv[0]);
		exit(1);
	}

	//fills wordsPath with path
	char wordsPath[BUF_LENGTH];
	sprintf(wordsPath, "%s/words.txt", argv[1]);

	//Inits searchIndex pointer
	char *searchIndex = malloc(strlen(argv[2]) + 1);

	if(searchIndex == NULL){
		exit(1);
	}

	//copy to searchIndex the name of file to search
	strcpy(searchIndex, argv[2]);

	//Pointer to first '.' in searchIndex else NULL
	char *dot = strrchr(searchIndex,'.');
	//whatever the case, changes char to terminator char, removes .txt
	*dot = '\0';


	FILE *src;

	//Open words' to search file only for read
	if ((src = fopen(wordsPath, "r")) == NULL)
	{
		printf("ERROR, words.txt not found at %s", wordsPath);
		exit(1);
	}

	//fill outFile with tmp+searchIndex
	char *outFile = malloc(strlen(argv[2]) + 1);
	strcat(outFile,"tmp");
	strcat(outFile, searchIndex);

	FILE * dest;

	//Create and open outFile for write
	if ((dest = fopen(outFile, "w")) == NULL)
	{
		printf("ERROR, %s not found.", outFile);
		exit(1);
	}

	//file descriptor
	int fd[2];

	if (pipe(fd) != 0)
	{
		printf("Error creating pipe.");
		exit(1);
	}

	pid_t pid = fork();

	//Father
	if (pid > 0) 
	{

		close(fd[WRITE]);
		wait(NULL);

		char word[BUF_LENGTH];
		char buffer[BUF_LENGTH];
		char str[BUF_LENGTH];
		
		int lineNumber;
		int nr;
		int n=0;
		int it=0;

		if ((nr = read(fd[READ], &buffer, BUF_LENGTH)) < 0)
		{
			printf("Error, reading fd");
			exit(1);
		}
			//copy buffer to str
			strcpy(str, buffer);

			//counts number of words found
			while(str[it] != '\0'){
				if (str[it] == '\n')
					n++;
				it++;
			}
			
			char *tok = strtok(str, "\n");

			while(n > 0){
				//Verify format and rewrite
				if (sscanf(tok, "%d:%s", &lineNumber, word) > 0)
					fprintf(dest, "%s: %s-%d\n", word, searchIndex, lineNumber);
				//next word
				tok = strtok(NULL, "\n");
				n--;
			}


		close(fd[READ]);
		wait(NULL);
	}
	else //son
	{
		close(fd[READ]);
		//Write grep output into file
		dup2(fd[WRITE], STDOUT_FILENO);
		execlp("grep", "grep",  "-w", "-o", "-n", "-f",wordsPath,argv[2], NULL);


	}

	fclose(src);
	fclose(dest);
	exit(0);
}












