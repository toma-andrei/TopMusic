#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#define QUIT "quit message"

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server;

    if (argc != 3)
    {
        printf("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        exit(errno);
    }

    int port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[client] Eroare la creare socket!\n");
        exit(errno);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client] Erorare la connect.\n");
        exit(errno);
    }

    int connected = 1;
    while (connected)
    {
        printf("Type a command: ");
        fflush(stdout);

        char comanda[1500];
        bzero(&comanda, 1500);

        scanf("%[^\n]%*c", comanda);
        fflush(stdin);

        int lenComanda = strlen(comanda);

        if (lenComanda <= 0)
        {
            break;
        }

        if (write(sd, &lenComanda, sizeof(int)) <= 0)
        {
            perror("[client] Eroare la scrierea dimensiunii comenzii catre server!\n");
            exit(errno);
        }

        if (write(sd, comanda, lenComanda) <= 0)
        {
            perror("[client] Eroare la scrierea comenzii catre server!\n");
            exit(errno);
        }

        int lenRaspuns = 0;

        if (read(sd, &lenRaspuns, sizeof(int)) < 0)
        {
            perror("[client] Eroare la citirea dimensiunii comenzi de la server!\n");
            exit(errno);
        }

        char raspuns[lenRaspuns + 5];

        bzero(&raspuns, lenRaspuns + 5);

        if (read(sd, raspuns, lenRaspuns) < 0)
        {
            perror("[client] Eroare la citirea raspunsului de la server!\n");
            exit(errno);
        }

        if (strcmp(raspuns, QUIT) == 0)
        {
            close(sd);
            break;
        }

        if (strcmp(raspuns, "You logged out!\n") == 0)
        {
            close(sd);
            printf("%s", raspuns);
            fflush(stdout);
            connected = 0;
            continue;
        }

        printf("%s\n", raspuns);
        fflush(stdout);

    }

    return 0;
}