#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

// fiecare functie va verifica daca utilizatorul autentificat este admin.
// In caz contrar va oferi un mesaj de necesitate a dreptului de administrator pentru a utiliza aceasta comanda.

int restrictVote(int client, int idThread, char *comanda, int length)
{
    // restrictioneaza votul pentru un anumit utilizator
    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "Vote restricted for user user1!\n");
    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }

    return 1;
}

int deleteSong(int client, int idThread, char *comanda, int length)
{
    // va sterge o melodie din baza de date

    char *song;

    song = calloc(sizeof(char), 200);

    return 1;
}

int getAdmReqList(int client, int idThread, char *comanda, int length)
{
    //ofera o lista a persoanelor care doresc drept de administrator
    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "Admin reqest list:\n 1.user1\n2.user2\n3.user3\n");
    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }

    return 1;
}

int acceptAdminRequest(int client, int idThread, char *comanda, int length)
{
    //accepta cererea de admin pentru un anumit utilizator

    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "User user1 is administrator now!");
    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }

    return 1;
}

int rejectAdminRequest(int client, int idThread, char *comanda, int length)
{
    // respinge cererea de admin pentru un anumit utilizator

    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "Administrator request rejected for user user1!");

    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }
    return 1;
}
