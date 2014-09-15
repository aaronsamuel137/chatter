#include "chatutilfunctions.h"
#include <map>
#include <sys/select.h>
#include <sys/time.h>

#define QLEN 32 // maximum connection queue length

extern int errno;

void reply(int upd_sock, sockaddr_in &cliaddr, std::string reply_str);
int updSocket(const char *portnum);
int sessionSocket(int upd_sock, sockaddr_in upd_cliaddr, std::string s_name);
int serveSession(int msock);
int handle_message(int fd, std::map<int, int> &last_read, std::map<int, std::string> &messages, int &message_index);

int main(int argc, char**argv)
{
    int upd_sock, session_sock, session_portnum, n;
    struct sockaddr_in cliaddr;
    socklen_t len;
    char mesg[MESSAGE_LENGTH];

    char *udp_portnum = "32000";
    std::map<std::string, int> ports;

    std::string mesg_str, s_name, reply_str;

    upd_sock = updSocket(udp_portnum);
    printf("Starting UDP socket with number %d\n", upd_sock);

    for (;;)
    {
        // message buffers
        memset(&mesg, 0, sizeof(mesg));

        len = sizeof(cliaddr);
        n = recvfrom(upd_sock, mesg, MESSAGE_LENGTH, 0, (struct sockaddr *)&cliaddr, &len);

        mesg_str = std::string(mesg);

        printf("Got communication: %s", mesg);

        if (mesg_str.compare(0, 6, "Start ") == 0)
        {
            s_name = get_message(mesg_str, 6);

            int portnum = sessionSocket(upd_sock, cliaddr, s_name);
            if (portnum)
            {
                ports[s_name] = portnum;
                if (ports.count(s_name) == 1)
                    printf("Added \"%s\" -> %d to ports maps\n", s_name.c_str(), ports[s_name]);
            }
            else
            {
                // only the child process session server will reach this point
                exit(0);
            }
        }
        else if (mesg_str.compare(0, 5, "Find ") == 0)
        {
            s_name = get_message(mesg_str, 5);

            printf("Searching for chatroom %s\n", s_name.c_str());

            // for(std::map<std::string,int>::iterator it = ports.begin(); it != ports.end(); it++)
            // {
            //     printf("%s -> %d\n", it->first.c_str(), it->second);
            // }

            if (ports.count(s_name) == 0)
            {
                printf("Error: no chatroom exists with name \"%s\"", s_name.c_str());
                reply(upd_sock, cliaddr, "0\0");
            }
            else
            {
                reply(upd_sock, cliaddr, std::to_string(ports[s_name]));
                reply(upd_sock, cliaddr, "Welcome to chatroom " + s_name + "\n");
                printf("chatroom %s on port %d\n", s_name.c_str(), ports[s_name]);
            }
        }
        else if (mesg_str.compare(0, 10, "Terminate ") == 0)
        {
            s_name = get_message(mesg_str, 10);

            reply_str = "Terminating chat room " + s_name;
            printf("Terminating chatroom \"%s\"\n", s_name.c_str());
        }
        else
        {
            reply_str = "Invalid command. Commands must beign with Start, Find or Terminate\n";
            reply(upd_sock, cliaddr, reply_str);
        }
        printf("Ending loop\n");
    }
}

