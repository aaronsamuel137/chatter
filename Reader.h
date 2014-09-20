#ifndef READER_H
#define READER_H

#include <cstdlib>
#include <string>

// Scans through a char array and keeps track of how much it has read and the size of the array
class Reader
{
    int index, size;
    char *buffer;

private:
    char get_char();

public:
    // Initialize a Reader with a pointer to the char array and an int that indicates the size
    Reader(char*, int);

    // Returns the next integer from the char array and converts it to int
    // Handles ints of 4 digits maximum
    int next_int();

    // Returns a std::string comprised of the chars from the current index of the Reader until a \n or \0 char
    std::string next_line();

    // Returns a std::string comprised of the chars from the current index of the Reader until a space char or \0 char
    std::string next_word();

    // return the current index of the Reader
    int get_index() {return index;};
};

#endif
