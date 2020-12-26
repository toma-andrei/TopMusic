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

int existsRowsInDataBase = 0;
int userAllowedToVote = 0;

static int checkIfSongExists(void *info, int argc, char **argv, char **colName)
{
    existsRowsInDataBase = 1;
    return 0;
}

static int callbackFctUserAllowedToVote(void *info, int argc, char **argv, char **colName)
{
    userAllowedToVote = 1;
    return 0;
}

int addSong(int client, char *username, char *comanda, int length)
{
    char songName[200], lenSongName = 0, okSongName = 0;
    bzero(&songName, 200);

    fflush(stdout);
    char songDescription[400];
    int lenDescription = 0;
    bzero(&songDescription, 400);

    char genre[15][30], nrGen = 0;
    bzero(&genre, 450);

    char link[250], lenLink = 0;
    bzero(&link, 250);

    int contorSpaces = 0, missingContent = 0;
    for (int i = 1; i < length - 1; i++)
    {
        if ((comanda[i - 1] == 'g' || comanda[i - 1] == ')') && comanda[i] == ' ' && comanda[i + 1] == '(')
            contorSpaces++;

        if (comanda[i] == '(')
        {
            if (lenSongName == 0)
            {
                for (int j = i + 1; j < length; j++)
                {
                    if (comanda[j] == ')')
                    {
                        i += (lenSongName);
                        break;
                    }
                    else
                    {
                        songName[lenSongName++] = comanda[j];
                    }
                }
            }
            else if (lenDescription == 0)
            {
                for (int j = i + 1; j < length; j++)
                {
                    if (comanda[j] == ')')
                    {
                        i += (lenDescription);
                        break;
                    }
                    else
                    {
                        songDescription[lenDescription++] = comanda[j];
                    }
                }
            }
            else if (nrGen == 0)
            {
                for (int j = i + 1; j < length; j++)
                {
                    int lenOneGen = 0;
                    for (int k = j; k < length; k++)
                    {
                        if (comanda[k] == ',' || comanda[k] == ' ' || comanda[k] == ')')
                        {
                            if (k != j)
                            {
                                j += lenOneGen;
                                nrGen++;
                            }
                            break;
                        }
                        genre[nrGen][lenOneGen++] = comanda[k];
                    }

                    if (comanda[j] == ')')
                        break;
                }
            }
            else if (lenLink == 0)
            {
                for (int j = i + 1; j < length; j++)
                {
                    if (comanda[j] == ')')
                    {
                        i += (lenLink);
                        break;
                    }
                    else
                    {
                        link[lenLink++] = comanda[j];
                    }
                }
            }
        }
    }

    if (contorSpaces == 0 || lenSongName == 0 || lenLink == 0 || lenDescription == 0 || nrGen == 0)
    {
        char ansForClient[200];
        bzero(&ansForClient, 200);

        strcpy(ansForClient, "Format unknown!\nTry using \"add song (<songName>) (<description>) (<gen1, gen2...>) (<link>)\"");
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
    char *errorMessage;
    int returnCode;
    char *sql;
    sqlite3_open("topDataBase.db", &database);

    sql = calloc(sizeof(char), 1000);

    strcpy(sql, "INSERT INTO SONG (SONGNAME, NRVOTES, ADDEDBY, DESCRIPTION, LINK) VALUES('");
    strcat(sql, songName);
    strcat(sql, "',0,'");
    strcat(sql, username);
    strcat(sql, "','");
    strcat(sql, songDescription);
    strcat(sql, "','");
    strcat(sql, link);
    strcat(sql, "');");

    printf("\n%s\n", sql);
    fflush(stdout);

    returnCode = sqlite3_exec(database, sql, checkIfSongExists, 0, &errorMessage);

    if (returnCode == SQLITE_OK)
    {
        char ansForClient[200];
        bzero(&ansForClient, 200);

        strcpy(ansForClient, "Song added successfully!\n");
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
        char ansForClient[200];
        bzero(&ansForClient, 200);

        strcpy(ansForClient, "Song already exists!\n");
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

    sql = calloc(sizeof(char), 1500);

    for (int i = 0; i < nrGen; i++)
    {
        strcat(sql, "INSERT INTO GENRE(GENRE, SONGNAME) VALUES('");
        strcat(sql, genre[i]);
        strcat(sql, "','");
        strcat(sql, songName);
        strcat(sql, "'); ");
    }
    printf("\n%s\n", sql);

    returnCode = sqlite3_exec(database, sql, checkIfSongExists, 0, &errorMessage);

    if (returnCode == SQLITE_OK)
    {
        printf("Inserted successfully in GENRE!\n");
        fflush(stdout);
    }

    sqlite3_close(database);
}

int voteSong(int client, char *username, char *comanda, int length)
{
    //userul voteaza o melodie si primeste confirmare daca nu este restrictionat dreptul sau de vot.

    char songName[150], lenSongName = 0;
    bzero(&songName, 150);

    for (int i = 5; i < length; i++)
    {
        songName[lenSongName++] = comanda[i];
    }

    sqlite3 *database;
    char *errorMessage;
    int returnCode;
    char *sql;
    sql = calloc(sizeof(char), 1500);

    sqlite3_open("topDataBase.db", &database);
    strcat(sql, "SELECT * FROM SONG WHERE SONGNAME='");
    strcat(sql, songName);
    strcat(sql, "';");

    existsRowsInDataBase = 0;

    returnCode = sqlite3_exec(database, sql, checkIfSongExists, NULL, &errorMessage);

    if (existsRowsInDataBase)
    {
        userAllowedToVote = 0;

        sql = calloc(sizeof(char), 300);
        strcpy(sql, "SELECT * FROM USERS WHERE USERNAME LIKE '");
        strcat(sql, username);
        strcat(sql, "' AND RIGHTTOVOTE = 1;");

        // printf("%s", sql);
        // fflush(stdout);

        sqlite3_exec(database, sql, callbackFctUserAllowedToVote, NULL, &errorMessage);

        if (userAllowedToVote)
        {
            sql = calloc(sizeof(char), 300);

            strcat(sql, "UPDATE SONG SET NRVOTES=NRVOTES+1 WHERE SONGNAME='");
            strcat(sql, songName);
            strcat(sql, "';");

            returnCode = sqlite3_exec(database, sql, checkIfSongExists, 0, &errorMessage);

            // printf("error msg: %s", errorMessage);
            // fflush(stdout);

            if (returnCode == SQLITE_OK)
            {
                char ansForClient[100];
                bzero(&ansForClient, 100);

                strcpy(ansForClient, "Song voted!\n");
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
        else
        {
            char ansForClient[100];
            bzero(&ansForClient, 100);

            strcpy(ansForClient, "You are not allowed to vote!\n");

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
    }

    return 1;
}

int topGeneral(int client, int idThread, char *comanda, int length)
{
    //ofera clientului topul general.

    char ansForClient[1000];
    bzero(&ansForClient, 1000);

    strcpy(ansForClient, "TOP GENERAL:\n1.\nsong1\ndescription1\ngenre1\nlink1\n\n2.\nsong2\ndescription2\ngenre2\nlink2\n\n");
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

int topForGenre(int client, int idThread, char *comanda, int length)
{
    //ofera clientului topul pentru genul specificat de catre el.

    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "TOP FOR <GENRE>:\n1.song1...\n\n");
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

int addComment(int client, int idThread, char *comanda, int length)
{
    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "Comment added!\n");
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

int showComment(int client, int idThread, char *msgFromClient, int length)
{
    char ansForClient[100];
    bzero(&ansForClient, 100);

    strcpy(ansForClient, "Comments for <songName>:\nuser1: comment1\nuser2: comment2\n\n");
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