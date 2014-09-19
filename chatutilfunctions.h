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

void clear_array(char *array)
{
    memset(&array, 0, sizeof(array));
}

class Reader {
    int index, size;
    char *buffer;

public:
    Reader(char*, int);
    int next_int();
    void next_line(char*);
    std::string next_line();
    std::string next_word();
    int get_index() {return index;};

private:
    char get_char();
};

Reader::Reader(char *buf, int n)
{
    index = 0;
    size = n;
    buffer = buf;
}

char Reader::get_char()
{

    char c = buffer[index++];
    if (index > size)
        return '\0';
    return c;
}

int Reader::next_int()
{
    char digit_buffer[4] = {0};
    int digit_index = 0;
    bool int_found = false;
    char c;

    while (1)
    {
        c = get_char();
        if (c == '\0')
            return 0;
        if (isdigit(c))
        {
            digit_buffer[digit_index++] = c;
            int_found = true;
        }
        else if (int_found)
            break;
    }
    return atoi(digit_buffer);
}

std::string Reader::next_line()
{
    char line_buffer[80] = {0};
    int line_index = 0;
    char c;

    while (1)
    {
        c = get_char();
        if (c == '\n' || c == '\0')
        {
            line_buffer[line_index] = '\0';
            return std::string(line_buffer);
        }
        else
            line_buffer[line_index++] = c;
    }
}

std::string Reader::next_word()
{
    char word_buffer[80] = {0};
    int word_index = 0;
    char c;

    while (1)
    {
        c = get_char();
        if (c == ' ' || c == '\0')
        {
            word_buffer[word_index] = '\0';
            return std::string(word_buffer);
        }
        else
            word_buffer[word_index++] = c;
    }
}

void Reader::next_line(char *line_buffer)
{
    clear_array(line_buffer);
    int line_index = 0;
    char c;

    while (1)
    {
        c = get_char();
        if (c == '\n' || c == '\0')
            return;
        else
            line_buffer[line_index++] = c;
    }
}

#endif
