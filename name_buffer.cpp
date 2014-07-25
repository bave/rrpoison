#include <iostream>
#include <string>
#include <unistd.h>

#include "name_buffer.hpp"
#include "utils.hpp"

int
main()
{
    // pkt_buffer test code
    std::string src("127.0.0.1");
    std::string dst("127.0.0.1");
    int sport = 53;
    int dport = 53;

    std::string opt_r("hoge.hoge");
    std::string opt_a("127.0.0.1");
    //const char* payload = "hoge";

    char b[BUFSIZ];
    memset(b, 0, sizeof(b));

    name_buffer nb;
    nb.set_buffer(b, sizeof(b));
    nb.set_iphdr(src, dst);
    nb.set_udphdr(sport, dport);
    nb.set_payload_dns(0x0000, 0xFFFF, opt_r, opt_a);
    nb.post_processing();

    memdump(nb.get_buffer(), nb.get_payload_size());

    nb.change_dport_dnsid(0x0000, 0xFFFF);
    nb.post_processing();

    memdump(nb.get_buffer(), nb.get_payload_size());

    return 0;
}
