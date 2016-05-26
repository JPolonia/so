#include <stdio.h>


int main(int argc, char *argv[])
{
    printf("hello world (pid:%d)\n", (int) getpid());
    
    return 0;
}

