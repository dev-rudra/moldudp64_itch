#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum FieldType {
    CHAR,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    INT16,
    INT32,
    INT64,
    STRING,
    BINARY
};

struct FieldSpec {
    std::string name;
    FieldType type;
    uint32_t size;
    uint32_t offset;
};

struct MsgSpec {
    char msg_type;
    std::string name;
    uint32_t total_length;
    std::vector<FieldSpec> fields;

    MsgSpec() : msg_type(0), total_length(0) {}
};

struct AppConfig {
    std::string mcast_ip;
    uint16_t mcast_port;

    std::string mcast_source_ip;
    std::string interface_ip;

    std::string mcast_rerequester_ip;
    uint16_t mcast_rerequester_port;

    uint16_t max_recovery_message_count;

    std::string protocol_spec;

    // Load spec
    std::unordered_map<char, MsgSpec> msg_specs;
    
    // Fast lookup by message type
    const MsgSpec* spec_by_type[256];

    AppConfig();
};

bool load_config(const char* config_path);
const AppConfig& config();

#endif
