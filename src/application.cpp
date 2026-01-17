#include "application.h"
#include "config.h"
#include <iostream>
#include <unordered_map>
#include <cstdio>

int Application::run() {
    const char* config_path = "config/config.ini";
    if (!load_config(config_path)) {
        std::printf("Failed to load config: %s\n", config_path);
        return 1;
    }

    const AppConfig& cfg = config();

    std::printf("mcast_ip: %s\n", cfg.mcast_ip.c_str());

    if (!cfg.msg_specs.empty()) {
        std::unordered_map<char, MsgSpec>::const_iterator it = cfg.msg_specs.begin();
        const MsgSpec& msg = it->second;

        std::printf("sample type: %c name: %s fields: %u total_length: %u\n",
                    msg.msg_type,
                    msg.name.c_str(),
                    (unsigned)msg.fields.size(),
                    (unsigned)msg.total_length);
    }

    return 0;
}
