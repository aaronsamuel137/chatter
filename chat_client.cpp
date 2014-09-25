#include "chatutilfunctions.h"

#include <netdb.h>

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif  /* INADDR_NONE */

#define LOGGING false

extern int errno;

int send_upd(int upd_sock, sockaddr_in servaddr, char sendline[], char recvline[]);
int connect_to_socket(sockaddr_in addr);
void leave(int s, std::string s_name);

int main(int argc, char**argv)
{
    int sockfd, session_sock, n, portnum, i, num_messages, messages_received, message_length;
    struct sockaddr_in servaddr, sessionaddr;
    struct hostent *phe;    // pointer to host information entry
    char sendline[MESSAGE_LENGTH];
    char recvline[MESSAGE_LENGTH];
    std::string send_str, s_name, message;
    s_name = "";

    const char* host;
    const char* port;

    switch (argc)
    {
    case 1:
        host = "localhost";
        port = "32000";
        break;

    case 2:
        host = "localhost";
        port = argv[1];
        break;

    case 3:
        host = argv[1];
        port = argv[2];
        break;

    default:
        printf("Usage: chat_client [server ip] [server port number] OR chat_client [server port number] (assuming localhost) \n");
        exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    memset(&sessionaddr, 0, sizeof(sessionaddr));
    sessionaddr.sin_family = AF_INET;

    // Map port number (char string) to port number (int)
    if ((servaddr.sin_port = htons((unsigned short)atoi(port))) == 0)
        errexit("can't get \"%s\" port number\n", port);

    // Map host name to IP address, allowing for dotted decimal
    if ( (phe = gethostbyname(host)) )
    {
        memcpy(&servaddr.sin_addr, phe->h_addr, phe->h_length);
        memcpy(&sessionaddr.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ( (servaddr.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
        errexit("can't get \"%s\" host entry\n", host);

    // Allocate a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        errexit("can't create socket: %s\n", strerror(errno));

    // main loop for getting text from client
    while (fgets(sendline, MESSAGE_LENGTH, stdin) != NULL)
    {
        memset(&recvline, 0, sizeof(recvline));

        Reader reader(sendline, strlen(sendline));
        send_str = reader.next_word();

        if (strncmp(sendline, "Start", 5) == 0)
        {
            s_name = reader.next_line();
            if (s_name.size() > 8)
            {
                printf("Invalid chat session name. Session name must be at most 8 characters long\n");
                s_name = "";
                continue;
            }

            if (LOGGING) printf("sending start: %s\n", sendline);

            portnum = send_upd(sockfd, servaddr, sendline, recvline);
            sessionaddr.sin_port = htons(portnum);
            if (portnum == -1)
                printf("Error starting chatroom. Chatroom name is already in use.\n");
            else
            {
                session_sock = connect_to_socket(sessionaddr);
                printf("A new chat session %s has been created and you have joined this session\n", s_name.c_str());
            }
        }
        else if (strncmp(sendline, "Join", 4) == 0)
        {
            s_name = reader.next_line();
            if (s_name.size() > 8)
            {
                printf("Invalid chat session name. Session name must be at most 8 characters long\n");
                s_name = "";
                continue;
            }

            send_str = "Find " + s_name;
            strncpy(sendline, send_str.c_str(), sizeof(sendline));
            portnum = send_upd(sockfd, servaddr, sendline, recvline);
            sessionaddr.sin_port = htons(portnum);
            if (portnum == -1)
                printf("Error joining chatroom\n");
            else
            {
                session_sock = connect_to_socket(sessionaddr);
                printf("You have joined the chat session %s\n", s_name.c_str());
            }
        }
        else if (strncmp(sendline, "Submit", 6) == 0)
        {
            message_length = reader.next_int();
            if (message_length == 0)
                printf("Error: must enter number of bytes sent.\nCorrect usage: Submit <message length> <message>\n");
            else
            {
                message = reader.next_n(message_length);
                if (message.size() > 79)
                {
                    printf("Invalid message. Messages must be at most 80 characters\n");
                    memset(&recvline, 0, sizeof(recvline));
                    continue;
                }

                n = send(session_sock, sendline, strlen(sendline), 0);
                if (n < 0)
                {
                    printf("Error sending message with Submit: %s. Have you started or joined a chat session?\n", strerror(errno));
                    continue;
                }

                if (LOGGING) printf("sent message: %s\n", message.c_str());
            }
        }
        else if (strncmp(sendline, "GetNext", 7) == 0)
        {
            send_str = "GetNext";
            strncpy(sendline, send_str.c_str(), sizeof(sendline));
            if (send(session_sock, sendline, strlen(sendline), 0) < 0)
            {
                printf("Error sending message: %s. Have you started or joined a chat session?\n", strerror(errno));
                continue;
            }
            if (recv(session_sock, recvline, sizeof(recvline), 0) < 0)
            {
                printf("Error receiving message: %s. Have you started or joined a chat session?\n", strerror(errno));
                continue;
            }

            if (LOGGING) printf("%s\n", recvline);

            reader = Reader(recvline, n);
            message_length = reader.next_int();
            message = reader.next_n(message_length);

            if (LOGGING) printf("message len: %d\n", message_length);

            if (message_length <= 0)
            {
                printf("No new message in the chat session\n");
                continue;
            }
            else
            {
                printf("%s\n", message.c_str());
            }
        }
        else if (strncmp(sendline, "GetAll", 6) == 0)
        {
            send_str = "GetAll";
            strncpy(sendline, send_str.c_str(), sizeof(sendline));
            if (send(session_sock, sendline, strlen(sendline), 0) < 0)
            {
                printf("Error sending message: %s. Have you started or joined a chat session?\n", strerror(errno));
                continue;
            }

            memset(&recvline, 0, sizeof(recvline));
            n = recv(session_sock, recvline, sizeof(recvline), 0);
            reader = Reader(recvline, n);

            num_messages = reader.next_int();
            if (num_messages <= 0)
            {
                printf("No new messages in the chat session\n");
                continue;
            }

            messages_received = 0;

            if (LOGGING) printf("getting %d messages\n", num_messages);

            // finish reading the current buffer in case there are messages there
            while (reader.get_index() < n)
            {
                message_length = reader.next_int();
                message = reader.next_n(message_length);
                printf("%s\n", message.c_str());
                messages_received++;
            }

            while (messages_received < num_messages)
            {
                memset(&recvline, 0, sizeof(recvline));
                n = recv(session_sock, recvline, sizeof(recvline), 0);
                reader = Reader(recvline, n);
                while (reader.get_index() < n)
                {
                    message_length = reader.next_int();
                    message = reader.next_n(message_length);
                    printf("%s\n", message.c_str());
                    messages_received++;
                }
            }

            if (LOGGING) printf("All messages got\n");
        }
        else if (strncmp(sendline, "Leave", 5) == 0)
        {
            leave(session_sock, s_name);
            s_name = ""; // reset s_name to be an empty string
        }
        else if (strncmp(sendline, "Exit", 4) == 0)
        {
            printf("Bye!\n");
            exit(0);
        }
        else
        {
            printf("Invalid command: %s", sendline);
        }
        memset(&sendline, 0, sizeof(sendline));
    }
}

void leave(int s, std::string s_name)
{
    char leave_str[6] = "Leave";
    if (send(s, leave_str, strlen(leave_str), 0) < 0)
        printf("Error sending Leave message %s\n", strerror(errno));
    else
    {
        close(s);
        printf("You have left the chat session %s\n", s_name.c_str());
    }
}

int send_upd(int upd_sock, sockaddr_in servaddr, char sendline[], char recvline[])
{
    if (sendto(upd_sock, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        printf("Error with sendto %s\n", strerror(errno));
    int n = recvfrom(upd_sock, recvline, MESSAGE_LENGTH, 0, NULL, NULL);
    if (n < 0)
        printf("Error calling recvfrom %s\n", strerror(errno));
    if (LOGGING) printf("received port: %s\n", recvline);
    recvline[n] = '\0';
    return atoi(recvline);
}

int connect_to_socket(sockaddr_in addr)
{
    int session_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (connect(session_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        errexit("can't connect to port %d, %s\n", addr.sin_port, strerror(errno));

    return session_sock;
}
