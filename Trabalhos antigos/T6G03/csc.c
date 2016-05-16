#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#define MAXLINE 4096
#define READ 0
#define WRITE 1

int main(int argc, char *argv[]){
	if(argc != 2){
		printf("Usage: %s <directory>\n", argv[0]);
	}

	setbuf(stdout, NULL);

	FILE *fp1, *fp2, *fIndex;

	int pid, status, numForks;

	char *word, *num;

	char buf[BUFSIZ],
	     line[MAXLINE],
	     previousWord[MAXLINE];

	//Open directory
	DIR *d;
	struct dirent *dir;

	//Create pipe
	int fd[2];
	pipe(fd);

	//Create tempory cat.txt with the concatenated text
	fp1 = fopen("cat.txt", "w");

	//Concatenate tmp files to cat.txt
	d = opendir(argv[1]);
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
				continue;
			}

			//Search directory for tmp files
			if(strncmp(dir->d_name, "tmp", 3) == 0){
				fp2 = fopen(dir->d_name, "r");
				fseek(fp2, 0, SEEK_END);
				int len = ftell(fp2);

				if (len == 0)
					continue;

				numForks++;
				pid = fork();

				if (pid == 0){
					dup2(fd[WRITE],STDOUT_FILENO);
					close(fd[READ]);
					execlp("cat", "cat", dir->d_name, NULL);
					fprintf(stderr, "ERROR: cat didn't execute\n");
					exit(2);
				}
				else{	
					int n = read(fd[READ],line,MAXLINE);
					fwrite(line, 1, n, fp1);
				}
			}
		}
		close(fd[READ]);
		close(fd[WRITE]);
	}
	fclose(fp1);
	closedir(d);

	//Wait for forks to end
	while(numForks > 0){
		wait(&status);
		numForks--;
	};


	pid = fork();

	//Sort and clean
	if (pid == 0) 
	{	//Child -> Sort cat.txt
		execlp("sort", "sort", "-t:", "-k1,1d", "-k2,2V", "cat.txt", "-o", "cat.txt", NULL);
	}
	else	
	{	//Parent -> Clean and copy to index.txt 
		waitpid(-1, &status, 0);

		fIndex = fopen ("index.txt", "w");
		fp1 = fopen("cat.txt", "r");

		fprintf(fIndex, "INDEX\n");

		while (fgets (buf, sizeof(buf), fp1) != NULL)
		{
			if (buf[strlen(buf)-1] == '\n') 
			{
				strcpy(line, buf);
				//Get word
				word = strtok(line, ":");

				strcpy(line, buf);
				//Get number of chapter and line
				num = strtok(line, ":");
				num = strtok(NULL, "\n");

				//Check if it's a new word
				if (strcmp(word, previousWord) == 0) 
					fprintf(fIndex, ",%s", num);
				else 
				{
					strcpy(previousWord, word);
					fprintf(fIndex, "\n\n%s: %s", word, num);
				}
			}
		}
		fclose(fIndex);
		fclose(fp1);
	}

	//Delete temporary files
	remove("cat.txt");

	d = opendir(argv[1]);
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
				continue;
			}
			//Search directory for tmp files
			if(strncmp(dir->d_name, "tmp", 3) == 0){
				remove(dir->d_name);
			}
		}
	}

	return 0;
}













