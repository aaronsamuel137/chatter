#ifndef READER_H
#define READER_H

#include <cstdlib>
#include <string>

class Reader
{
    int index, size;
    char *buffer;

private:
    char get_char();

public:
    Reader(char*, int);
    int next_int();
    std::string next_line();
    std::string next_word();
    int get_index() {return index;};
};

#endif
