#ifndef LOG_H
#define LOG_H

#include <time.h>
#include <stdio.h>
#include <string.h>

void initLog()
{
    const char fileName[] = "shm.log";

    /* Open file in append mode */
    FILE *file;
    file = fopen(fileName, "w");

    fprintf(file, "%-25s | %-7s | %-6s | %-25s | %-18s \n", "quando", "quem", "balcao", "o_que", "canal_criado/usado");
    int i;
    for(i=0; i < 93; i++)
        fprintf(file, "-");
    fprintf(file, "\n");

    fclose(file);
}

void writeLog(char *who, int counter, char *msg, char *fifoName)
{

    const char fileName[] = "shm.log";

    FILE *file;

    if ((file = fopen(fileName, "a")) == NULL)
    {
        perror("Opening file.\n");
        exit(1);
    }

    //system time struct
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    fprintf(file,"%d-%d-%d %d:%d:%d\t\t ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(file, " | %-7s | %-6d | %-25s | %-18s \n", who, counter, msg, fifoName);

    fclose(file);
}

#endif /* LOG_H */
