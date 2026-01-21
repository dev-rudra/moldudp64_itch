#include "application.h"
#include "config.h"
#include "socket.h"
#include "decoder.h"
#include "recovery.h"

#include <cstdio>
#include <cstdint>
#include <string>
#include <cstring>
#include <cerrno>

Application::Application()
: max_messages(0),
  verbose(false),
  has_type_filter(false),
  has_start_seq(false),
  start_seq(0),
  enable_recovery(false) {
    std::memset(type_allowed, 0, sizeof(type_allowed));
}

void Application::set_max_messages(uint64_t value) {
    max_messages = value;
}

void Application::set_verbose(bool value) {
    verbose = value;
}

void Application::set_type_filter(char type) {
    has_type_filter = true;
    type_allowed[(unsigned char)type] = true;
}

void Application::set_start_seq(uint64_t value) {
    has_start_seq = true;
    start_seq = value;
}

void Application::set_enable_recovery(bool value) {
    enable_recovery = value;
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


// Purpose:
// Wrapper to decode a full MoldUDP packet (header + payload (all messages))
// Used mode:
// - Live, replay(download) / live + recovery (-g)
//
// Funtions:
// Calls parse_mold_header() to parse Mold header(session, startseq, msgcount)
// Iterates each MoldMessage in the packet
// Computes per-message seq = header.seqnum + msgcount
// Calls decode_itch_message() for each message
static uint16_t decode_packet_messages(const uint8_t* buffer, int bytes,
                                      const AppConfig& cfg, bool has_type_filter,
                                      const bool type_allowed[256],
                                      uint64_t& decoded_count,
                                      uint64_t max_messages_limit, bool& stop_now, bool verbose) {

    stop_now = false;

    MoldHeader header;
    if (!parse_mold_header(buffer, bytes, &header)) {
        return 0;
    }

    int offset = 10 + 8 + 2;
    uint16_t remaining = (uint16_t)header.message_count;
    uint16_t index = 0;
    const uint8_t* msg = 0;
    uint16_t msg_len = 0;
    uint16_t processed = 0;

    while (next_mold_message(buffer, bytes, &offset, &remaining, &msg, &msg_len)) {
        uint64_t seq = header.sequence_number + (uint64_t)index;
        index++;

        // Print filter by message type.
        bool allow_print = true;
        if (has_type_filter) {
            allow_print = type_allowed[(unsigned char)msg[0]];
        }

        if (allow_print) {
            decode_itch_message(msg, msg_len, cfg, header.session, seq, (uint16_t)header.message_count, verbose);
        }

        decoded_count++;
        processed++;

        // Stop after N total messages
        // on -n
        if (max_messages_limit != 0 && decoded_count >= max_messages_limit) {
            stop_now = true;
            return processed;
        }
    }
    return processed;
}

int Application::run() {
    const char* config_path = "config/config.ini";
    if (!load_config(config_path)) {
        std::printf("Failed to load config: %s\n", config_path);
        return 1;
    }

    const AppConfig& cfg = config();
    if (verbose) {
        std::printf("verbose on\n");
    }

    // Download mode -s <startseq>
    if (has_start_seq) {
        if (cfg.mcast_rerequester_ip.empty() || cfg.mcast_rerequester_port == 0) {
            std::printf("Error: rerequester IP/Port not set in the config\n");
            return 1;
        }

        // Get valid SessionId from first valid live Mold header
        // To send the rerequest packet to.
        char session[10];
        std::memset(session, ' ', sizeof(session));

        Socket live_sock;
        if (!live_sock.connect_socket(cfg.mcast_ip, cfg.mcast_port, cfg.interface_ip, cfg.mcast_source_ip)) {
            std::printf("Failed to connect multicast socket\n");
            return 1;
        }

        live_sock.set_receive_buffer(4 * 1024 * 1024);
        const int udp_packet_capacity = 64 * 1024;
        uint8_t live_buf[udp_packet_capacity];

        std::printf("Getting SessionID...\n");

        while (1) {
            int bytes = live_sock.receive_bytes(live_buf, udp_packet_capacity);
            if (bytes <= 0) {
                continue;
            }

            MoldHeader header;
            if (!parse_mold_header(live_buf, bytes, &header)) {
                continue;
            }

            if (header.session.size() < 10) {
                continue;
            }

            std::memcpy(session, header.session.data(), 10);
            std::printf("Session found: %.*s\n", 10, session);
            break;
        }

        live_sock.close();

        // Open rerequester Socket
        // Send request + receive reply
        // packets on same socket.
        Rerequester rr;
        int receive_buffer_bytes = 4 * 1024 * 1024;
        int timeout_ms = 1000;

        if (!rr.open(cfg.mcast_rerequester_ip, cfg.mcast_rerequester_port,
                     receive_buffer_bytes, timeout_ms)) {
            std::printf("Error: failed to open rerequester socket\n");
            return 1;
        }

        // Request reply
        // in chunks
        // configured in config with 
        // max_recovery_message_count
        uint16_t max_per_request = cfg.max_recovery_message_count;
        if (max_per_request == 0) {
            max_per_request = 5000;
        }

        uint64_t decoded_count = 0;
        uint64_t current_seq = start_seq;

        // If -n is provided => bounded download
        // If -n is not provided => download all (until stalled / Ctrl+C)
        bool bounded_download = (max_messages != 0);
        uint64_t remaining = max_messages;

        uint8_t rxbuf[udp_packet_capacity];

        std::printf("Recovery download start_seq=%llu max_per_request=%u\n",
                    (unsigned long long)start_seq,
                    (unsigned)max_per_request);

        while (!bounded_download || remaining > 0) {
            uint16_t req_count = max_per_request;
            if (bounded_download) {
                req_count = (remaining > (uint64_t)max_per_request)
                            ? max_per_request
                            : (uint16_t)remaining;
            }

            // Send rerequest for 
            // [current_seq ... current_seq + req_count -1]
            if (!rr.send_request(session, current_seq, req_count)) {
                std::printf("Recovery send_request failed seq=%llu count=%u\n",
                            (unsigned long long)current_seq, (unsigned)req_count);
                rr.close();
                return 1;
            }

            std::printf("Recovery request sent=%llu count=%u\n",
                        (unsigned long long)current_seq, (unsigned)req_count);

            // Receive reply packets
            // until enough message decode
            // for this chunk
            uint64_t got_in_chunk = 0;
            int timeouts = 0;

            while (got_in_chunk < (uint64_t)req_count) {
                int n = rr.receive_packet(rxbuf, udp_packet_capacity);
                if (n <= 0) {
                    // Timeout is expected sometimes
                    // allow a few then stop this chunk
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        timeouts++;
                        if (timeouts >= 3) {
                            std::printf("Recovery timeout seq=%llu got=%llu req=%u\n",
                                        (unsigned long long)current_seq,
                                        (unsigned long long)got_in_chunk,
                                        (unsigned)req_count);
                            break;
                        }
                        continue;
                    }
                    std::printf("Recovery recv error errno=%d\n", errno);
                    break;
                }

                bool stop_now = false;
                uint16_t processed = decode_packet_messages(rxbuf, n, cfg, has_type_filter, type_allowed,
                                                           decoded_count, max_messages, stop_now, verbose);

                got_in_chunk += (uint64_t)processed;

                if (stop_now) {
                    std::printf("Stop: decoded_count=%llu\n", (unsigned long long)decoded_count);
                    rr.close();
                    return 0;
                }
            }

            // If nothing arrived
            if (got_in_chunk == 0) {
                std::printf("Recovery stalled seq=%llu req=%u\n",
                            (unsigned long long)current_seq,
                            (unsigned)req_count);

                rr.close();

                // Unbounded (-s without -n): treat stall as end of download (exit OK)
                // Bounded (-s with -n): treat stall as failure (didn't get requested amount)
                return bounded_download ? 1 : 0;
            }

            // Skip broken data 
            // and move forward with 
            // all valid messages (best-effort)
            current_seq += got_in_chunk;

            if (bounded_download) {
                if (got_in_chunk >= remaining) {
                    remaining = 0;
                } else {
                    remaining -= got_in_chunk;
                }
            }
        }

        std::printf("Recovery done decoded_count=%llu\n", (unsigned long long)decoded_count);
        rr.close();
        return 0;
    }

    // Live Mode

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
    uint64_t decoded_count = 0;

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

        // Gap/Duplicate/SessionChange
        check_sequence_gap(header, current_session, joined, expected_seq);

        // Decode the packet's message
        bool stop_now = false;
        decode_packet_messages(buffer, bytes, cfg, has_type_filter, type_allowed,
                               decoded_count, max_messages, stop_now, verbose);

        // Expected next packet startseq
        expected_seq = header.sequence_number + (uint64_t)header.message_count;

        if (stop_now) {
            std::printf("Stop: decoded_count=%llu\n", (unsigned long long)decoded_count);
            sock.close();
            return 0;
        }
    }

    sock.close();
    return 0;
}
