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

bool is_ipv4_address(const std::string &addr)
{
    struct sockaddr_in sin;
    if (inet_pton(AF_INET, addr.c_str(), &sin.sin_addr) > 0) {
        return true;
    } else {
        return false;
    }
}


/*
// XXX broken...
void memdump(void* mem, int i)
{
    if(i >= 2000)
    {
        printf("allocation memory size over\n");
        return;
    }

    int j;
    int max;
    int *memi;
    int *buf;
    buf = (int*)malloc(2000);
    memset(buf, 0, 2000);
    memcpy(buf, mem, i);
    memi = buf;

    printf("start memory dump %p ***** (16byte alignment)\n", mem);
    max = i / 16 + (i % 16 ? 1 : 0);

    for (j = 0; j < max; j++)
    {
        printf("%p : %08x %08x %08x %08x\n",
                memi,
                htonl(*(memi)),
                htonl(*(memi+1)),
                htonl(*(memi+2)),
                htonl(*(memi+3))
              );
        memi += 4;
    }

    printf("end memory dump ***** %d byte\n", i);
    free(buf);

    return;
}
*/

void memdump(void* buffer, int length)
{
    uint32_t* addr32 = (uint32_t*)buffer;
    int i;
    int j;
    int k;
    int lines = length/16 + (length%16?1:0);
    if (lines > 1) {
        for (i=0; i<lines; i++) {
            printf("%p : %08x %08x %08x %08x\n",
                    addr32,
                    htonl(*(addr32)),
                    htonl(*(addr32+1)),
                    htonl(*(addr32+2)),
                    htonl(*(addr32+3))
                  );
            addr32 += 4;
        }
    } else {
    }

    j = length%16;
    if (j == 0) return;
    k = 0;
    uint8_t*  addr8 = (uint8_t*)addr32;
    printf("%p : ", addr8);
    for (i=0; i<16; i++) {
        if (k%4 == 0 && i != 0) printf(" ");
        if (j > i) {
            printf("%02x", *addr8);
            addr8++;
        } else {
            printf("XX");
        }
        k++;
    }
    printf("\n");
    return;
}


#endif //__UTILS_HPP__

