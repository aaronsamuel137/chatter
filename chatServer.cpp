#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string>
#include <cstdlib>

#include <sys/errno.h>
#include <stdarg.h>

#define MESSAGE_LENGTH 256
#define START_LEN      6
#define FIND_LEN       5
#define TERMINATE_LEN  10

extern int errno;

// using namespace std;

int errexit(const char *format, ...);
int updSocket(const char *portnum);

int main(int argc, char**argv)
{
    int sockfd, n;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char mesg[MESSAGE_LENGTH];
    char reply[MESSAGE_LENGTH];
    char *portnum = "32000";
    std::string mesg_str, reply_str, s_name;

    sockfd = updSocket(portnum);

    for (;;)
    {
        // clear the reply and message buffers
        memset(&reply, 0, sizeof(reply));
        memset(&mesg, 0, sizeof(mesg));

        len = sizeof(cliaddr);
        n = recvfrom(sockfd, mesg, MESSAGE_LENGTH, 0, (struct sockaddr *)&cliaddr, &len);

        mesg_str = std::string(mesg);

        if (strncmp("Start ", mesg, START_LEN) == 0)
        {
            s_name = mesg_str.substr(START_LEN, mesg_str.size());
            reply_str = "Starting chat room " + s_name;

            printf("Got Start command\n");
            printf("chatroom name: %s\n", s_name.c_str());
        }
        else if (strncmp("Find ", mesg, FIND_LEN) == 0)
        {
            s_name = mesg_str.substr(5, mesg_str.size());
            reply_str = "Finding chat room " + s_name;

            printf("Got Find command\n");
            printf("chatroom name: %s\n", s_name.c_str());
        }
        else if (strncmp("Terminate ", mesg, TERMINATE_LEN) == 0)
        {
            s_name = mesg_str.substr(TERMINATE_LEN, mesg_str.size());
            reply_str = "Terminating chat room " + s_name;

            printf("Got Terminate command\n");
            printf("chatroom name: %s\n", s_name.c_str());
        }
        else
        {
            reply_str = "Invalid command. Commands must beign with Start, Find or Terminate\n\0";
        }

        reply_str.copy(reply, reply_str.size(), 0);
        reply[MESSAGE_LENGTH-1] = '\0';
        n = strlen(reply);
        sendto(sockfd, reply, n, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
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
            socklen_t socklen = sizeof(sin);

            if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
                errexit("getsockname: %s\n", strerror(errno));
            printf("New server port number is %d\n", ntohs(sin.sin_port));
        }
    }
    return s;
}