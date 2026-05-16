#include "log.hpp"

#include <ctime>
#include <cstdarg>
#include <cstdlib>

const char* currentTime()
{
    static char buf[20];
    std::time_t t = std::time(nullptr);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

void die(const char *fmt, ...)
{
    fprintf(stderr, "fatal: ");

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}
