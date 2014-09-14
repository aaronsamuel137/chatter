#include "chatutilfunctions.h"

int main(int argc, char**argv)
{
    int sockfd, session_sock, n;
    struct sockaddr_in servaddr, sessionaddr;
    char sendline[MESSAGE_LENGTH];
    char recvline[MESSAGE_LENGTH];

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
        if (strncmp("Start ", sendline, START_LEN) == 0 || strncmp("Find ", sendline, FIND_LEN) == 0)
        {
            sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
            n = recvfrom(sockfd, recvline, MESSAGE_LENGTH, 0, NULL, NULL);
            recvline[n] = 0;

            if (atoi(recvline) == 0)
            {
                printf("Error: chatroom not found\n");
                continue;
            }

            sessionaddr.sin_family = AF_INET;
            sessionaddr.sin_addr.s_addr = inet_addr(argv[1]);
            sessionaddr.sin_port = htons(atoi(recvline));

            session_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

            if (connect(session_sock, (struct sockaddr *)&sessionaddr, sizeof(sessionaddr)) < 0)
                errexit("can't connect to %s: %s\n", atoi(recvline), strerror(errno));

            memset(&recvline, 0, sizeof(recvline));
            n = recvfrom(sockfd, recvline, MESSAGE_LENGTH, 0, NULL, NULL);
            recvline[n] = 0;
            printf("%s", recvline);

            memset(&sendline, 0, sizeof(sendline));
            memset(&recvline, 0, sizeof(recvline));

            printf("ME > ");
            while (fgets(sendline, MESSAGE_LENGTH, stdin) != NULL)
            {
                send(session_sock, sendline, strlen(sendline), 0);
                printf("Sent: %s", sendline);
                memset(&sendline, 0, sizeof(sendline));
                printf("ME > ");
            }

            // n = recvfrom(sockfd, recvline, MESSAGE_LENGTH, 0, NULL, NULL);
            // recvline[n] = 0;
        }
        // else if (strncmp("Find ", sendline, FIND_LEN) == 0)
        // {

        // }
        else if (strncmp("Terminate ", sendline, TERMINATE_LEN) == 0)
        {

        }
        else
        {
            sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        }
        n = recvfrom(sockfd, recvline, MESSAGE_LENGTH, 0, NULL, NULL);
        recvline[n] = 0;
        fputs(recvline, stdout);
    }
}
