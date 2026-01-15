#include "socket.h"
#include <sys/socket.h>
#include <unistd.h>

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

    return true;
}

int Socket::receive_bytes(uint8_t* buffer, int buffer_capacity) {
    return -1;
}

int Socket::receive_batch(struct mmsghdr* message_vector, int message_count) {
    return -1;
}

bool Socket::set_receive_buffer(int receive_buffer_bytes) {
    return false;
}


