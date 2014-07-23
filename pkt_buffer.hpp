#ifndef __PKT_BUFFER_HPP__
#define __PKT_BUFFER_HPP__


#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

//XXX
#include "utils.hpp"

#if defined(__MACH__)
struct icmphdr {
    uint8_t  icmp_type;
    uint8_t  icmp_code;
    uint16_t icmp_cksum;
};
#endif

class pkt_buffer {
public:
    pkt_buffer() { init(); };
    virtual ~pkt_buffer() { };

    bool set_buffer(char* buf, int buf_size);
    char* get_buffer();
    int get_buffer_size();
    int get_payload_size();
    bool set_iphdr(std::string& src, std::string& dst);
    bool set_udphdr(int sport, int dport);
    bool post_processing();
    bool set_payload(const char* data, int size);
    struct sockaddr_in get_src_sockaddr();
    struct sockaddr_in get_dst_sockaddr();


protected:
    char* buffer;
    struct ip* iphdr;
    struct udphdr* udphdr;
    int buffer_size;
    int iphdr_size;
    int udphdr_size;
    int payload_size;

private:
    void init();
    uint16_t checksum(const uint8_t* buf, size_t size, uint32_t adjust);
    void checksum_ip(struct ip* iphdr);
    void checksum_transport(struct ip* iphdr, size_t size);
};

void
pkt_buffer::init()
{
    buffer = NULL;
    iphdr  = NULL;
    udphdr = NULL;
    buffer_size = 0;
    iphdr_size  = 0;
    udphdr_size = 0;
    payload_size = 0;
    return;
}

bool
pkt_buffer::set_buffer(char* buf, int buf_size)
{
    if (buf_size >= 1500) {
        return false;
    } else {
        init();
        buffer = buf;
        buffer_size = buf_size;
        return true;
    }
}

char*
pkt_buffer::get_buffer()
{
    return buffer;
}

int
pkt_buffer::get_buffer_size()
{
    return buffer_size;
}

int
pkt_buffer::get_payload_size()
{
    return iphdr_size + udphdr_size + payload_size;
}


bool
pkt_buffer::set_iphdr(std::string& src, std::string& dst)
{
    if (buffer == NULL) {
        return false;
    }

    // set raw ip header and raw udp header
#ifdef __linux__

   //struct iphdr ip_header;
   //struct tcphdr tcp_header;
   return false;

#else //defined(__FreeBSD__) || defined(__MACH__)

    iphdr = (struct ip*)buffer;

    // ip header
    iphdr->ip_v   = 4;
    iphdr->ip_hl  = sizeof(struct ip) >> 2;
    iphdr->ip_tos = 0;
    iphdr->ip_len = 0;
    iphdr->ip_id  = 0;
    iphdr->ip_off = 0;
    iphdr->ip_ttl = 0xFF;
    iphdr->ip_p   = IPPROTO_UDP;
    iphdr->ip_sum = 0;
    inet_pton(AF_INET, src.c_str(), &iphdr->ip_src);
    inet_pton(AF_INET, dst.c_str(), &iphdr->ip_dst);

    iphdr_size = sizeof(struct ip);
    return true;

#endif

}

/*
 * 0                16               31
 * +--------+--------+--------+--------+
 * |     src port    |     dst port    |
 * +--------+--------+--------+--------+
 * |  udp length     |   check sum     |
 * +--------+--------+--------+--------+
 */
bool
pkt_buffer::set_udphdr(int sport, int dport)
{
    if (iphdr_size == 0) {
        return false;
    }

    udphdr = (struct udphdr*)(buffer + iphdr_size);

    // udp header
    udphdr->uh_sport = htons(sport);
    udphdr->uh_dport = htons(dport);
    udphdr->uh_ulen  = 0;
    udphdr->uh_sum   = 0;

    udphdr_size = sizeof(struct udphdr);
    return true;
}

bool
pkt_buffer::set_payload(const char* data, int size)
{
    if (iphdr_size == 0 || udphdr_size == 0) {
        return false;
    }
    char* payload = (char*)(buffer + iphdr_size + udphdr_size);
    memcpy(payload, data, size);
    payload_size = size;
    return true;
}

bool
pkt_buffer::post_processing()
{
    if (iphdr_size == 0 || udphdr_size ==0 || payload_size == 0) {
        return false;
    }

    struct ip* iphdr = (struct ip*)buffer;
    struct udphdr* udphdr = (struct udphdr*)(buffer + iphdr_size);

    iphdr->ip_len = iphdr_size + udphdr_size + payload_size;
    udphdr->uh_ulen = htons(udphdr_size + payload_size);

    checksum_transport(iphdr, iphdr->ip_len);

    return true;
}

uint16_t
pkt_buffer::checksum(const uint8_t* buf, size_t size, uint32_t adjust)
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

void
pkt_buffer::checksum_ip(struct ip* iphdr)
{
    int header_length;
    uint16_t cksum;

    header_length = iphdr->ip_hl<<2;

    iphdr->ip_sum = 0x0000;
    cksum = checksum((uint8_t*)iphdr, header_length, 0);
    iphdr->ip_sum = htons(cksum);

    return;
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
void
pkt_buffer::checksum_transport(struct ip* iphdr, size_t size)
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

struct sockaddr_in
pkt_buffer::get_src_sockaddr()
{

    /*
    __uint8_t   sin_len; 
    sa_family_t sin_family;
    in_port_t   sin_port;
    struct  in_addr sin_addr; 
    char        sin_zero[8];
    */

    struct sockaddr_in hoge;
    memset(&hoge, 0, sizeof(hoge));

    if (iphdr_size == 0 || udphdr_size == 0) {
        return hoge;
    }

#ifndef __linux__
    hoge.sin_len = sizeof(hoge);
#endif
    hoge.sin_family = AF_INET;
    hoge.sin_addr.s_addr = iphdr->ip_src.s_addr;
    hoge.sin_port = udphdr->uh_sport;
    return hoge;
}

struct sockaddr_in
pkt_buffer::get_dst_sockaddr()
{
    struct sockaddr_in hoge;
    memset(&hoge, 0, sizeof(hoge));

    if (iphdr_size == 0 || udphdr_size == 0) {
        return hoge;
    }

#ifndef __linux__
    hoge.sin_len = sizeof(hoge);
#endif
    hoge.sin_family = AF_INET;
    hoge.sin_addr.s_addr = iphdr->ip_dst.s_addr;
    hoge.sin_port = udphdr->uh_dport;
    return hoge;
}

#endif // __PKT_BUFFER_HPP__
