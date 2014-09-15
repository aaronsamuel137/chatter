// #include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string>
#include <string.h>
#include <cstdlib>
#include <unistd.h>

#include <sys/errno.h>
#include <stdarg.h>



#define MESSAGE_LENGTH 128

#ifndef chatutilfunctions
#define chatutilfunctions

/*------------------------------------------------------------------------
 * errexit - print an error message and exit
 *------------------------------------------------------------------------
 */
int errexit(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

std::string get_message(std::string input, int start_index)
{
    std::string message = input.substr(start_index, input.size());
    return message.erase(message.find_last_not_of(" \n\r\t") + 1);
}

int TCPread(int socket)
{
    char buf[MESSAGE_LENGTH+1];  /* buffer for one line of text  */
    int n;                       /* socket descriptor, read count*/
    int outchars, inchars;       /* characters sent and received */

    while (fgets(buf, sizeof(buf), stdin)) {
        buf[MESSAGE_LENGTH] = '\0';    /* insure line null-terminated  */
        outchars = strlen(buf);
        (void) write(socket, buf, outchars);

        /* read it back */
        for (inchars = 0; inchars < outchars; inchars+=n ) {
            n = read(socket, &buf[inchars], outchars - inchars);
            if (n < 0)
                errexit("socket read failed: %s\n",
                    strerror(errno));
        }
        fputs(buf, stdout);
    }
}

#endif
