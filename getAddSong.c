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
#include <ctype.h>

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

char *top;

static int callbackFctForTop(void *info, int argc, char **argv, char **colName)
{
    for (int i = 0; i < argc; i++)
    {
        //printf("%s : %s\n", colName[i], argv[i]);
        //fflush(stdout);
        strcat(top, colName[i]);
        strcat(top, ": ");
        strcat(top, argv[i]);
        strcat(top, "\n");
    }
    strcat(top, "\n");
    return 0;
}

char *comments;

static int callbackFctForComments(void *info, int argc, char **argv, char **colName)
{
    strcat(comments, argv[2]);
    strcat(comments, ": ");
    strcat(comments, argv[1]);
    strcat(comments, "\n");
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

    char genre[15][30], nrGen = 0, genreList[300];
    bzero(&genre, 450);
    bzero(&genreList, 300);

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
        return 0;
    }

    sqlite3 *database;
    char *errorMessage;
    int returnCode;
    char *sql;
    sqlite3_open("topDataBase.db", &database);
    for (int i = 0; i < nrGen - 1; i++)
    {
        strcat(genreList, genre[i]);
        strcat(genreList, ", ");
    }

    strcat(genreList, genre[nrGen - 1]);

    sql = calloc(sizeof(char), 1000);

    strcpy(sql, "INSERT INTO SONG (SONGNAME, NRVOTES, ADDEDBY, DESCRIPTION, GENRE, LINK) VALUES('");
    strcat(sql, songName);
    strcat(sql, "',0,'");
    strcat(sql, username);
    strcat(sql, "','");
    strcat(sql, songDescription);
    strcat(sql, "','");
    strcat(sql, genreList);
    strcat(sql, "','");
    strcat(sql, link);
    strcat(sql, "');");

    // printf("\n%s\n", sql);
    // fflush(stdout);

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

    sqlite3_close(database);
    return 1;
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
            return 0;
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
        return 0;
    }

    return 1;
}

int topGeneral(int client, int idThread, char *comanda, int length)
{
    //ofera clientului topul general.

    sqlite3 *database;
    int returnCode;
    char *errormsg;
    char *sql;

    sqlite3_open("topDataBase.db", &database);

    sql = calloc(sizeof(char), 300);

    strcpy(sql, "SELECT * FROM SONG ORDER BY NRVOTES DESC;");

    top = (char *)calloc(sizeof(char), 400000);

    strcpy(top, "\n***TOP GENERAL***\n");

    returnCode = sqlite3_exec(database, sql, callbackFctForTop, 0, &errormsg);

    if (returnCode == SQLITE_OK)
    {
        // printf("dim: %d !!!\nstring: %s\n", strlen(top), top);
        // fflush(stdout);

        int lenAnswer = strlen(top);

        if (write(client, &lenAnswer, sizeof(int)) <= 0)
        {
            perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        }

        if (write(client, top, lenAnswer) <= 0)
        {
            perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        }
    }
    else
    {

        printf("dim: %d\nstring: %s\n", returnCode, top);
        fflush(stdout);
    }

    sqlite3_close(database);

    return 1;
}

int topForGenre(int client, int idThread, char *comanda, int length)
{
    //ofera clientului topul pentru genul specificat de catre el.

    sqlite3 *database;
    int returnCode;
    char *errormsg;
    char *genre, lggen = 0;
    char *sql;

    genre = calloc(sizeof(char), 30);

    for (int i = 8; i < length; i++)
    {
        genre[lggen++] = comanda[i];
    }

    sqlite3_open("topDataBase.db", &database);

    sql = calloc(sizeof(char), 300);

    strcpy(sql, "SELECT * FROM SONG WHERE GENRE LIKE '%");
    strcat(sql, genre);
    strcat(sql, "%' ORDER BY NRVOTES DESC;");

    top = (char *)calloc(sizeof(char), 400000);

    for (int i = 0; i < lggen; i++)
    {
        genre[i] = toupper(genre[i]);
    }

    strcpy(top, "\n***TOP FOR ");
    strcat(top, genre);
    strcat(top, "***\n");

    returnCode = sqlite3_exec(database, sql, callbackFctForTop, 0, &errormsg);

    if (returnCode == SQLITE_OK)
    {
        // printf("dim: %d !!!\nstring: %s\n", strlen(top), top);
        // fflush(stdout);

        int lenAnswer = strlen(top);

        if (write(client, &lenAnswer, sizeof(int)) <= 0)
        {
            perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
        }

        if (write(client, top, lenAnswer) <= 0)
        {
            perror("[thread server] Eroare la scrierea mesajului catre client!\n");
        }
    }
    else
    {

        printf("dim: %d\nstring: %s\n", returnCode, top);
        fflush(stdout);
    }

    sqlite3_close(database);

    return 1;
}

