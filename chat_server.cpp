#include "chatutilfunctions.h"

#define QLEN 32 // maximum connection queue length

int handle_message(int fd, std::map<int, int> &last_read, std::map<int, std::string> &messages, int &message_index, Timer timer);

int main(int argc, char**argv)
{
    struct sockaddr_in fsin;    // the from address of a client
    fd_set rfds;                // read file descriptor set
    fd_set afds;                // active file descriptor set
    unsigned int alen;          // from-address length
    int fd, nfds;
    int message_index = 0;

    int msock = atoi(argv[1]);

    if (listen(msock, QLEN) < 0)
        errexit("can't listen: %s\n", strerror(errno));

    printf("Starting session with socket %d\n", msock);

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
    timer.set();

    while (1)
    {
        memcpy(&rfds, &afds, sizeof(rfds));

        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)
            errexit("select: %s\n", strerror(errno));
        if (FD_ISSET(msock, &rfds))
        {
            int ssock;

            alen = sizeof(fsin);
            ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
            if (ssock < 0)
                errexit("accept: %s\n", strerror(errno));
            last_read[ssock] = 0;
            printf("Accepted socket %d\n", ssock);
            FD_SET(ssock, &afds);
        }

        for (fd = 0; fd < nfds; ++fd)
        {
            if (fd != msock && FD_ISSET(fd, &rfds))
            {
                if (handle_message(fd, last_read, messages, message_index, timer) == 0) {
                    (void) close(fd);
                    FD_CLR(fd, &afds);
                }
            }
        }
        if (timer.check_seconds_passed(60))
        {
            printf("TERMINATE\n");
        }
    }
}

int handle_message(int fd, std::map<int, int> &last_read, std::map<int, std::string> &messages, int &message_index, Timer timer)
{
    char sendline[MESSAGE_LENGTH] = {0};
    char recvline[MESSAGE_LENGTH] = {0};

    std::string message;
    int index, message_size, i, n, mesg_len;

    n = recv(fd, recvline, sizeof(recvline), 0);
    // printf("recvline: %s\nsize: %d\n", recvline, n);

    if (n < 0)
    {
        printf("Error receiving message %s\n", strerror(errno));
        return -1;
    }

    Reader reader(recvline, n);

    while (reader.get_index() < n)
    {

        message = reader.next_word();
        printf("word: %s\n", message.c_str());
        printf("\n");

        if (message.compare(0, 6, "Submit") == 0)
        {
            mesg_len = reader.next_int();
            message = reader.next_line();
            messages[message_index++] = message;
            printf("Got message: %s with size: %d\n", message.c_str(), mesg_len);
            timer.set();
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
            timer.set();
        }
        else if (message.compare(0, 6, "GetAll") == 0)
        {
            message = to_string(message_index - last_read[fd]);

            clear_array(sendline);
            strncpy(sendline, message.c_str(), sizeof(sendline));
            sendline[message.size()] = '\n';
            sendline[message.size()+1] = '\0';
            send(fd, sendline, strlen(sendline), 0);
            printf("send line %s\n", sendline);

            for (i = last_read[fd]; i < message_index; i++)
            {
                printf("message i is %s\n", messages[i].c_str());
                message = to_string(messages[i].size()) + " " + messages[i];

                clear_array(sendline);
                strncpy(sendline, message.c_str(), sizeof(sendline));
                sendline[message.size()] = '\n';
                sendline[message.size()+1] = '\0';
                send(fd, sendline, strlen(sendline), 0);
                printf("send line %s\n", sendline);
            }
            last_read[fd] = i;

            printf("Finished GetAll\n");
            timer.set();
        }
        else if (message.compare(0, 5, "Leave") == 0)
        {
            printf("closing connection with socket %d\n", fd);
            timer.set();
            return 0;
        }

        printf("reader index: %d\n", reader.get_index());
    }
    return 1;
}
