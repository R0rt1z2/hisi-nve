#pragma once

#include <unistd.h>

#define NVE_LOG_COLOR_RED     "\x1b[31m"
#define NVE_LOG_COLOR_GREEN   "\x1b[32m"
#define NVE_LOG_COLOR_YELLOW  "\x1b[33m"
#define NVE_LOG_COLOR_BLUE    "\x1b[34m"
#define NVE_LOG_COLOR_MAGENTA "\x1b[35m"
#define NVE_LOG_COLOR_CYAN    "\x1b[36m"
#define NVE_LOG_COLOR_RESET   "\x1b[0m"

#define NVE_LOG(verbosity, ...)                                            \
    do {                                                                   \
        if (verbosity == '-') {                                            \
            if (access("/sbin/recovery", F_OK) == 0) {                     \
                fprintf(stderr, "[-] " __VA_ARGS__);                       \
            } else {                                                       \
                fprintf(stderr, NVE_LOG_COLOR_RED "[-] " __VA_ARGS__);     \
                fprintf(stderr, NVE_LOG_COLOR_RESET);                      \
            }                                                              \
            exit(EXIT_FAILURE);                                            \
        } else if (verbosity == '!') {                                     \
            if (access("/sbin/recovery", F_OK) == 0) {                     \
                fprintf(stderr, "[!] " __VA_ARGS__);                       \
            } else {                                                       \
                fprintf(stderr, NVE_LOG_COLOR_YELLOW "[!] " __VA_ARGS__);  \
                fprintf(stderr, NVE_LOG_COLOR_RESET);                      \
            }                                                              \
        } else if (verbosity == '?') {                                     \
            if (access("/sbin/recovery", F_OK) == 0) {                     \
                fprintf(stderr, "[?] " __VA_ARGS__);                       \
            } else {                                                       \
                fprintf(stderr, NVE_LOG_COLOR_BLUE "[?] " __VA_ARGS__);    \
                fprintf(stderr, NVE_LOG_COLOR_RESET);                      \
            }                                                              \
        } else if (verbosity == '*') {                                     \
            if (access("/sbin/recovery", F_OK) == 0) {                     \
                fprintf(stderr, "[*] " __VA_ARGS__);                       \
            } else {                                                       \
                fprintf(stderr, NVE_LOG_COLOR_MAGENTA "[*] " __VA_ARGS__); \
                fprintf(stderr, NVE_LOG_COLOR_RESET);                      \
            }                                                              \
        } else if (verbosity == '+') {                                     \
            if (access("/sbin/recovery", F_OK) == 0) {                     \
                fprintf(stderr, "[+] " __VA_ARGS__);                       \
            } else {                                                       \
                fprintf(stderr, NVE_LOG_COLOR_GREEN "[+] " __VA_ARGS__);   \
                fprintf(stderr, NVE_LOG_COLOR_RESET);                      \
            }                                                              \
        }                                                                  \
    } while (0)