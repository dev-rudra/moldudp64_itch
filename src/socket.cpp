#include "socket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

Socket::Socket() : fd(-1) {}

Socket::~Socket() {
    close();
}

void Socket::close() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

bool Socket::connect_socket(const std::string& mcast_ip, uint16_t mcast_port,
                            const std::string& interface_ip, const std::string& source_ip) {

    if (fd >= 0) {
        close();
    }

    fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return false;
    }

    // Allow multi-listner
    int reuse = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in local_addr;
    std::memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(mcast_port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(fd, (sockaddr*)&local_addr, sizeof(local_addr)) <0) {
        close();
        return false;
    }

    // SSM (Source Specific Multicast)
    if (!source_ip.empty()) {
        ip_mreq_source multicast_request;
        std::memset(&multicast_request, 0, sizeof(multicast_request));
        multicast_request.imr_multiaddr.s_addr = ::inet_addr(mcast_ip.c_str());
        multicast_request.imr_interface.s_addr = ::inet_addr(interface_ip.c_str());
        multicast_request.imr_sourceaddr.s_addr = ::inet_addr(source_ip.c_str());

        if (::setsockopt(fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,
                         &multicast_request, sizeof(multicast_request)) < 0) {
            close();
            return false;
        }
        
        std::printf("INFO : Joined : %s Source: %s\n", mcast_ip.c_str(), source_ip.c_str());
        return true;
    }

    // ASM (Any Source Multicast)
    ip_mreq multicast_request;
    std::memset(&multicast_request, 0, sizeof(multicast_request));
    multicast_request.imr_multiaddr.s_addr = ::inet_addr(mcast_ip.c_str());
    multicast_request.imr_interface.s_addr = ::inet_addr(interface_ip.c_str());

    if (::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     &multicast_request, sizeof(multicast_request)) < 0) {
        close();
        return false;
    }

    std::printf("INFO : Joined : %s\n", mcast_ip.c_str());
    return true;
}

int Socket::receive_bytes(uint8_t* buffer, int buffer_capacity) {
    if (fd < 0) {
        return -1;
    }

    return (int)::recvfrom(fd, buffer, (size_t)buffer_capacity, 0, 0, 0);
}

#ifdef __linux__
int Socket::receive_batch(struct mmsghdr* message_vector, int message_count) {
    if (fd < 0) {
        return -1;
    }

    // MSG_WAITFORONE: block until >=1 packet arrives
    // then return quickly
    return ::recvmmsg(fd, message_vector, (unsigned int)message_count, MSG_WAITFORONE, 0);
}
#endif

bool Socket::set_receive_buffer(int receive_buffer_bytes) {
    if (fd < 0) {
        return false;
    }

    if (::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &receive_buffer_bytes, sizeof(receive_buffer_bytes)) < 0) {
        return false;
    }

    return true;
}


