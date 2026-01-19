#ifndef RECOVERY_H
#define RECOVERY_H

#include <cstdint>
#include <string>
#include <netinet/in.h>

class Rerequester {
public:
    Rerequester();
    ~Rerequester();

    // Open UDP Socket for 
    // sending rerequest + receive unicast reply
    bool open(const std::string& ip, uint16_t port, int receive_buffer_bytesm, int timeout_ms);
    void close();
    bool send_request(const char session[10], uint64_t start_seq, uint16_t count);
    int receive_packet(uint8_t* buffer, int capacity);

private:
    int fd;
    sockaddr_in dst_addr;
};

#endif


