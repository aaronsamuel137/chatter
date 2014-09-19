#include "chatutilfunctions.h"

int send_upd(int upd_sock, sockaddr_in servaddr, char sendline[], char recvline[]);
int connect_to_socket(sockaddr_in servaddr, int portnum);

int main(int argc, char**argv)
{
    int sockfd, session_sock, n, portnum, i, num_messages;
    struct sockaddr_in servaddr;
    char sendline[MESSAGE_LENGTH];
    char recvline[MESSAGE_LENGTH];
    char digit_buffer[4];           // for holding message length
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
        clear_array(recvline);
        clear_array(sendline);
        clear_array(digit_buffer);

        send_str = std::string(sendline);

        if (send_str.compare(0, 6, "Start ") == 0)
        {
            s_name = get_message(send_str, 6);
            portnum = send_upd(sockfd, servaddr, sendline, recvline);
            printf("Port %d\n", portnum);
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
            // make sure that a message length is included
            memset(&digit_buffer, 0, sizeof(digit_buffer));
            for (i = 7; i < strlen(sendline); i++)
            {
                if (isdigit(sendline[i]))
                    digit_buffer[i - 7] = sendline[i];
                else
                    break;
            }
            if (atoi(digit_buffer) == 0)
                printf("Error: must enter number of bytes sent.\nCorrect usage: Submit <message length> <message>\n");
            else
            {
                message = get_message(send_str, i + 1);
                printf("message: %s\n", message.c_str());

                if (send(session_sock, sendline, strlen(sendline), 0) < 0)
                    printf("Error sending message %s\n", strerror(errno));
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

            clear_array(recvline);
            n = recv(session_sock, recvline, sizeof(recvline), 0);
            Reader reader(recvline, n);

            int num_messages = reader.next_int();
            int messages_received = 0;

            printf("getting %d messages\n", num_messages);

            if (reader.get_index() >= n)
            {
                clear_array(recvline);
                n = recv(session_sock, recvline, sizeof(recvline), 0);
                reader = Reader(recvline, n);
            }
            while (reader.get_index() < n)
            {
                int mesg_len = reader.next_int();
                std::string message = reader.next_line();
                printf("%s\n", message.c_str());
                messages_received++;
                if (messages_received < num_messages)
                {
                    clear_array(recvline);
                    n = recv(session_sock, recvline, sizeof(recvline), 0);
                    reader = Reader(recvline, n);
                }
            }
            printf("All messages got\n");
        }
        else if (send_str.compare(0, 5, "Leave") == 0)
        {
            send_str = "Leave";
            strncpy(sendline, send_str.c_str(), sizeof(sendline));
            if (send(session_sock, sendline, strlen(sendline), 0) < 0)
                printf("Error sending Leave message %s\n", strerror(errno));
            else {
                close(session_sock);
                printf("You have left the chat session %s\n", s_name.c_str());
            }
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
    }
}

int send_upd(int upd_sock, sockaddr_in servaddr, char sendline[], char recvline[])
{
    if (sendto(upd_sock, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        printf("Error with sendto %s\n", strerror(errno));
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
}
