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

int userExistsInAdminFile = 0;

static int callbackFctForExistingUser(void *data, int argc, char **argv, char **colName)
{
    userExistsInAdminFile = 1;
    return 0;
}

char *adminsReqList;

static int callbackFctForGettingAdminReqList(void *data, int argc, char **argv, char **colName)
{
    strcat(adminsReqList, argv[0]);
    strcat(adminsReqList, "\n");

    // printf("%s", adminsReqList);
    // fflush(stdout);
    return 0;
}

int restrictVote(int client, int idThread, char *comanda, int length)
{
    char *username, lgu = 0;

    username = calloc(sizeof(char), 200);

    for (int i = 16; i < length; i++)
    {
        username[lgu++] = comanda[i];
    }

    if (lgu == 0)
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "Invalid username!\n");
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

    sqlite3 *database;
    sqlite3_open("topDataBase.db", &database);
    char *errorMessage;
    int returnCode;
    char *sql;
    sql = calloc(sizeof(char), 300);

    strcpy(sql, "SELECT * FROM USERS WHERE USERNAME LIKE '");
    strcat(sql, username);
    strcat(sql, "';");

    userExistsInAdminFile = 0;

    returnCode = sqlite3_exec(database, sql, callbackFctForExistingUser, NULL, &errorMessage);

    if (userExistsInAdminFile)
    {
        sql = calloc(sizeof(char), 300);

        // printf("working!");
        // fflush(stdout);

        strcpy(sql, "UPDATE USERS SET RIGHTTOVOTE = 0 WHERE USERNAME LIKE '");
        strcat(sql, username);
        strcat(sql, "';");

        printf("sql : %s\n", sql);
        fflush(stdout);

        returnCode = sqlite3_exec(database, sql, callbackFctForExistingUser, NULL, &errorMessage);

        printf("return code: %d", returnCode);
        fflush(stdout);

        if (returnCode == SQLITE_OK)
        {
            printf("SQLITE_OK in restrictVote function!\n");
            fflush(stdout);

            char ansForClient[100];
            bzero(&ansForClient, 100);

            strcpy(ansForClient, "Vote restricted for user ");
            strcat(ansForClient, username);
            strcat(ansForClient, "!\n");
            int lenAnswer = strlen(ansForClient);
            printf("%s", ansForClient);
            fflush(stdout);
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
    else
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "User does not exist!\n");
        int lenAnswer = strlen(ansForClient);
        printf("%s", ansForClient);
        fflush(stdout);
        if (write(client, &lenAnswer, sizeof(int)) <= 0)
        {
            perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        }

        if (write(client, ansForClient, lenAnswer) <= 0)
        {
            perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        }
    }

    sqlite3_close(database);

    return 1;
}

int deleteSong(int client, int idThread, char *comanda, int length)
{
    char *songname, lgsong = 0;

    songname = calloc(sizeof(char), 200);

    for (int i = 12; i < length; i++)
    {
        songname[lgsong++] = comanda[i];
    }

    sqlite3 *database;
    char *errormsg;
    int returnCode;
    char *sql;
    sqlite3_open("topDataBase.db", &database);

    sql = calloc(sizeof(char), 300);

    userExistsInAdminFile = 0;

    strcpy(sql, "SELECT * FROM SONG WHERE SONGNAME='");
    strcat(sql, songname);
    strcat(sql, "';");

    sqlite3_exec(database, sql, callbackFctForExistingUser, 0, &errormsg);

    if (userExistsInAdminFile)
    {
        sql = calloc(sizeof(char), 500);
        strcpy(sql, "DELETE FROM SONG WHERE SONGNAME='");
        strcat(sql, songname);
        strcat(sql, "'; DELETE FROM COMMENTS WHERE SONGNAME='");
        strcat(sql, songname);
        strcat(sql, "'; DELETE FROM GENRE WHERE SONGNAME ='");
        strcat(sql, songname);
        strcat(sql, "';");

        printf("%s\n", sql);
        fflush(stdout);

        sqlite3_exec(database, sql, callbackFctForExistingUser, 0, &errormsg);
    }
    else
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "Song does not exist!\n");

        int lenAnswer = strlen(ansForClient);

        if (write(client, &lenAnswer, sizeof(int)) <= 0)
        {
            perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        }

        if (write(client, ansForClient, lenAnswer) <= 0)
        {
            perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        }

        sqlite3_close(database);

        return 0;
    }
    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "Song deleted!\n");

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

