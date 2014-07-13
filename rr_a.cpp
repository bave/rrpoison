#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
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

#ifdef __linux__
#include <arpa/nameser.h>
#elif __FreeBSD__
#else
#include <nameser.h>
#endif

#include <resolv.h>

#include "utils.hpp"
#include "name_pkt.hpp"

#define SA struct sockaddr
#define SAIN struct sockaddr_in

void usage(char* prog_name);
int   get_resolver_count(void);
char* get_resolver_name(int i);
uint16_t checksum(const uint8_t* buf, size_t size, uint32_t adjust);
void checksum_transport(struct ip* iphdr, size_t size);
void memdump(void* mem, int i);

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

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));

    fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (fd < 0) {
        PERROR("socket");
        exit(EXIT_FAILURE);
    }

    // set raw ip header and raw udp header
#ifdef __linux__
  //struct iphdr ip_header;
  //struct tcphdr tcp_header;

#else // __FreeBSD__ || __NetBSD__ || __APPLE__

    err = setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
    if (err < 0)
    {
        PERROR("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct ip* ip_hdr = (struct ip*)buffer;
    struct udphdr* udp_hdr = (struct udphdr*)(buffer + sizeof(struct ip));
    char* payload = (char*)(buffer + sizeof(struct ip)+sizeof(struct udphdr));

    // ip header
    ip_hdr->ip_v   = 4;
    ip_hdr->ip_hl  = sizeof(struct ip) >> 2;
    ip_hdr->ip_tos = 0;
    ip_hdr->ip_len = sizeof(struct ip) + sizeof(struct udphdr);
    ip_hdr->ip_id  = 0;
    ip_hdr->ip_off = 0;
    ip_hdr->ip_ttl = 0xFF;
    ip_hdr->ip_p   = IPPROTO_UDP;
    ip_hdr->ip_sum = 0;
    inet_pton(AF_INET, opt_s.c_str(), &ip_hdr->ip_src);
    inet_pton(AF_INET, opt_d.c_str(), &ip_hdr->ip_dst);


    // udp header
    udp_hdr->uh_sport = htons(0xAA);
    udp_hdr->uh_dport = htons(0xBB);
    udp_hdr->uh_ulen  = htons(sizeof(struct udphdr)+1);
    udp_hdr->uh_sum   = 0;

    checksum_transport(ip_hdr, ip_hdr->ip_len);


    // payload
    payload[0] = 0xFF;

    // preparing sendto information
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ip_hdr->ip_dst.s_addr;
    sin.sin_port = udp_hdr->uh_dport;


    size_t buffer_size = sizeof(struct ip) + sizeof(struct udphdr) +1;
    memdump(buffer, buffer_size);
    //ssize_t send_size = sendto(fd, buffer, buffer_size, MSG_DONTROUTE, (struct sockaddr*)&sin, sizeof(sin));
    ssize_t send_size = sendto(fd, buffer, buffer_size, 0, (struct sockaddr*)&sin, sizeof(sin));
    if (send_size < 0 ) {
        PERROR("snedto");
        exit(EXIT_FAILURE);
    }

#endif //__LINUX__

    return 0;
}

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


uint16_t checksum(const uint8_t* buf, size_t size, uint32_t adjust)
{
    uint32_t sum = 0;
    uint16_t element = 0;

    while (size>0) {
        element = (*buf)<<8;
        buf++;
        size--;
        if (size>0) {
            element |= *buf;
            buf++;
            size--;
        }
        sum += element;
    }
    sum += adjust;

    while (sum>0xFFFF) {
        sum = (sum>>16) + (sum&0xFFFF);
    }

    return (~sum) & 0xFFFF;
}

/*
 * 0                16               31
 * +----------------+----------------+       ∧
 * |       Source IPv4 Address       |       |
 * +----------------+----------------+       |
 * |     Destination IPv4 Address    | pseudo-header
 * +--------+-------+----------------+       |
 * | dummmy | Proto | TCP/UDP SegLen |       |
 * +--------+-------+----------------+       ∨
 * :                                 :
 */
void checksum_transport(struct ip* iphdr, size_t size)
{
    uint32_t pseudoSum = 0;
    uint8_t  protocol;
    uint8_t* l3_buf = (uint8_t*)iphdr;
    uint8_t* l4_buf = (uint8_t*)iphdr+(iphdr->ip_hl<<2);

    // Src Address ipv4
    pseudoSum += (l3_buf[12]<<8) | l3_buf[13];
    pseudoSum += (l3_buf[14]<<8) | l3_buf[15];

    // Dst Address ipv4
    pseudoSum += (l3_buf[16]<<8) | l3_buf[17];
    pseudoSum += (l3_buf[18]<<8) | l3_buf[19];

    // Protocol Number
    pseudoSum += protocol = iphdr->ip_p;

    size_t segment_size = size - (iphdr->ip_hl<<2);
    pseudoSum += segment_size;

    // protocol check !!
    if (protocol == IPPROTO_TCP) {
        //pseudoSum += (uint8_t)IPPROTO_TCP;
        struct tcphdr* tcphdr = (struct tcphdr*)l4_buf;
        tcphdr->th_sum = 0x0000;
        tcphdr->th_sum = htons(checksum(l4_buf, segment_size, pseudoSum));
    } else if (protocol == IPPROTO_UDP) {
        //pseudoSum += (uint8_t)IPPROTO_UDP;
        struct udphdr* udphdr = (struct udphdr*)l4_buf;
        udphdr->uh_sum = 0x0000;
        udphdr->uh_sum = htons(checksum(l4_buf, segment_size, pseudoSum));
    } else if (protocol == IPPROTO_ICMP) {
        struct icmphdr* icmphdr = (struct icmphdr*)l4_buf;
        icmphdr->icmp_cksum = 0x0000;
        icmphdr->icmp_cksum = htons(checksum((uint8_t*)icmphdr, segment_size, 0));
    } else {
        ;
    }

    return;
}


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

    printf("end memory dump *****\n");
    free(buf);

    return;
}
