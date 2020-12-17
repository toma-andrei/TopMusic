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

//zonele critice (actualizari sau adaugari in baza de date vor fi blocate cu mecanismul MUTEX)

static int callbackFct(void *data, int argc, char **argv, char **colName)
{
}

int loginApproval(int client, int idThread, char *comanda, int length)
{
    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "You are logged in!\n");

    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        return 0;
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        return 0;
    }

    return 1; // returneaza daca utilizatorul este prezent in baza de date.
}

int regAsAdmin(int client, int idThread, char *comanda, int length)
{
    char *username;
    char *password;

    username = calloc(sizeof(char), 200);
    password = calloc(sizeof(char), 200);

    int lgu = 0, lgp = 0;
    int schimb = 0;

    for (int i = 26; i < length; i++)
    {
        if (comanda[i] == ' ')
        {
            schimb = 1;
            continue;
        }

        if (schimb == 0)
        {
            username[lgu++] = comanda[i];
        }
        else
        {
            password[lgp++] = comanda[i];
        }
    }
    if (lgp == 0 || lgu == 0)
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "username or password too short!\n");

        int lenAnswer = strlen(ansForClient);

        if (write(client, &lenAnswer, sizeof(int)) <= 0)
        {
            perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        }

        if (write(client, ansForClient, lenAnswer) <= 0)
        {
            perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        }
        return 0;
    }
    else
    {
        char *sql;
        sql = calloc(sizeof(char), 200);

        strcpy(sql, "INSERT INTO USERS (REGASADMIN,RIGHTTOVOTE,USERNAME,PASSWORD) VALUES(0,1,'");
        strcat(sql, username);
        strcat(sql, "','");
        strcat(sql, password);
        strcat(sql, "');");
        // printf("\n%s\n", sql);
        // fflush(stdout);

        sqlite3 *database;
        int returnCode;
        char *errorMessage = 0;

        if (sqlite3_open("topDataBase.db", &database))
        {
            perror("[server] Eroare la creearea bazei de date!\n");
            exit(errno);
        }

        returnCode = sqlite3_exec(database, sql, callbackFct, 0, &errorMessage);
        // printf("\n%d\n", returnCode);
        // fflush(stdout);
        if (returnCode == SQLITE_OK)
        {
            printf("Inserted successfully in USERS!\n");
            fflush(stdout);

            char ansForClient[100];
            bzero(&ansForClient, 100);

            strcpy(ansForClient, "You are registered as user. An admin will accept your request soon. Please log in!\n");

            int lenAnswer = strlen(ansForClient);

            if (write(client, &lenAnswer, sizeof(int)) <= 0)
            {
                perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
            }

            if (write(client, ansForClient, lenAnswer) <= 0)
            {
                perror("[thread server] Eroare la scrierea mesajului catre client!\n");
            }
        }
        else if (returnCode == SQLITE_CONSTRAINT)
        {
            char ansForClient[100];
            bzero(&ansForClient, 100);

            strcpy(ansForClient, "Username already exists!\n");

            int lenAnswer = strlen(ansForClient);

            if (write(client, &lenAnswer, sizeof(int)) <= 0)
            {
                perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
            }

            if (write(client, ansForClient, lenAnswer) <= 0)
            {
                perror("[thread server] Eroare la scrierea mesajului catre client!\n");
            }

            return 0;
        }

        strcpy(sql, "INSERT INTO REQLIST (USERNAME) VALUES('");
        strcat(sql, username);
        strcat(sql, "');");
        returnCode = sqlite3_exec(database, sql, callbackFct, 0, &errorMessage);
        sqlite3_close(database);

        if (returnCode == SQLITE_OK)
        {
            printf("Inserted successfully in Reqlist!\n");
            fflush(stdout);
        }
    }
    return 1;
}

int regAsUser(int client, int idThread, char *comanda, int length)
{
    char *username;
    char *password;
    username = calloc(sizeof(char), 200);
    password = calloc(sizeof(char), 200);

    int lgu = 0, lgp = 0;
    int schimb = 0;

    for (int i = 17; i < length; i++)
    {
        if (comanda[i] == ' ')
        {
            schimb = 1;
            continue;
        }

        if (schimb == 0)
        {
            username[lgu++] = comanda[i];
        }
        else
        {
            password[lgp++] = comanda[i];
        }
    }

    if (lgp == 0 || lgu == 0)
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "username or password too short!\n");

        int lenAnswer = strlen(ansForClient);

        if (write(client, &lenAnswer, sizeof(int)) <= 0)
        {
            perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        }

        if (write(client, ansForClient, lenAnswer) <= 0)
        {
            perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        }
    }
    else
    {
        //sql= "CREATE TABLE USERS(REGASADMIN INT, RIGHTTOVOTE INT, USERNAME CHAR(50), PASSWORD CHAR(50));";
        char *sql;
        sql = calloc(sizeof(char), 200);

        strcpy(sql, "INSERT INTO USERS (REGASADMIN,RIGHTTOVOTE,USERNAME,PASSWORD) VALUES(0,1,'");
        strcat(sql, username);
        strcat(sql, "','");
        strcat(sql, password);
        strcat(sql, "');");
        printf("\n%s\n", sql);
        fflush(stdout);

        sqlite3 *database;
        int returnCode;
        char *errorMessage = 0;

        if (sqlite3_open("topDataBase.db", &database))
        {
            perror("[server] Eroare la creearea bazei de date!\n");
            exit(errno);
        }
        else
        {
            printf("Baza de date deschisa cu succes!\n");
            fflush(stdout);
        }

        returnCode = sqlite3_exec(database, sql, callbackFct, 0, &errorMessage);
        sqlite3_close(database);

        if (returnCode == SQLITE_OK)
        {
            printf("insert successful");
            fflush(stdout);

            char ansForClient[100];
            bzero(&ansForClient, 100);

            strcpy(ansForClient, "You are registered as user. Please log in!\n");

            int lenAnswer = strlen(ansForClient);

            if (write(client, &lenAnswer, sizeof(int)) <= 0)
            {
                perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
            }

            if (write(client, ansForClient, lenAnswer) <= 0)
            {
                perror("[thread server] Eroare la scrierea mesajului catre client!\n");
            }
        }
        else
        {
            char ansForClient[100];
            bzero(&ansForClient, 100);

            strcpy(ansForClient, "Something went wrong! You couldn't register!\n");

            int lenAnswer = strlen(ansForClient);

            if (write(client, &lenAnswer, sizeof(int)) <= 0)
            {
                perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
            }

            if (write(client, ansForClient, lenAnswer) <= 0)
            {
                perror("[thread server] Eroare la scrierea mesajului catre client!\n");
            }
        }
    }
}

int isAdmin(int client, int idThread, char *comanda, int length)
{
    return 1;
}

void noAdmRight(int client)
{
    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "You must have administrator right for this command!\n");

    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }
}

void unknownCommand(int client)
{

    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "Unknown command!\n");

    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }
}
