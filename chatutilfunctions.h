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

#include <map>
#include <sys/select.h>
#include <sys/time.h>
#include <sstream>
#include <time.h>

#include "Reader.h"

#define MESSAGE_LENGTH 128

#ifndef chatutilfunctions
#define chatutilfunctions

// print an error message and exit
int errexit(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

void clear_array(char *array)
{
    memset(&array, 0, sizeof(array));
}

// convert an int into a std::string
std::string to_string(int i)
{
    std::stringstream ss;
    ss << i;
    return ss.str();
}

class Timer
{
    clock_t init;
public:
    Timer() {set();};

    void set() {init = clock();};

    // return true if more the 'seconds' seconds has passed since calling set
    bool check_seconds_passed(int seconds) {
        return ((float)(clock() - init)) / CLOCKS_PER_SEC > seconds;
    };
};

#endif
