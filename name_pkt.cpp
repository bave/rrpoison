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
    //printf("%d\n", get_resolver_count());
    //printf("%s\n", get_resolver(0));

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
            exit(1);
        }
        //printf("%d\n", err);
    } else {
    }

    /*
    struct timeval {
        time_t       tv_sec;
        suseconds_t  tv_usec;
    };
    */

    struct timeval prev;
    struct timeval current;

    int s_retval;
    fd_set s_fd;

    /*
    struct timeval t_val;
    memset(&t_val, 0, sizeof(t_val));
    t_val.tv_sec = 2;
    */

    gettimeofday(&prev, NULL);
    sendto(sockfd, npkt.n_payload(), npkt.n_payload_size(), 0, (SA*)&sin1, sizeof(sin1));

    int hit = 0;
    re:

    FD_ZERO(&s_fd);
    FD_SET(sockfd, &s_fd);
    //s_retval = select((sockfd+1), &s_fd, NULL, NULL, &t_val);
    s_retval = select((sockfd+1), &s_fd, NULL, NULL, NULL);
    if (s_retval <= 0) {
        if (s_retval == -1) {
            perror("select");
            close(sockfd);
            exit(1);
        }
        if (s_retval == 0) {
            printf("2.000000 %s timeout\n", argv[1]);
            close(sockfd);
            return 0;
        }
    }

    len = recvfrom(sockfd, buf, BUFSIZ, 0, (SA*)&sin2, &sin_size); 
    gettimeofday(&current, NULL);


    time_t sec;
    suseconds_t usec;
    if (current.tv_sec == prev.tv_sec) {
        sec = current.tv_sec - prev.tv_sec;
        usec = current.tv_usec - prev.tv_usec;
    }

    else if (current.tv_sec != prev.tv_sec) {
        int carry = 1000000;
        sec = current.tv_sec - prev.tv_sec;
        if (prev.tv_usec > current.tv_usec) {
            usec = carry - prev.tv_usec + current.tv_usec;
            sec--;
            if (usec > carry) {
                usec = usec - carry;
                sec++;
            }
        } else {
            usec = current.tv_usec - prev.tv_usec;
        }
    }



    if (len == -1) { exit(1); }

    unsigned int name_id;
    ns_msg ns_handle;
    memset(&ns_handle, 0, sizeof(ns_handle));
    ns_initparse((const unsigned char*)buf,  len, &ns_handle);
    name_id = ns_msg_id(ns_handle);
    printf("port hit!! %d:%d\n", hit, name_id);
    if (name_id != ns_id) {
        hit++;
        goto re;
    }


    int count;
    ns_rr rr;
    memset(&rr, 0, sizeof(rr));
    count = ns_msg_count(ns_handle, ns_s_an);

    if (count == 0) {
#ifdef __linux__
        printf("%lu.%06li %s nxdomain\n", sec, usec, argv[1]);
#elif __FreeBSD__
        printf("%lu.%06ld %s nxdomain\n", sec, usec, argv[1]);
#else
        printf("%lu.%06d %s nxdomain\n", sec, usec, argv[1]);
#endif
        close(sockfd);
        return 0;
    } else {
#ifdef __linux__
        printf("%lu.%06li ", sec, usec);
#elif __FreeBSD__
        printf("%lu.%06ld ", sec, usec);
#else
        printf("%lu.%06d ", sec, usec);
#endif
    }
    
    int i;
    int type;
    //char rdata_buffer[BUFSIZ];
    for (i=0; i<count; i++) {
        ns_parserr(&ns_handle, ns_s_an, i, &rr);
        printf("%s", argv[1]);
        printf(" ");

        /*
        memset(rdata_buffer, '\0', BUFSIZ);
        ret = ns_name_uncompress(
            ns_msg_base(ns_handle),
            ns_msg_end(ns_handle),
            ns_rr_rdata(rr),
            rdata_buffer,
            BUFSIZ);
        printf("%s", rdata_buffer);
        */

        uint32_t rbuf;
        struct in_addr* addr;
        memcpy(&rbuf, ns_rr_rdata(rr), sizeof(rbuf));
        addr = (struct in_addr*)&rbuf;
        printf("%s", inet_ntoa(*addr));
        printf(" ");
        type = ns_rr_type(rr);
        // type 1 : a
        if(type == 1) {
            printf("A");
            printf(" ");
        }
        printf("\n");
        /*
        // type 28: aaaa
        if(type == 28) {
            printf(":AAAA\n");
        }
        */
    }
    close(sockfd);

    return 0;
}

void usage(char* filename){
    printf("%s [hostname]\n", filename);
    printf("%s [hostname] [dns server]\n", filename);
    printf("%s [hostname] [dns server] [source port]\n", filename);
    exit(-1);
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
