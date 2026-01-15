#include "socket.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <mcast_ip> <port> <interface_ip> [source_ip]\n";
        return 1;
    }

    std::string mcast_ip = argv[1];
    uint16_t port = (uint16_t)std::atoi(argv[2]);
    std::string interface_ip = argv[3];
    std::string source_ip = (argc >= 5) ? argv[4] : "";

    Socket sock;
    if (!sock.connect_socket(mcast_ip, port, interface_ip, source_ip)) {
        std::cout << "connect_socket failed\n";
        return 2;
    }

    std::cout << "Listening...\n";

    uint8_t buf[65536];

    while (true) {
        int n = sock.receive_bytes(buf, (int)sizeof(buf));
        if (n > 0) {
            std::cout << "recv bytes=" << n << "\n";
        }
    }

    return 0;
}
