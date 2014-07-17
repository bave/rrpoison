#include <iostream>
#include <string>

#include "pkt_buffer.hpp"
#include "utils.hpp"

int
main()
{
    // pkt_buffer test code
    std::string src("127.0.0.1");
    std::string dst("127.0.0.1");
    int sport = 53;
    int dport = 53;
    const char* payload = "hoge";
    char b[BUFSIZ];

    memset(b, 0, sizeof(b));
    pkt_buffer pb;
    pb.set_buffer(b, BUFSIZ);
    pb.set_iphdr(src, dst);
    pb.set_udphdr(sport, dport);
    pb.set_payload(payload, strlen(payload));
    pb.post_processing();
    memdump(pb.get_buffer(), pb.get_payload_size());
    return 0;
}
