#include "chatutilfunctions.h"

extern int errno;

#define TIMEOUT 60 // number of seconds until session server terminates due to timeout
#define LOGGING false

int handle_message(int fd, std::map<int, int> &last_read, std::map<int, std::string> &messages, int &message_index);

int main(int argc, char**argv)
{
    struct sockaddr_in fsin;    // the from address of a client
    fd_set rfds;                // read file descriptor set
    fd_set afds;                // active file descriptor set
    unsigned int alen;          // from-address length
    int fd, nfds, upd_sock;
    int message_index = 0;
    struct sockaddr_in servaddr;

    int msock = atoi(argv[1]);
    int chat_coordinator_port = atoi(argv[2]);
    std::string s_name = std::string(argv[3]);

    if (LOGGING) printf("cc port %d\n", chat_coordinator_port);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(chat_coordinator_port);

    if (LOGGING)  printf("Starting session with socket %d and pid %d\n", msock, getpid());

    std::map<int, std::string> messages;
    std::map<int, int> last_read;

#ifdef __APPLE__
    // OSX workaround, can't have more than 1024 fds
    nfds = 1024;
#else
    nfds = getdtablesize();
#endif

    FD_ZERO(&afds);
    FD_SET(msock, &afds);

    Timer timer;

    struct timeval timeout = {TIMEOUT, 0};

    while (1)
    {
        memcpy(&rfds, &afds, sizeof(rfds));

        timer.set();
        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &timeout) < 0)
            errexit("select: %s\n", strerror(errno));

        if (timer.check_seconds_passed(TIMEOUT))
        {
            std::string send_str = "Terminate " + s_name;
            char terminate_buffer[32] = {0};
            strncpy(terminate_buffer, send_str.c_str(), sizeof(terminate_buffer));

            upd_sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (upd_sock < 0)
                errexit("Error creating UPD socket to send Terminate message: %s\n", strerror(errno));
            if (sendto(upd_sock, terminate_buffer, strlen(terminate_buffer), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
                printf("Error sending Terminate message to char coordinator: %s\n", strerror(errno));

            if (LOGGING) printf("session on socket %d terminating due to timeout\n", msock);

            // close sockets with all clients
            for (fd = 0; fd < nfds; ++fd)
            {
                if (fd != msock && FD_ISSET(fd, &afds))
                {
                    close(fd);
                    FD_CLR(fd, &afds);
                }
            }
            close(msock);
            exit(0);
        }

        if (FD_ISSET(msock, &rfds))
        {
            int ssock;

            alen = sizeof(fsin);
            ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
            if (ssock < 0)
                errexit("accept: %s\n", strerror(errno));
            last_read[ssock] = 0;
            if (LOGGING) printf("Accepted socket %d\n", ssock);
            FD_SET(ssock, &afds);
        }

        for (fd = 0; fd < nfds; ++fd)
        {
            if (fd != msock && FD_ISSET(fd, &rfds))
            {
                if (handle_message(fd, last_read, messages, message_index) == 0) {
                    close(fd);
                    FD_CLR(fd, &afds);
                }
            }
        }
    }
}

int handle_message(int fd, std::map<int, int> &last_read, std::map<int, std::string> &messages, int &message_index)
{
    char sendline[MESSAGE_LENGTH] = {0};
    char recvline[MESSAGE_LENGTH] = {0};

    std::string message;
    int index, message_size, i, n, mesg_len;

    n = recv(fd, recvline, sizeof(recvline), 0);

    if (n < 0)
    {
        printf("Error receiving message %s\n", strerror(errno));
        return -1;
    }

    Reader reader(recvline, n);

    while (reader.get_index() < n)
    {
        message = reader.next_word();
        if (LOGGING) printf("word: %s\n", message.c_str());

        if (message.compare(0, 6, "Submit") == 0)
        {
            mesg_len = reader.next_int();
            message = reader.next_n(mesg_len);
            messages[message_index++] = message;
            if (LOGGING) printf("Got message: %s with size: %d\n", message.c_str(), mesg_len);
        }
        else if (message.compare(0, 7, "GetNext") == 0)
        {
            index = last_read[fd];
            if (messages.count(index) == 1)
            {
                message = to_string(messages[index].size()) + " " + messages[index];
                last_read[fd]++;
            }
            else
                message = "-1";
            strncpy(sendline, message.c_str(), sizeof(sendline));
            send(fd, sendline, strlen(sendline), 0);
        }
        else if (message.compare(0, 6, "GetAll") == 0)
        {
            message = to_string(message_index - last_read[fd]);

            memset(&sendline, 0, sizeof(sendline));
            strncpy(sendline, message.c_str(), sizeof(sendline));
            sendline[message.size()] = '\n';
            sendline[message.size()+1] = '\0';
            send(fd, sendline, strlen(sendline), 0);
            if (LOGGING) printf("send line %s\n", sendline);

            for (i = last_read[fd]; i < message_index; i++)
            {
                if (LOGGING) printf("message i is %s\n", messages[i].c_str());
                message = to_string(messages[i].size()) + " " + messages[i];

                memset(&sendline, 0, sizeof(sendline));
                strncpy(sendline, message.c_str(), sizeof(sendline));
                sendline[message.size()] = '\n';
                sendline[message.size()+1] = '\0';
                send(fd, sendline, strlen(sendline), 0);
                if (LOGGING) printf("send line %s\n", sendline);
            }
            last_read[fd] = i;

            if (LOGGING) printf("Finished GetAll\n");
        }
        else if (message.compare(0, 5, "Leave") == 0)
        {
            if (LOGGING) printf("closing connection with socket %d\n", fd);
            return 0;
        }
    }
    return 1;
}