int addComment(int client, char *username, char *comanda, int length)
{
    char *comment, *songname;

    int lgcomm = 0, lgsong = 0, schimb = 0;

    comment = calloc(sizeof(char), 500);
    songname = calloc(sizeof(char), 200);

    for (int i = 13; i < length; i++)
    {
        if (comanda[i] == ')')
            schimb = 1, i += 3;

        if (!schimb)
            songname[lgsong++] = comanda[i];
        else if (comanda[i] != ')')
            comment[lgcomm++] = comanda[i];
    }

    printf("comment: %s\nsongname: %s\n", comment, songname);
    fflush(stdout);

    if (lgsong == 0 || lgcomm == 0)
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "Comment too short!\n");
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
    sql = calloc(sizeof(char), 500);

    sqlite3_open("topDataBase.db", &database);

    strcpy(sql, "SELECT * FROM SONG WHERE SONGNAME = '");
    strcat(sql, songname);
    strcat(sql, "';");

    existsRowsInDataBase = 0;

    returnCode = sqlite3_exec(database, sql, checkIfSongExists, 0, &errormsg);

    // printf("%d\n%s\n%s\n", returnCode, sql, errormsg);
    // fflush(stdout);

    if (existsRowsInDataBase)
    {
        sql = calloc(sizeof(char), 500);

        //sql = "CREATE TABLE COMMENTS(SONGNAME TEXT, COMMENT TEXT, ADDEDBY TEXT);";

        strcpy(sql, "INSERT INTO COMMENTS (SONGNAME, COMMENT, ADDEDBY) VALUES ('");
        strcat(sql, songname);
        strcat(sql, "', '");
        strcat(sql, comment);
        strcat(sql, "', '");
        strcat(sql, username);
        strcat(sql, "');");

        // printf("%s\n", sql);
        // fflush(stdout);

        returnCode = sqlite3_exec(database, sql, checkIfSongExists, 0, &errormsg);

        // printf("%d", returnCode);
        // fflush(stdout);
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

    sqlite3_close(database);

    return 1;
}

int deleteComment(int client, int idThread, char *comanda, int length)
{
    char *comment;
    comment = calloc(sizeof(char), 300);

    int lencomm = 0;

    for (int i = 15; i < length; i++)
    {
        comment[lencomm++] = comanda[i];
    }

    sqlite3 *database;
    char *errormsg;
    int returnCode;
    char *sql;

    sql = calloc(sizeof(char), 400);

    sqlite3_open("topDataBase.db", &database);

    strcpy(sql, "DELETE FROM COMMENTS WHERE COMMENT='");
    strcat(sql, comment);
    strcat(sql, "';");

    returnCode = sqlite3_exec(database, sql, callbackFctForAdminCheck, 0, &errormsg);

    if (returnCode == SQLITE_OK)
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "Comment deleted!\n");
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
        printf("%s", errormsg);
        fflush(stdout);
    }
}

int showComment(int client, int idThread, char *comanda, int length)
{
    char *songname;

    int lgsong = 0;

    songname = calloc(sizeof(char), 200);

    for (int i = 13; i < length; i++)
    {
        songname[lgsong++] = comanda[i];
    }

    if (lgsong == 0)
    {
        char ansForClient[100];
        bzero(&ansForClient, 100);

        strcpy(ansForClient, "Song name too short!\n");
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
    sql = calloc(sizeof(char), 500);

    sqlite3_open("topDataBase.db", &database);

    strcpy(sql, "SELECT * FROM COMMENTS WHERE SONGNAME = '");
    strcat(sql, songname);
    strcat(sql, "';");

    existsRowsInDataBase = 0;

    returnCode = sqlite3_exec(database, sql, checkIfSongExists, 0, &errormsg);

    // printf("%d\n%s\n%s\n", returnCode, sql, errormsg);
    // fflush(stdout);

    if (existsRowsInDataBase)
    {
        sql = calloc(sizeof(char), 500);

        comments = (char *)calloc(sizeof(char), 400000);

        //sql = "CREATE TABLE COMMENTS(SONGNAME TEXT, COMMENT TEXT, ADDEDBY TEXT);";

        strcpy(sql, "SELECT * FROM COMMENTS WHERE SONGNAME='");
        strcat(sql, songname);
        strcat(sql, "';");

        strcpy(comments, "***COMMENTS FOR ");

        for (int i = 0; i < lgsong; i++)
        {
            songname[i] = toupper(songname[i]);
        }

        strcat(comments, songname);
        strcat(comments, "***\n");
        // printf("%s\n", sql);
        // fflush(stdout);

        returnCode = sqlite3_exec(database, sql, callbackFctForComments, 0, &errormsg);
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

    int lenAnswer = strlen(comments);

    if (write(client, &lenAnswer, sizeof(int)) <= 0)
    {
        perror("[thread server] Eroare la scrierea dimensiunii catre client!\n");
    }

    if (write(client, comments, lenAnswer) <= 0)
    {
        perror("[thread server] Eroare la scrierea mesajului catre client!\n");
    }

    sqlite3_close(database);

    return 1;
}