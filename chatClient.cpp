#include "chatutilfunctions.h"

int send_upd(int upd_sock, sockaddr_in servaddr, char sendline[], char recvline[]);
int connect_to_socket(sockaddr_in servaddr, int portnum);

int main(int argc, char**argv)
{
    int sockfd, session_sock, n, portnum;
    struct sockaddr_in servaddr;
    char sendline[MESSAGE_LENGTH];
    char recvline[MESSAGE_LENGTH];
    std::string send_str, s_name, message;

    if (argc != 2)
    {
        printf("usage: client <IP address>\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(32000);

    while (fgets(sendline, MESSAGE_LENGTH, stdin) != NULL)
    {
        memset(&recvline, 0, sizeof(recvline));

        send_str = std::string(sendline);

        if (send_str.compare(0, 6, "Start ") == 0)
        {
            s_name = get_message(send_str, 6);
            portnum = send_upd(sockfd, servaddr, sendline, recvline);
            if (portnum == -1)
                printf("Error starting chatroom");
            else
            {
                session_sock = connect_to_socket(servaddr, portnum);
                printf("A new chat session %s has been created and you have joined this session\n", s_name.c_str());
            }
        }
        else if (send_str.compare(0, 5, "Join ") == 0)
        {
            s_name = get_message(send_str, 5);
            send_str = "Find " + s_name;
            strncpy(sendline, send_str.c_str(), sizeof(sendline));
            portnum = send_upd(sockfd, servaddr, sendline, recvline);
            if (portnum == -1)
                printf("Error joining chatroom");
            else
            {
                session_sock = connect_to_socket(servaddr, portnum);
                printf("You have joined the chat session %s\n", s_name.c_str());
            }
        }
        else if (send_str.compare(0, 7, "Submit ") == 0)
        {
            // send(session_sock, sendline, strlen(sendline), 0);
            if (send(session_sock, sendline, strlen(sendline), 0) < 0)
                printf("Error sending message %s\n", strerror(errno));
            // printf("Sent: %s", sendline);
            // printf("%s > ", s_name.c_str());
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
            /* code */
        }
        else if (send_str.compare(0, 5, "Leave") == 0)
        {
            /* code */
        }
        else if (send_str.compare(0, 4, "Exit ") == 0)
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

int send_upd(int upd_sock, sockaddr_in servaddr, char sendline[], char recvline[])
{
    sendto(upd_sock, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    int n = recvfrom(upd_sock, recvline, MESSAGE_LENGTH, 0, NULL, NULL);
    recvline[n] = 0;
    return atoi(recvline);
}

int connect_to_socket(sockaddr_in servaddr, int portnum)
{
    // struct sockaddr_in sessionaddr;
    // sessionaddr.sin_family = AF_INET;
    // sessionaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(portnum);
    int session_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (connect(session_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        errexit("can't connect to %s\n", portnum, strerror(errno));

    return session_sock;
    // memset(&recvline, 0, sizeof(recvline));
    // n = recvfrom(upd_sock, recvline, MESSAGE_LENGTH, 0, NULL, NULL);
    // recvline[n] = 0;
    // printf("%s", recvline);

    // memset(&sendline, 0, sizeof(sendline));
    // memset(&recvline, 0, sizeof(recvline));
}
