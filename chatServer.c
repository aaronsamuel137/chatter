// #define _XOPEN_SOURCE_EXTENDED 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
// #include <string>
// #include <cstdlib>

#include <sys/errno.h>
#include <stdarg.h>

#define MESSAGE_LENGTH 128

extern int errno;

int errexit(const char *format, ...);
int updSocket(const char *portnum);

int main(int argc, char**argv)
{
    int sockfd, n;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char mesg[MESSAGE_LENGTH];
    char *portnum = "32000";

    char roomName[24];
    char *name;

    sockfd = updSocket(portnum);

    for (;;)
    {
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, mesg, MESSAGE_LENGTH, 0, (struct sockaddr *)&cliaddr, &len);

        if (strncmp("Start ", mesg, 6) == 0)
        {
            printf("Got Start command\n");
            strcpy(roomName, mesg);
            name = strtok(roomName, " ");
            name = strtok(NULL, "\0");
            printf("name: %s\n", name);
            strncpy(mesg, name, strlen(name));
            // n = strlen(mesg);
        }
        else if (strncmp("Find ", mesg, 5) == 0)
        {
            printf("Find\n");
        }
        else if (strncmp("Terminate ", mesg, 10) == 0)
        {
            printf("Terminate\n");
        }
        else
        {
            strcpy(mesg, "Invalid command. Commands must beign with Start, Find or Terminate\n\0");
        }
        n = strlen(mesg);
        sendto(sockfd, mesg, n, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));

        // printf("-------------------------------------------------------\n");
        // mesg[n] = 0;
        // printf("Received the following:\n");
        // printf("%s", mesg);
        // printf("-------------------------------------------------------\n");
    }
}

/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
int errexit(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

int updSocket(const char *portnum)
/*
 * Arguments:
 *      portnum   - port number of the server
 */
{
    struct sockaddr_in sin; /* an Internet endpoint address  */
    int    s;               /* socket descriptor             */

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;

    /* Map port number (char string) to port number (int) */
    if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
        errexit("can't get \"%s\" port number\n", portnum);

    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        errexit("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        fprintf(stderr, "can't bind to %s port: %s; Trying other port\n", portnum, strerror(errno));
        sin.sin_port = htons(0); /* request a port number to be allocated by bind */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
            errexit("can't bind: %s\n", strerror(errno));
        else
        {
            int socklen = sizeof(sin);

            if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
                errexit("getsockname: %s\n", strerror(errno));
            printf("New server port number is %d\n", ntohs(sin.sin_port));
        }
    }
    return s;
}