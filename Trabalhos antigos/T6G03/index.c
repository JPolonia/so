#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Program usage: %s <directory>", argv[0]);
		exit(1);
	}

	DIR* dirp;

	struct dirent* direntp;

	if ((dirp = opendir(argv[1])) == NULL)
	{
		perror(argv[1]);
		exit(1);
	}

	int returnValue;
	char* wordsPath = malloc(strlen(argv[1]) + 11);

	strcat(wordsPath, argv[1]);
	strcat(wordsPath, "/words.txt");

	FILE* words = fopen(wordsPath, "r");

	if (words == NULL)
	{
		exit(1);
	}


	while ((direntp = readdir(dirp)) != NULL)
	{

		if(strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) {
			continue;
		}

		char* fullPath = malloc(strlen(argv[1]) + strlen(direntp->d_name) + 2);

		strcat(fullPath, argv[1]);
		strcat(fullPath, "/");
		strcat(fullPath, direntp->d_name);

		if (fork() == 0)
		{
			execl("./sw", "./sw", argv[1], direntp->d_name, NULL);
			perror("sw");
			exit(1);
		}
		else
		{
			wait(&returnValue);

			if (WEXITSTATUS(returnValue) != 0)
			{
				printf("sw returned %d\n", WEXITSTATUS(returnValue));
				exit(1);
			}
		}
	}

	if (fork() == 0)
	{
		execlp("./csc", "./csc", argv[1], NULL);
		perror("csc");
		exit(1);
	}
	else
	{
		wait(&returnValue);

		if (WEXITSTATUS(returnValue) != 0)
		{
			printf("csc returned %d\n", WEXITSTATUS(returnValue));
			exit(1);
		}
	}

	closedir(dirp);
	fclose(words);

	return 0;
}





