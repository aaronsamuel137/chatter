#include "chatutilfunctions.h"

extern int errno;

void reply(int upd_sock, sockaddr_in &cliaddr, std::string reply_str);
int updSocket(const char *portnum);
int sessionSocket(int upd_sock, sockaddr_in upd_cliaddr, char *udp_portnum);
void recvfrom_session(int session_port, int upd_sock);

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
        clear_array(mesg);

        len = sizeof(cliaddr);
        n = recvfrom(upd_sock, mesg, MESSAGE_LENGTH, 0, (struct sockaddr *)&cliaddr, &len);

        Reader reader(mesg, n);
        mesg_str = reader.next_word();

        if (mesg_str.compare(0, 5, "Start") == 0)
        {
            s_name = reader.next_line();

            int portnum = sessionSocket(upd_sock, cliaddr, udp_portnum);
            if (portnum)
            {
                ports[s_name] = portnum;
                printf("Added \"%s\" -> %d to ports maps\n", s_name.c_str(), ports[s_name]);
            }
        }
        else if (mesg_str.compare(0, 4, "Find") == 0)
        {
            s_name = reader.next_line();

            printf("Searching for chatroom %s\n", s_name.c_str());

            if (ports.count(s_name) == 0)
            {
                printf("Error: no chatroom exists with name \"%s\"", s_name.c_str());
                reply(upd_sock, cliaddr, "-1\0");
            }
            else
            {
                printf("chatroom %s on port %d\n", s_name.c_str(), ports[s_name]);
                reply(upd_sock, cliaddr, std::to_string(ports[s_name]));
            }
        }
        else if (mesg_str.compare(0, 9, "Terminate") == 0)
        {
            printf("cc got Terminate message\n");
            s_name = reader.next_line();
            printf("terminating %s\n", s_name.c_str());

            for (std::map<std::string, int>::iterator it = ports.begin(); it != ports.end(); it++)
            {
                printf("comparing with %s\n", (it->first).c_str());
                if (s_name.compare(it->first) == 0)
                {
                    printf("Terminating chatroom %s\n", (it->first).c_str());
                }
            }

        }
        // printf("Before\n");

        // // recvfrom all session servers in case a Terminate message is sent

        // printf("After\n");
    }
}

void recvfrom_session(int session_port, int upd_sock)
{
    struct sockaddr_in session_address;
    socklen_t len;
    char buffer[16] = {0};

    memset(&session_address, 0, sizeof(session_address));
    session_address.sin_family = AF_INET;
    session_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    session_address.sin_port = htons(session_port);

    len = sizeof(session_address);
    int n = recvfrom(upd_sock, buffer, MESSAGE_LENGTH, 0, (struct sockaddr *)&session_address, &len);
    printf("RECV %s\n", buffer);
}

void reply(int upd_sock, sockaddr_in &cliaddr, std::string reply_str)
{
    char reply[MESSAGE_LENGTH];
    clear_array(reply);
    reply_str.copy(reply, reply_str.size(), 0);
    reply[MESSAGE_LENGTH-1] = '\0';
    sendto(upd_sock, reply, strlen(reply), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
}

int updSocket(const char *portnum)
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
    }
    printf("Server running on port: %d\n", ntohs(sin.sin_port));
    return s;
}

/*------------------------------------------------------------------------
 * sessionSocket - allocate & bind a server socket using TCP
 *------------------------------------------------------------------------
 */
int sessionSocket(int upd_sock, sockaddr_in upd_cliaddr, char *udp_portnum)
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
        // send the TCP port number back to client
        std::string port_str = std::to_string(portnum);
        port_str.copy(portnum_reply, port_str.size(), 0);
        portnum_reply[port_str.size()] = '\0';
        sendto(upd_sock, portnum_reply, strlen(portnum_reply), 0, (struct sockaddr *)&upd_cliaddr, sizeof(upd_cliaddr));

        execl("./chat_server", "chat_server", std::to_string(s).c_str(), std::string(udp_portnum).c_str(), (char*)NULL);
        perror("execl() failure!");

        return 0;
    }
    // parent process
    else
    {
        return portnum;
    }
}
