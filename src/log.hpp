#pragma once

#include <cstdio>

#define LOG_DEBUG(fmt, ...)   fprintf(stdout, "[%s] [DEBUG] " fmt "\n", currentTime(), ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)    fprintf(stdout, "[%s] [INFO] "  fmt "\n", currentTime(), ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) fprintf(stdout, "[%s] [WARNING] " fmt "\n", currentTime(), ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)   fprintf(stderr, "[%s] [ERROR] " fmt "\n", currentTime(), ##__VA_ARGS__)

const char* currentTime();
void die(const char *fmt, ...);
