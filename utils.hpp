#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#include <arpa/inet.h>

bool debug = true;

#ifdef DEBUG
#define MESG(format, ...) do {                             \
    if (debug) {                                           \
        fprintf(stderr, "%s:%s(%d): " format "\n",         \
        __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                      \
} while (false)
#else
#define MESG(format, ...) do {} while (false)
#endif //DEBUG

#ifdef DEBUG
#define PERROR(func) do {                    \
    if (debug) {                             \
    char s[BUFSIZ];                          \
    memset(s, 0, BUFSIZ);                    \
    snprintf(s, BUFSIZ, "%s:%s(%d): %s",     \
    __FILE__, __FUNCTION__, __LINE__, func); \
    perror(s);                               \
    }                                        \
} while (false)
#else
#define PERROR(func) do {} while (false)
#endif //DEBUG

#ifdef __MACH__
struct icmphdr {
    uint8_t  icmp_type;
    uint8_t  icmp_code;
    uint16_t icmp_cksum;
};
#endif

bool is_ipv4_address(const std::string &addr)
{
    struct sockaddr_in sin;
    if (inet_pton(AF_INET, addr.c_str(), &sin.sin_addr) > 0) {
        return true;
    } else {
        return false;
    }
}

#endif //__UTILS_HPP__