void reply(int upd_sock, sockaddr_in &cliaddr, std::string reply_str)
{
    char reply[MESSAGE_LENGTH];
    memset(&reply, 0, sizeof(reply));
    reply_str.copy(reply, reply_str.size(), 0);
    reply[MESSAGE_LENGTH-1] = '\0';
    sendto(upd_sock, reply, strlen(reply), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
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

/*------------------------------------------------------------------------
 * sessionSocket - allocate & bind a server socket using TCP
 *------------------------------------------------------------------------
 */
int sessionSocket(int upd_sock, sockaddr_in upd_cliaddr, std::string s_name)
{
    struct sockaddr_in sin;
    struct sockaddr* tcp_cliaddr;
    int    s;
    char portnum_reply[8];
    char mesg[MESSAGE_LENGTH];

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(0); /* request a port number to be allocated by bind */

    /* Allocate a socket */
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0)
        errexit("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit("can't bind: %s\n", strerror(errno));
    else {
        socklen_t socklen = sizeof(sin);

        if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
            errexit("getsockname: %s\n", strerror(errno));
        printf("Starting TCP socket on port %d\n", ntohs(sin.sin_port));
    }

    pid_t pid = fork();
    int portnum = ntohs(sin.sin_port);

    // child process
    if (pid == 0)
    {
        if (listen(s, QLEN) < 0)
            errexit("can't listen on %s port: %s\n", portnum, strerror(errno));

        // send the TCP port number back to client
        std::string port_str = std::to_string(portnum);
        port_str.copy(portnum_reply, port_str.size(), 0);
        sendto(upd_sock, portnum_reply, strlen(portnum_reply), 0, (struct sockaddr *)&upd_cliaddr, sizeof(upd_cliaddr));
        reply(upd_sock, upd_cliaddr, "Welcome to chatroom " + s_name + "\n");

        serveSession(s);

        return 0;
    }
    // parent process
    else
    {
        return portnum;
    }
}

int serveSession(int msock)
{
    struct sockaddr_in fsin;    /* the from address of a client */
    fd_set rfds;                /* read file descriptor set */
    fd_set afds;                /* active file descriptor set   */
    unsigned int alen;          /* from-address length      */
    int fd, nfds;
    int message_index = 0;

    printf("Starting session with socket %d\n", msock);

    std::map<int, std::string> messages;
    std::map<int, int> last_read;

    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(msock, &afds);

    while (1) {
        memcpy(&rfds, &afds, sizeof(rfds));

        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)
            errexit("select: %s\n", strerror(errno));
        if (FD_ISSET(msock, &rfds)) {
            int ssock;

            alen = sizeof(fsin);
            ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
            if (ssock < 0)
                errexit("accept: %s\n", strerror(errno));
            last_read[ssock] = 0;
            printf("Accepted socket %d\n", ssock);
            FD_SET(ssock, &afds);
        }

        // OSX workaround, can't have more than 1024 fds
        for (fd = 0; fd < 1024; ++fd)
        // for (fd = 0; fd < nfds; ++fd)
        {
            if (fd != msock && FD_ISSET(fd, &rfds))
            {
                printf("Calling handle_message with socket %d\n", fd);
                if (handle_message(fd, last_read, messages, message_index) == 0) {
                    (void) close(fd);
                    FD_CLR(fd, &afds);
                }
                // printf("Printing messages:\n");
                // for(std::map<int,std::string>::iterator it = messages.begin(); it != messages.end(); it++)
                // {
                //     printf("%d -> %s\n", it->first, it->second.c_str());
                // }
            }
        }
    }
}

int handle_message(int fd, std::map<int, int> &last_read, std::map<int, std::string> &messages, int &message_index)
{
    char sendline[MESSAGE_LENGTH];
    char recvline[MESSAGE_LENGTH];
    char digit_buffer[4];
    memset(&sendline, 0, sizeof(sendline));
    memset(&recvline, 0, sizeof(recvline));

    std::string message;
    int index, message_size, i;

    if (recv(fd, recvline, sizeof(recvline), 0) < 0)
        printf("Error receiving message %s\n", strerror(errno));

    printf("Got message: %s\n", recvline);

    std::string mesg_str = std::string(recvline);

    if (mesg_str.compare(0, 7, "Submit ") == 0)
    {
        memset(&digit_buffer, 0, sizeof(digit_buffer));
        for (i = 7; i < strlen(recvline); i++)
        {
            if (isdigit(recvline[i]))
                digit_buffer[i - 7] = recvline[i];
            else
                break;
        }
        messages[message_index++] = get_message(recvline, i + 1);
        printf("Got message: %s with size: %d\n", messages[message_index-1].c_str(), atoi(digit_buffer));
    }
    else if (mesg_str.compare(0, 7, "GetNext") == 0)
    {
        index = last_read[fd]++;
        message = messages[index];
        strncpy(sendline, message.c_str(), sizeof(sendline));
        send(fd, sendline, strlen(sendline), 0);
    }
    else if (mesg_str.compare(0, 6, "GetAll") == 0)
    {
        index = last_read[fd];
        while (messages.count(index) == 1)
        {
            message = messages[index];
            strncpy(sendline, message.c_str(), sizeof(sendline));
            send(fd, sendline, strlen(sendline), 0);
            index++;
            memset(&sendline, 0, sizeof(sendline));
        }
        last_read[fd] = index;

        message = "\0";
        strncpy(sendline, message.c_str(), sizeof(sendline));
        int n = send(fd, sendline, strlen(sendline) + 1, 0);
        printf("%d\n", n);
        if (n < 0)
            printf("Error sending message %s\n", strerror(errno));

        printf("DONE\n");
    }
    else if (mesg_str.compare(0, 5, "Leave") == 0)
    {
    }
    else
    {
        // reply_str = "Invalid command. Commands must beign with Start, Find or Terminate\n";
        // reply(upd_sock, cliaddr, reply_str);
    }

    return 1;

    // std::string message_str = std::string(buf);
    // messages[message_index++] = message_str.erase(message_str.find_last_not_of(" \n\r\t")+1);
}

