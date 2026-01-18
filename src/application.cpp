#include "application.h"
#include "config.h"
#include "socket.h"
#include "decoder.h"

#include <cstdio>
#include <cstdint>

int Application::run() {
    const char* config_path = "config/config.ini";
    if (!load_config(config_path)) {
        std::printf("Failed to load config: %s\n", config_path);
        return 1;
    }

    const AppConfig& cfg = config();

    std::printf("mcast_ip: %s\n", cfg.mcast_ip.c_str());

    Socket sock;

    if (!sock.connect_socket(cfg.mcast_ip, cfg.mcast_port, cfg.interface_ip, cfg.mcast_source_ip)) {
        std::printf("Failed to connect socket\n");
        return 1;
    }

    sock.set_receive_buffer(4 * 1024 * 1024);
    const int buffer_capacity = 64 * 1024;
    uint8_t buffer[buffer_capacity];

    std::printf("Listening... (Ctrl+C to stop)\n");

    while (1) {
        int bytes = sock.receive_bytes(buffer, buffer_capacity);
        if (bytes <= 0) {
            continue;
        }

        MoldHeader header;
        if (!parse_mold_header(buffer, bytes, &header)) {
            continue;
        }

        int offset = 10 + 8 + 2;
        uint16_t remaining = header.message_count;
        uint16_t index = 0;
        const uint8_t* msg = 0;
        uint16_t msg_len = 0;

        while (next_mold_message(buffer, bytes, &offset, &remaining, &msg, &msg_len)) {
            uint64_t seq = header.sequence_number + (uint64_t)index;
            index++;

            decode_itch_message(msg, msg_len, cfg, header.session, seq, header.message_count);
        }
    }

    sock.close();
    return 0;
}
