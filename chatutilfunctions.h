// #include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string>
#include <string.h>
#include <cstdlib>

#include <sys/errno.h>
#include <stdarg.h>

#define MESSAGE_LENGTH 256
#define START_LEN      6
#define FIND_LEN       5
#define TERMINATE_LEN  10

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

#endif
