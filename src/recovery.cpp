#include "recovery.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <endian.h>

#pragma pack(push, 1)
struct RerequestPacket {
    char session[10];
    uint64_t sequence;
    uint16_t count;
};
#pragma pack(pop)

Rerequester::Rerequester()
: fd(-1) {
    std::memset(&dst_addr, 0, sizeof(dst_addr));
}

Rerequester::~Rerequester() {
    close();
}

bool Rerequester::open(const std::string& ip, uint16_t port, int receive_buffer_bytes, int timeout_ms) {
    close();

    if (ip.empty() || port == 0) {
        return false;
    }

    fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return false;
    }

    // Increase receive buffer
    ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &receive_buffer_bytes, sizeof(receive_buffer_bytes));

    // set recv timeout
    // so recvfrom doesn't
    // get stock forever
    timeval tv;
    std::memset(&tv, 0, sizeof(tv));
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Bind to any local port
    sockaddr_in local;
    std::memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(0);

    if (::bind(fd, (sockaddr*)&local, sizeof(local)) < 0) {
        close();
        return false;
    }

    // Prep
    // Dest addr for sendto
    std::memset(&dst_addr, 0, sizeof(dst_addr));
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(port);

    uint32_t addr = ::inet_addr(ip.c_str());
    if (addr == INADDR_NONE) {
        close();
        return false;
    }
    dst_addr.sin_addr.s_addr = addr;
    return true;
}

void Rerequester::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

bool Rerequester::send_request(const char session[10], uint64_t start_seq, uint16_t count) {
    if (fd < 0 || !session || count == 0) {
        return false;
    }

    RerequestPacket packet;
    std::memset(&packet, 0, sizeof(packet));

    // Copy 10 byetes session
    std::memcpy(packet.session, session, 10);

    // convert numeric fields to
    // big-endian
    packet.sequence = htobe64(start_seq);
    packet.count = htobe16(count);

    int sent = (int)::sendto(fd, &packet, (int)sizeof(packet), 0,
                             (sockaddr*)&dst_addr, sizeof(dst_addr));

    return (sent == (int)sizeof(packet));
}

int Rerequester::receive_packet(uint8_t* buffer, int capacity) {
    if (fd < 0 || !buffer || capacity <= 0) {
        return -1;
    }

    // Receive one reply MoldUDP64 packet
    // (unicast) into buffer
    int n = (int)::recvfrom(fd, buffer, (size_t)capacity, 0, 0, 0);
    return n;
}
