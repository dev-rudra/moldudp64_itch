#include "decoder.h"

#include <cstdint>
#include <string>

// Read 2 bytes as big-endian (network order)
// unsigned 16-bit value.
static uint16_t read_u16_big_endian(const uint8_t* bytes) {
    uint16_t high = (uint16_t)bytes[0];
    uint16_t low = (uint16_t)bytes[1];
    return (uint16_t)((high <<8) | low);
}

// Read 8 bytes as big-endian (network order)
// unsigned 64-bit value.
static uint64_t read_u64_big_endian(const uint8_t* bytes) {
    uint64_t b0 = (uint64_t)bytes[0];
    uint64_t b1 = (uint64_t)bytes[1];
    uint64_t b2 = (uint64_t)bytes[2];
    uint64_t b3 = (uint64_t)bytes[3];
    uint64_t b4 = (uint64_t)bytes[4];
    uint64_t b5 = (uint64_t)bytes[5];
    uint64_t b6 = (uint64_t)bytes[6];
    uint64_t b7 = (uint64_t)bytes[7];

    return (b0 << 56) |
           (b1 << 48) |
           (b2 << 40) |
           (b3 << 32) |
           (b4 << 24) |
           (b5 << 16) |
           (b6 << 8) |
           (b7 << 0);
}

bool parse_mold_header(const uint8_t* packet, int packet_len, MoldHeader* out) {
    if (!packet) return false;
    if (!out) return false;

    const int header_len = 10 + 8 + 2;
    if (packet_len < header_len) {
        return false;
    }

    out->session.assign((const char*)packet, 10);
    out->sequence_number = read_u64_big_endian(packet + 10);
    out->message_count = read_u16_big_endian(packet + 10 + 8);

    return true;
}
