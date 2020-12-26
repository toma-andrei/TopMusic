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

static int getInfoDATABASEFUNCTION(void *info, int argc, char **argv, char **colName)
{
}

void createDataBase()
{
    //Creeaza baza de date
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
        printf("Baza de date creata cu succes!\n");
        fflush(stdout);
    }

    char *sql;
    sql = "CREATE TABLE USERS(REGASADMIN INT, RIGHTTOVOTE INT, USERNAME CHAR(50) UNIQUE, PASSWORD CHAR(50));";

    returnCode = sqlite3_exec(database, sql, getInfoDATABASEFUNCTION, 0, &errorMessage);

    if (returnCode != SQLITE_OK)
    {
        printf("Error at creating table users!\n");
        fflush(stdout);
    }
    else
    {
        printf("Table users created successfully\n");
        fflush(stdout);
    }

    sql = "CREATE TABLE SONG(SONGNAME TEXT UNIQUE, NRVOTES INT, ADDEDBY TEXT, DESCRIPTION TEXT, LINK TEXT);";
    returnCode = sqlite3_exec(database, sql, getInfoDATABASEFUNCTION, 0, &errorMessage);

    if (returnCode != SQLITE_OK)
    {
        printf("Error at creating table SONG!\n");
        fflush(stdout);
    }
    else
    {
        printf("Table song created successfully\n");
        fflush(stdout);
    }

    sql = "CREATE TABLE GENRE(GENRE CHAR(20), SONGNAME TEXT);";
    returnCode = sqlite3_exec(database, sql, getInfoDATABASEFUNCTION, 0, &errorMessage);

    if (returnCode != SQLITE_OK)
    {
        printf("Error at creating table GENRE!\n");
        fflush(stdout);
    }
    else
    {
        printf("Table genre created successfully\n");
        fflush(stdout);
    }

    sql = "CREATE TABLE COMMENTS(SONGNAME TEXT, COMMENT TEXT, ADDEDBY TEXT);";
    returnCode = sqlite3_exec(database, sql, getInfoDATABASEFUNCTION, 0, &errorMessage);

    if (returnCode != SQLITE_OK)
    {
        printf("Error at creating table Comments!\n");
        fflush(stdout);
    }
    else
    {
        printf("Table comments created successfully\n");
        fflush(stdout);
    }

    sql = "CREATE TABLE REQLIST(USERNAME TEXT UNIQUE)";
    returnCode = sqlite3_exec(database, sql, getInfoDATABASEFUNCTION, 0, &errorMessage);

    if (returnCode != SQLITE_OK)
    {
        printf("Error at creating table reqlist!\n");
        fflush(stdout);
    }
    else
    {
        printf("Table reqlist created successfully\n");
        fflush(stdout);
    }

    sqlite3_close(database);
}
