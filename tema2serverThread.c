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
#include <sqlite3.h>
#include "loginApproval.c"
#include "getAddSong.c"
#include "adminCommands.c"
#include "dataBaseFunction.c"
#define PORT 8080
#define QUIT "quit message"

static void *treat(void *);

int differentClients = 0;

typedef struct
{
    pthread_t idThread;
    int thCount;
} Thread;

Thread *threadArray;

int sd;

int nothreads;

pthread_mutex_t lockAccept = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockDecrement = PTHREAD_MUTEX_INITIALIZER;

void raspunde(int client, int idThread)
{
    int loggedIn = 0;
    int adminRight = 0;
    int connected = 1;

    char username[100], lenUserName = 0;
    bzero(&username, 100);

    while (connected)
    {
        int length = 0;

        if (read(client, &length, sizeof(int)) <= 0)
        {
            perror("[server] Eroare la citirea dimensiunii de la client!\n");
            break;
        }

        char msgFromClient[length + 5];
        bzero(&msgFromClient, length + 5);

        if (read(client, msgFromClient, length) <= 0)
        {
            perror("[server] Eroare la citirea mesajului de la client!\n");
            continue;
        }

        if (!loggedIn) // daca clientul nu e logat
        {
            if (strstr(msgFromClient, "login "))
            {
                loggedIn = loginApproval(client, idThread, msgFromClient, length);

                if (loggedIn)
                {
                    for (int i = 6; i < length; i++)
                    {
                        username[lenUserName++] = msgFromClient[i];
                    }
                    adminRight = isAdmin(client, idThread, msgFromClient, length);
                }
                continue;
            }

            if (strstr(msgFromClient, "register as administrator "))
            {
                regAsAdmin(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "register as user "))
            {
                regAsUser(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "quit"))
            {
                char ansForClient[100];
                bzero(&ansForClient, 100);

                strcpy(ansForClient, QUIT);

                int lenAnswer = strlen(ansForClient);

                if (write(client, &lenAnswer, sizeof(int)) <= 0)
                {
                    perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
                }

                if (write(client, ansForClient, lenAnswer) <= 0)
                {
                    perror("[thread server] Eroare la scrierea mesajului catre client!\n");
                }
                break;
            }

            char ansForClient[100];
            bzero(&ansForClient, 100);

            strcpy(ansForClient, "You must be logged in!\n");
            int lenAnswer = strlen(ansForClient);
            //printf("%s\n", ansForClient);
            //fflush(stdout);
            if (write(client, &lenAnswer, sizeof(int)) <= 0)
            {
                perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
                break;
            }

            if (write(client, ansForClient, lenAnswer) <= 0)
            {
                perror("[thread server] Eroare la scrierea mesajului catre client!\n");
                break;
            }
        }
        else if (loggedIn) // daca clientul este logat
        {
            if (strstr(msgFromClient, "logout") || strstr(msgFromClient, "quit"))
            {
                char ansForClient[100];
                bzero(&ansForClient, 100);

                strcpy(ansForClient, "You logged out!\n");

                int lenAnswer = strlen(ansForClient);

                if (write(client, &lenAnswer, sizeof(int)) <= 0)
                {
                    perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
                }

                if (write(client, ansForClient, lenAnswer) <= 0)
                {
                    perror("[thread server] Eroare la scrierea mesajului catre client!\n");
                }
                connected = 0;
                continue;
            }

            if (strstr(msgFromClient, "add song ("))
            {
                addSong(client, username, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "vote "))
            {
                voteSong(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "top general"))
            {
                topGeneral(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "top for "))
            {
                topForGenre(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "add comment ("))
            {
                addComment(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "comments for "))
            {
                showComment(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "deny voting for "))
            {
                fflush(stdout);

                if (!adminRight)
                {
                    noAdmRight(client);
                    continue;
                }

                restrictVote(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "delete song "))
            {
                if (!adminRight)
                {
                    noAdmRight(client);
                    continue;
                }

                deleteSong(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "accept admin "))
            {
                if (!adminRight)
                {
                    noAdmRight(client);
                    continue;
                }

                acceptAdminRequest(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "get admin req list"))
            {
                if (!adminRight)
                {
                    noAdmRight(client);
                    continue;
                }

                getAdmReqList(client, idThread, msgFromClient, length);
                continue;
            }

            if (strstr(msgFromClient, "reject admin "))
            {
                if (!adminRight)
                {
                    noAdmRight(client);
                    continue;
                }

                rejectAdminRequest(client, idThread, msgFromClient, length);
                continue;
            }

            unknownCommand(client);
        }
    }
}

int main(int argc, char *argv[])
{
    //Creeaza baza de date
    createDataBase();

    struct sockaddr_in server;
    void createThread(int);

    if (argc < 2)
    {
        perror("[server] Primul argument : numar fire de executie!\n");
        exit(errno);
    }

    nothreads = atoi(argv[1]);

    if (nothreads <= 0)
    {
        perror("[server] Numar de fire invalid!\n");
        exit(errno);
    }

    threadArray = calloc(sizeof(Thread), 500000);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server] Eroare la creare socket!\n");
        exit(errno);
    }

    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server] Eroare la bind!\n");
        exit(errno);
    }

    if (listen(sd, 5) == -1)
    {
        perror("[server] Eroare la listen!\n");
        exit(errno);
    }

    printf("Nr threaduri: %d \n", nothreads);
    fflush(stdout);

    for (int i = 0; i < nothreads; i++)
        createThread(i);

    while (1)
    {
        sleep(1);
        if (differentClients + 1 == nothreads)
        {
            createThread(nothreads);
            nothreads++;
        }
    }

    for (;;)
    {
        printf("[server]Astept la portul, %d!\n", PORT);
        fflush(stdout);
        pause();
    }
}

void createThread(int i)
{
    void *treat(void *);

    pthread_create(&threadArray[i].idThread, NULL, &treat, (void *)i);
    return;
}

void *treat(void *arg)
{
    int client;

    struct sockaddr_in from;
    bzero(&from, sizeof(from));

    printf("[server thread] thread %d incepe!\n", (int)arg);
    fflush(stdout);

    for (;;)
    {
        int length = sizeof(from);

        pthread_mutex_lock(&lockAccept);
        differentClients++;
        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server thread] Eroare la accept!\n");
        }
        pthread_mutex_unlock(&lockAccept);

        threadArray[(int)arg].thCount++;

        raspunde(client, (int)arg);

        pthread_mutex_lock(&lockDecrement);
        differentClients--;
        pthread_mutex_unlock(&lockDecrement);

        // printf("nr_clienti=%d\n", differentClients);
        // fflush(stdout);
        printf("[server] Clientul a terminat conexiunea!\n");
        fflush(stdout);
        close(client);
    }
}