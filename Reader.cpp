#include "Reader.h"

Reader::Reader(char *buf, int n)
{
    if (n > 127)
    {
        printf("ERROR: reader size can't be more than 127\n");
        exit(1);
    }
    index = 0;
    size = n;
    buffer = buf;
}

char Reader::next_char()
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
        c = next_char();
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

std::string Reader::next_n(int n)
{
    char line_buffer[128] = {0};
    int line_index = 0;
    char c;

    for (int i = 0; i < n; i++)
    {
        c = next_char();
        if (c == '\0')
        {
            line_buffer[line_index] = '\0';
            return std::string(line_buffer);
        }
        else
            line_buffer[line_index++] = c;
    }
    // discard trailing newline after n chars if it is there
    if (buffer[index] == '\n') index++;
    return std::string(line_buffer);
}

std::string Reader::next_line()
{
    char line_buffer[128] = {0};
    int line_index = 0;
    char c;

    while (1)
    {
        c = next_char();
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
    char word_buffer[128] = {0};
    int word_index = 0;
    char c;

    while (1)
    {
        c = next_char();
        if (c == ' ' || c == '\0')
        {
            word_buffer[word_index] = '\0';
            return std::string(word_buffer);
        }
        else
            word_buffer[word_index++] = c;
    }
}
