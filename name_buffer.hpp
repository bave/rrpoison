#ifndef __NAME_BUFFER_HPP__
#define __NAME_BUFFER_HPP__

#include "pkt_buffer.hpp"
#include "name_pkt.hpp"

#include "utils.hpp"

class name_buffer : public pkt_buffer {
public:
    name_buffer();
    virtual ~name_buffer();

    bool set_payload_dns_ans(uint16_t id, uint16_t flags, std::string& req, std::string& ans);
    bool change_dport_dnsid(uint16_t port, uint16_t id);
    bool set_payload_ioprey(uint16_t id,
                            uint16_t flags, 
                            std::string& request_name,
                            //std::string& answer_name,
                            std::string& authority_name,
                            std::string& glue_name,
                            std::string& glue_addr);

protected:
    name_pkt np;
};

name_buffer::name_buffer() {
}

name_buffer::~name_buffer() {
}


bool
name_buffer::set_payload_dns_ans(uint16_t id, uint16_t flags, std::string& req, std::string& ans)
{
    np.n_init();
    np.n_set_id(id);
    np.n_set_flags(flags);
    np.n_create_rr_questionA(req);
    np.n_create_rr_answer(ans);
    np.n_build_payload();
    return set_payload(np.n_payload(), np.n_payload_size());
}

bool
name_buffer::set_payload_ioprey(uint16_t id,
                                uint16_t flags, 
                                std::string& request_name,
                                //std::string& answer_name,
                                std::string& authority_name,
                                std::string& glue_name,
                                std::string& glue_addr)
{
    np.n_init();
    np.n_set_id(id);
    np.n_set_flags(flags);
    np.n_create_rr_questionA(request_name);
    //np.n_create_rr_answer(answer_name);
    np.n_create_rr_nameserverNS(authority_name, glue_name);
    np.n_create_rr_additionalA(glue_name, glue_addr);
    np.n_build_payload();
    return set_payload(np.n_payload(), np.n_payload_size());
}

bool
name_buffer::change_dport_dnsid(uint16_t port, uint16_t id)
{
    if (iphdr_size == 0 || udphdr_size ==0 || payload_size == 0) {
        return false;
    }

#ifdef __linux__
    udphdr->dest = htons(port);
#else
    udphdr->uh_dport = htons(port);
#endif

    uint16_t* dns_id = (uint16_t*)(buffer + iphdr_size + udphdr_size);
    *dns_id = htons(id);

    return true;
}

#endif //__NAME_BUFFER_HPP__
