#include "chatutilfunctions.h"

extern int errno;

#define QLEN 32 // maximum connection queue length
#define LOGGING false

void reply(int upd_sock, sockaddr_in &cliaddr, std::string reply_str);
int updSocket(char portnum[]);
int sessionSocket(int upd_sock, sockaddr_in upd_cliaddr, char *udp_portnum, std::string s_name);

int main(int argc, char**argv)
{
    int upd_sock, session_sock, session_portnum, n;
    struct sockaddr_in cliaddr;
    socklen_t len;
    char mesg[MESSAGE_LENGTH];

    char udp_portnum[6] = "32000";
    std::map<std::string, int> ports;

    std::string mesg_str, s_name, reply_str;

    upd_sock = updSocket(udp_portnum);
    if (LOGGING) printf("Starting UDP socket with number %d\n", upd_sock);

    for (;;)
    {
        memset(&mesg, 0, sizeof(mesg));

        len = sizeof(cliaddr);
        n = recvfrom(upd_sock, mesg, MESSAGE_LENGTH, 0, (struct sockaddr *)&cliaddr, &len);

        Reader reader(mesg, n);
        mesg_str = reader.next_word();

        if (mesg_str.compare(0, 5, "Start") == 0)
        {
            s_name = reader.next_line();

            if (ports.count(s_name) == 1)
            {
                if (LOGGING) printf("Error chatroom already exists\n");
                reply(upd_sock, cliaddr, "-1\0");
                continue;
            }

            int portnum = sessionSocket(upd_sock, cliaddr, udp_portnum, s_name);
            if (portnum)
            {
                ports[s_name] = portnum;
                if (LOGGING) printf("Added \"%s\" -> %d to ports maps\n", s_name.c_str(), ports[s_name]);
            }
        }
        else if (mesg_str.compare(0, 4, "Find") == 0)
        {
            s_name = reader.next_line();

            if (LOGGING) printf("Searching for chatroom %s\n", s_name.c_str());

            if (ports.count(s_name) == 0)
            {
                if (LOGGING) printf("Error: no chatroom exists with name \"%s\"", s_name.c_str());
                reply(upd_sock, cliaddr, "-1\0");
            }
            else
            {
                if (LOGGING) printf("chatroom %s on port %d\n", s_name.c_str(), ports[s_name]);
                reply(upd_sock, cliaddr, to_string(ports[s_name]));
            }
        }
        else if (mesg_str.compare(0, 9, "Terminate") == 0)
        {
            s_name = reader.next_line();

            for (std::map<std::string, int>::iterator it = ports.begin(); it != ports.end(); it++)
            {
                if (s_name.compare(it->first) == 0)
                {
                    if (LOGGING) printf("Terminating chatroom %s\n", (it->first).c_str());
                    ports.erase(it);
                    break;
                }
            }
        }
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

int updSocket(char portnum[])
/*
 * Arguments:
 *      portnum - port number of the server
 */
{
    struct sockaddr_in sin; /* an Internet endpoint address  */
    int    s;               /* socket descriptor             */

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;

    /* Map port number (char string) to port number (int) */
    if ((sin.sin_port = htons((unsigned short)atoi(portnum))) == 0)
        errexit("can't get \"%s\" port number\n", portnum);

    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        errexit("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        fprintf(stderr, "can't bind to default port %s: %s; Trying other port\n", portnum, strerror(errno));
        sin.sin_port = htons(0); /* request a port number to be allocated by bind */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
            errexit("can't bind: %s\n", strerror(errno));
        else
        {
            socklen_t socklen = sizeof(sin);

            if (getsockname(s, (struct sockaddr *)&sin, &socklen) < 0)
                errexit("getsockname: %s\n", strerror(errno));
        }
        std::string new_port = to_string(ntohs(sin.sin_port));
        int i = 0;
        for (std::string::iterator it = new_port.begin(); it != new_port.end(); it++)
        {
            printf("%c\n", *it);
            portnum[i++] = *it;
        }
        portnum[i] = '\0';
    }
    printf("Server running on port: %d\n", ntohs(sin.sin_port));
    return s;
}

/*------------------------------------------------------------------------
 * sessionSocket - allocate & bind a server socket using TCP
 *------------------------------------------------------------------------
 */
int sessionSocket(int upd_sock, sockaddr_in upd_cliaddr, char *udp_portnum, std::string s_name)
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
        if (LOGGING) printf("Starting TCP socket on port %d\n", ntohs(sin.sin_port));
    }

    if (listen(s, QLEN) < 0)
        errexit("can't listen: %s\n", strerror(errno));

    pid_t pid = fork();
    int portnum = ntohs(sin.sin_port);

    // child process
    if (pid == 0)
    {
        // send the TCP port number back to client
        std::string port_str = to_string(portnum);
        port_str.copy(portnum_reply, port_str.size(), 0);
        portnum_reply[port_str.size()] = '\0';
        sendto(upd_sock, portnum_reply, strlen(portnum_reply), 0, (struct sockaddr *)&upd_cliaddr, sizeof(upd_cliaddr));

        execl("./chat_server",
            "chat_server",
            to_string(s).c_str(),
            udp_portnum,
            s_name.c_str(),
            (char*)NULL
        );

        perror("execl() failure!");

        exit(1);
    }
    // parent process
    else
    {
        return portnum;
    }
}
