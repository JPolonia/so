#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "ioctl.h"
#include <string.h>

int main(int argc, char *argv[]) {
    int devFile = 0;
    char *buff, t = 0, iot = 0, *str;
    int size = 0;
    int auxVar = 0;
    int control = 0;
    int i = 0;

    //Teste argumentos 
    if (argc != 2) {
        printf("Usage: %s <dev file>\n", argv[0]);
        exit(1);
    }

    buff = argv[1];
    //Open
    devFile = open(buff, O_RDWR & ~O_NONBLOCK);
    if (devFile == 0) {
        printf("Error!\n");
        exit(1);
    }
    printf("File open!\n");

    printf("Definir tamanho máximo das strings a serem lidas ou escritas:");
    scanf("%d", &size);
    str = (char *) calloc(size + 1, sizeof (char));

    printf("Que pretende testar: (w)rite), (r)ead, (i)octl ?\n");
    scanf(" %c", &t);


    switch (t) {
        case 'w'://Testa o write()
            printf("String a enviar:\n");
            scanf("%s", str);
            write(devFile, str, size);
            break;

        case 'r'://Testa o read()
            read(devFile, str, size);
            str[size] = '\0';
            printf("%s\n", str);
            break;


        case 'i'://Testa ioctl()
            printf("Apresentar estado (e), modificar LCR (l) ou modificar baudrate (r) ?\n");
            scanf(" %c", &iot);
            switch (iot) {
                case 'e':
                    ioctl(devFile, SERP_IOCGLCR);
                    break;

                case 'l':
                    printf("Valor para carregar no registo LCR?\n");
loop1:
                    printf("Word Length?\n (5, 6, 7, 8)");
                    scanf(" %d", &auxVar);
                    switch (auxVar) {
                        case 5:
                            control = 0;
                            break;

                        case 6:
                            control = 1;
                            break;

                        case 7:
                            control = 2;
                            break;

                        case 8:
                            control = 3;
                            break;

                        default:
                            printf("Bad input!\n");
                            goto loop1;
                            break;
                    }

loop2:
                    printf("No of stop bits?\n (1, 2)");
                    scanf(" %d", &auxVar);
                    switch (auxVar) {
                        case 1:
                            control |= 0 << 2;
                            break;

                        case 2:
                            control |= 1 << 2;
                            break;

                        default:
                            printf("Bad input!\n");
                            goto loop2;
                            break;
                    }

                    printf("Break enable? (0, 1)\n");
                    scanf(" %d", &auxVar);

                    if (auxVar == 1)
                        control = 1 << 6;

                    printf("Parity?\n (Odd, Even, 1, 0, None (default))");
                    scanf(" %s", str);

                    for (i = 0; i < strlen(str); i++)
                        str[i] = tolower(str[i]);

                    if (!strcmp(str, "odd"))
                        control |= 1 << 4;
                    else if (!strcmp(str, "even"))
                        control |= 3 << 4;
                    else if (!strcmp(str, "1"))
                        control |= 5 << 4;
                    else if (!strcmp(str, "0"))
                        control |= 7 << 4;
                    else {
                        control |= 0 << 4;
                        printf("None! (default)\n");
                    }


                    ioctl(devFile, SERP_IOCSLCR, control);
                    break;

                case 'r':
                    printf("Qual a nova baudrate que pretende definir?\n");
                    scanf("%d", &auxVar);
                    ioctl(devFile, SERP_IOCSBAUD, auxVar);
                    break;

                default:
                    return 1;
            }
            break;

        default:
            break;
    }


    //Close
    if (close(devFile) != 0) {
        printf("Error closing!\n.");
        exit(1);
    }

    printf("File closed!\n");
    return 0;
}
