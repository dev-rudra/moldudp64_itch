#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>

struct AppConfig {
    std::string mcast_ip;
    uint16_t mcast_port;

    std::string mcast_source_ip;
    std::string interface_ip;

    std::string mcast_rerequester_ip;
    uint16_t mcast_rerequester_port;

    uint16_t max_recovery_message_count;

    AppConfig()
        : mcast_port(0),
          mcast_rerequester_port(0),
          max_recovery_message_count(5000) {}
};

bool load_config(const char* config_path);
const AppConfig& config();

#endif
