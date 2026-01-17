#ifndef DECODER_H
#define DECODER_H

#include <cstdint>
#include <string>

struct AppConfig;

struct MoldHeader {
    std::string session;
    uint64_t sequence_number;
    uint64_t message_count;

    MoldHeader() : sequence_number(0), message_count(0) {}
};

bool parse_moldudp64_header(const uint8_t* packet, int packet_len, MoldHeader* out);

#endif
