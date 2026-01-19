#include "application.h"
#include "config.h"
#include "socket.h"
#include "decoder.h"

#include <cstdio>
#include <cstdint>
#include <string>
#include <cstring>

Application::Application()
: max_messages(0),
  verbose(false),
  has_type_filter(false) {
    std::memset(type_allowed, 0, sizeof(type_allowed));
}

void Application::set_max_messages(uint64_t value) {
    max_messages = value;
}

void Application::set_verbose(bool value) {
    verbose = value;
}

void Application::set_type_filter(char type) {
    if (!has_type_filter) {
        std::memset(type_allowed, 0, sizeof(type_allowed));
        has_type_filter = true;
    }
    type_allowed[(unsigned char)type] = true;
}

static void check_sequence_gap(const MoldHeader& header,
                               std::string& current_session,
                               bool& joined, uint64_t& expected_seq) {

    if (!joined) {
        current_session = header.session;
        expected_seq = header.sequence_number;
        joined = true;
        return;
    }

    if (header.session != current_session) {
        std::printf("Session Change from =%.*s to=%.*s seq=%llu\n",
                    (int)current_session.size(), current_session.c_str(),
                    (int)header.session.size(), header.session.c_str(),
                    (unsigned long long)header.sequence_number);

        current_session = header.session;
        expected_seq = header.sequence_number;
        return;
    }

    if (header.sequence_number > expected_seq) {
        uint64_t missing = header.sequence_number - expected_seq;
        std::printf("Gap session=%.*s expected=%llu got=%llu missing=%llu\n",
                    (int)header.session.size(), header.session.c_str(),
                    (unsigned long long)expected_seq,
                    (unsigned long long)header.sequence_number,
                    (unsigned long long)missing);
        return;
    }

    if (header.sequence_number < expected_seq) {
        std::printf("Duplicate session=%.*s expected=%llu got=%llu\n",
                    (int)header.session.size(), header.session.c_str(),
                    (unsigned long long)expected_seq,
                    (unsigned long long)header.sequence_number);
        return;
    }
}

int Application::run() {
    const char* config_path = "config/config.ini";
    if (!load_config(config_path)) {
        std::printf("Failed to load config: %s\n", config_path);
        return 1;
    }

    const AppConfig& cfg = config();

    std::printf("mcast_ip: %s\n", cfg.mcast_ip.c_str());

    if (verbose) {
        std::printf("verbose on\n");
    }

    Socket sock;

    if (!sock.connect_socket(cfg.mcast_ip, cfg.mcast_port, cfg.interface_ip, cfg.mcast_source_ip)) {
        std::printf("Failed to connect socket\n");
        return 1;
    }

    sock.set_receive_buffer(4 * 1024 * 1024);
    const int buffer_capacity = 64 * 1024;
    uint8_t buffer[buffer_capacity];

    std::string current_session;
    bool joined = false;
    uint64_t expected_seq = 0;

    uint64_t decoded_msg_count = 0;

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

        check_sequence_gap(header, current_session, joined, expected_seq);

        int offset = 10 + 8 + 2;
        uint16_t remaining = header.message_count;
        uint16_t index = 0;
        const uint8_t* msg = 0;
        uint16_t msg_len = 0;

        while (next_mold_message(buffer, bytes, &offset, &remaining, &msg, &msg_len)) {
            uint64_t seq = header.sequence_number + (uint64_t)index;
            index++;

            bool allow = true;
            if (has_type_filter) {
                allow = type_allowed[(unsigned char)msg[0]];
            }

            if (allow) {
                decode_itch_message(msg, msg_len, cfg, header.session, seq, header.message_count);
            }

            decoded_msg_count++;
            if (max_messages != 0 && decoded_msg_count >= max_messages) {
                std::printf("Stop: Message Decode Count=%llu\n", (unsigned long long)decoded_msg_count);
                sock.close();
                return 0;
            }
        }
        
        expected_seq = header.sequence_number + (uint64_t)header.message_count;
    }

    sock.close();
    return 0;
}
