#ifndef SOCKET_H
#define SOCKET_H

#include <cstdint>
#include <string>
#include <sys/socket.h>

class Socket {
public:
    Socket();     // Constructor
    ~Socket();    // Destructor

    bool connect_socket(const std::string& mcast_ip, uint16_t mcast_port,
                        const std::string& interface_ip, const std::string& source_ip);

    // Receive one datagram into buffer.
    // Returns bytes received, or -1 on error
    int receive_bytes(uint8_t* buffer, int buffer_capacity);
    
    // Receive up to message_count datagrams using recvmmsg().
    // Returns packets received, or -1 on error.
    #ifdef __linux__
    int receive_batch(struct mmsghdr* message_vector, int message_count);
    #endif

    // Set SO_RCVBUF size.
    // Returns true on success.
    bool set_receive_buffer(int receive_buffer_bytes);

    void close();

private:
    int fd;
};

#endif
