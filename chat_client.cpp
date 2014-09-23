#include "chatutilfunctions.h"

extern int errno;

int send_upd(int upd_sock, sockaddr_in servaddr, char sendline[], char recvline[]);
int connect_to_socket(sockaddr_in addr);
void leave(int s, std::string s_name);

int main(int argc, char**argv)
{
    int sockfd, session_sock, n, portnum, i, num_messages, messages_received, mesg_len;
    struct sockaddr_in servaddr, sessionaddr;
    char sendline[MESSAGE_LENGTH];
    char recvline[MESSAGE_LENGTH];
    std::string send_str, s_name, message;
    s_name = "";

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    memset(&sessionaddr, 0, sizeof(sessionaddr));
    sessionaddr.sin_family = AF_INET;
    sessionaddr.sin_addr.s_addr = inet_addr(argv[1]);

    while (fgets(sendline, MESSAGE_LENGTH, stdin) != NULL)
    {
        memset(&recvline, 0, sizeof(recvline));

        Reader reader(sendline, strlen(sendline));
        send_str = reader.next_word();

        if (send_str == "Start")
        {
            if (s_name != "")
                leave(session_sock, s_name);

            s_name = reader.next_line();
            if (s_name.size() > 8)
            {
                printf("Invalid chat session name. Session name must be at most 8 characters long\n");
                s_name = "";
                continue;
            }

            printf("sending start: %s\n", sendline);
            portnum = send_upd(sockfd, servaddr, sendline, recvline);
            sessionaddr.sin_port = htons(portnum);
            printf("Port %d\n", portnum);
            if (portnum == -1)
                printf("Error starting chatroom\n");
            else
            {
                session_sock = connect_to_socket(sessionaddr);
                printf("A new chat session %s has been created and you have joined this session\n", s_name.c_str());
            }
        }
        else if (send_str == "Join")
        {
            if (s_name != "")
                leave(session_sock, s_name);

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
        else if (send_str == "Submit")
        {
            mesg_len = reader.next_int();
            if (mesg_len == 0)
                printf("Error: must enter number of bytes sent.\nCorrect usage: Submit <message length> <message>\n");
            else
            {
                message = reader.next_line();
                if (message.size() > 79)
                {
                    printf("Invalid message. Messages must be at most 80 characters\n");
                    memset(&recvline, 0, sizeof(recvline));
                    continue;
                }

                n = send(session_sock, sendline, strlen(sendline), 0);
                if (n < 0)
                    printf("Error sending message with Submit: %s. Have you started or joined a chat session?\n", strerror(errno));
                printf("message: %s\n", message.c_str());
            }
        }
        else if (send_str.compare(0, 7, "GetNext") == 0)
        {
            send_str = "GetNext";
            strncpy(sendline, send_str.c_str(), sizeof(sendline));
            if (send(session_sock, sendline, strlen(sendline), 0) < 0)
                printf("Error sending message %s\n", strerror(errno));
            if (recv(session_sock, recvline, sizeof(recvline), 0) < 0)
                printf("Error receiving message %s\n", strerror(errno));
            printf("%s\n", recvline);
        }
        else if (send_str.compare(0, 6, "GetAll") == 0)
        {
            send_str = "GetAll";
            strncpy(sendline, send_str.c_str(), sizeof(sendline));
            if (send(session_sock, sendline, strlen(sendline), 0) < 0)
                printf("Error sending message %s\n", strerror(errno));

            memset(&recvline, 0, sizeof(recvline));
            n = recv(session_sock, recvline, sizeof(recvline), 0);
            reader = Reader(recvline, n);

            num_messages = reader.next_int();
            messages_received = 0;

            printf("getting %d messages\n", num_messages);

            // finish reading the current buffer in case there are messages there
            while (reader.get_index() < n)
            {
                mesg_len = reader.next_int();
                message = reader.next_line();
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
                    mesg_len = reader.next_int();
                    message = reader.next_line();
                    printf("%s\n", message.c_str());
                    messages_received++;
                }
            }
            printf("All messages got\n");
        }
        else if (send_str.compare(0, 5, "Leave") == 0)
        {
            s_name = ""; // reset s_name to be an empty string
            leave(session_sock, s_name);
        }
        else if (send_str.compare(0, 4, "Exit") == 0)
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
    printf("received port: %s\n", recvline);
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
