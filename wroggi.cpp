#include <stdlib.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>

#if defined(__linux__) || defined(__FreeBSD__)
#include <arpa/nameser.h>
#else
#include <nameser.h>
#endif

#include <resolv.h>

#include "name_pkt.hpp"

#define SA struct sockaddr

void usage(char* filename);
char* get_resolver(int i);
int get_resolver_count(void);

int main(int argc, char** argv)
{
    if (!(argc == 2 || argc == 3 || argc == 4)) usage(argv[0]);

    srand((unsigned)time(NULL));
    uint16_t ns_id = (rand()&0x0000FFFF);

    std::string name(argv[1]);

    name_pkt npkt;
    npkt.n_set_id(ns_id);
    npkt.n_set_flags(0x0100);
    npkt.n_create_rr_questionA(name);
    npkt.n_build_payload();

    ssize_t len;
    socklen_t sin_size = sizeof(struct sockaddr_in);

    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sin1;
    struct sockaddr_in sin2;
    unsigned char buf[BUFSIZ];

    memset(&sin1, 0, sizeof(sin1));
    memset(&sin2, 0, sizeof(sin2));
    memset(buf, 0, BUFSIZ);

    res_init();

    sin1.sin_family = AF_INET;
    sin1.sin_port = htons(53);
    if (argc == 2) {
        inet_pton(AF_INET, get_resolver(0), &sin1.sin_addr);
        //printf("%s\n", get_resolver(0));
    } else if (argc == 3) {
        inet_pton(AF_INET, argv[2], &sin1.sin_addr);
    } else if (argc == 4) {
        inet_pton(AF_INET, argv[2], &sin1.sin_addr);
#ifdef __linux__
        sin2.sin_family = AF_INET;
        sin2.sin_port = htons(atoi(argv[3]));
#else
        sin2.sin_len = sizeof(struct sockaddr_in);
        sin2.sin_family = AF_INET;
        sin2.sin_port = htons(atoi(argv[3]));
#endif
        //inet_pton(AF_INET, argv[2], &sin2.sin_addr);
        if (bind(sockfd, (struct sockaddr*)&sin2, sizeof(sin2)) < 0) {
            perror("bind");
            exit(EXIT_FAILURE);
        }
        //printf("%d\n", err);
    } else {
        ;
    }

    struct timeval prev;
    struct timeval current;

    ssize_t retval = sendto(sockfd, npkt.n_payload(), npkt.n_payload_size(), 0, (SA*)&sin1, sizeof(sin1));
    if (retval < 0) {
        perror("sendto");
    } else {
        ;
    }
    close(sockfd);

    exit(EXIT_SUCCESS);
}

void usage(char* filename){
    printf("%s [hostname]\n", filename);
    printf("%s [hostname] [dns server]\n", filename);
    printf("%s [hostname] [dns server] [source port]\n", filename);
    exit(EXIT_FAILURE);
}

char* get_resolver(int i)
{
    struct sockaddr_in sin;
    memcpy(&sin, &(_res.nsaddr_list[i]), sizeof(struct sockaddr_in));
    return inet_ntoa(sin.sin_addr);
}

int get_resolver_count(void)
{
    return _res.nscount;
}
