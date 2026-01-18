#ifndef DECODER_H
#define DECODER_H

#include <cstdint>
#include <string>

struct AppConfig;

struct MoldHeader {
    std::string session;
    uint64_t sequence_number;
    uint64_t message_count;
};

bool parse_mold_header(const uint8_t* packet, int packet_len, MoldHeader* out);

bool next_mold_message(const uint8_t* packet,
                       int packet_len, int* offset,
                       uint16_t* remaining, const uint8_t** msg,
                       uint16_t* msg_len);

#endif