int getAdmReqList(int client, int idThread, char *comanda, int length)
{
    //ofera o lista a persoanelor care doresc drept de administrator
    sqlite3 *database;

    sqlite3_open("topDataBase.db", &database);

    char *errorMessage;
    int returnCode;
    char *sql;
    sql = calloc(sizeof(char), 300);

    strcpy(sql, "SELECT * FROM REQLIST;");

    adminsReqList = (char *)calloc(sizeof(char), 1500);

    strcpy(adminsReqList, "**ADMIN REQUEST LIST**\n");

    returnCode = sqlite3_exec(database, sql, callbackFctForGettingAdminReqList, NULL, &errorMessage);

    printf("%d\n", returnCode);
    fflush(stdout);

    if (returnCode == SQLITE_OK)
    {
        int ansLen = 0;
        ansLen = strlen(adminsReqList);

        char ansForClient[ansLen];
        bzero(&ansForClient, ansLen);

        if (write(client, &ansLen, sizeof(int)) <= 0)
        {
            perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        }

        if (write(client, adminsReqList, ansLen) <= 0)
        {
            perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        }
    }
    sqlite3_close(database);

    return 1;
}

int acceptAdminRequest(int client, int idThread, char *comanda, int length)
{
    //accepta cererea de admin pentru un anumit utilizator

    char *username, lgu = 0;
    username = calloc(sizeof(char), 200);

    for (int i = 13; i < length; i++)
    {
        username[lgu++] = comanda[i];
    }

    if (lgu == 0)
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "username too short!\n");
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

    sqlite3 *database;
    char *errormsg;
    int returnCode;
    char *sql;

    sqlite3_open("topDataBase.db", &database);
    sql = calloc(sizeof(char), 300);

    strcpy(sql, "SELECT * FROM REQLIST WHERE USERNAME='");
    strcat(sql, username);
    strcat(sql, "';");

    userExistsInAdminFile = 0;

    sqlite3_exec(database, sql, callbackFctForExistingUser, 0, &errormsg);

    if (userExistsInAdminFile)
    {
        sql = calloc(sizeof(char), 300);
        strcpy(sql, "UPDATE USERS SET REGASADMIN = 1 WHERE USERNAME='");
        strcat(sql, username);
        strcat(sql, "'; DELETE FROM REQLIST WHERE USERNAME='");
        strcat(sql, username);
        strcat(sql, "';");

        returnCode = sqlite3_exec(database, sql, callbackFctForExistingUser, 0, &errormsg);

        printf("returnCode: %d", returnCode);
        fflush(stdout);
    }
    else
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "User does not exist!\n");

        int lenAnswer = strlen(ansForClient);

        if (write(client, &lenAnswer, sizeof(int)) <= 0)
        {
            perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        }

        if (write(client, ansForClient, lenAnswer) <= 0)
        {
            perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        }

        sqlite3_close(database);

        return 0;
    }

    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "User ");
    strcat(ansForClient, username);
    strcat(ansForClient, " is administrator now!\n");

    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }

    sqlite3_close(database);

    return 1;
}

int rejectAdminRequest(int client, int idThread, char *comanda, int length)
{
    // respinge cererea de admin pentru un anumit utilizator

    char *username, lgu = 0;
    username = calloc(sizeof(char), 200);

    for (int i = 13; i < length; i++)
    {
        username[lgu++] = comanda[i];
    }

    if (lgu == 0)
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "username too short!\n");
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

    sqlite3 *database;
    char *errormsg;
    int returnCode;
    char *sql;

    sqlite3_open("topDataBase.db", &database);
    sql = calloc(sizeof(char), 300);

    strcpy(sql, "SELECT * FROM REQLIST WHERE USERNAME='");
    strcat(sql, username);
    strcat(sql, "';");

    userExistsInAdminFile = 0;

    sqlite3_exec(database, sql, callbackFctForExistingUser, 0, &errormsg);

    if (userExistsInAdminFile)
    {
        sql = calloc(sizeof(char), 300);
        strcpy(sql, "DELETE FROM REQLIST WHERE USERNAME='");
        strcat(sql, username);
        strcat(sql, "';");

        returnCode = sqlite3_exec(database, sql, callbackFctForExistingUser, 0, &errormsg);

        printf("returnCode: %d\n", returnCode);
        fflush(stdout);
    }
    else
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "User does not exist!\n");

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

    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "Admin right rejected for ");
    strcat(ansForClient, username);
    strcat(ansForClient, "!\n");

    int lenAnswer = strlen(ansForClient);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, ansForClient, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }

    sqlite3_close(database);

    return 1;
}
