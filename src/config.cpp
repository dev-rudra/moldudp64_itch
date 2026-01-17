#include "config.h"

#include <fstream>
#include <string>
#include <cctype>
#include <cstdlib>

static AppConfig app_config;

static std::string trim(const std::string& s) {
    const char* whitespace = " \t\r\n";

    size_t first_non_whitespace = s.find_first_not_of(whitespace);
    if (first_non_whitespace == std::string::npos) {
        return "";
    }

    size_t last_non_whitespace = s.find_last_not_of(whitespace);
    return s.substr(first_non_whitespace, last_non_whitespace - first_non_whitespace +1);
}

static std::string to_upper_case(const std::string& s) {
    std::string out = s;
    for (size_t i = 0; i < out.size(); i++) {
        out[i] = (char)std::toupper((unsigned char)out[i]);
    }
    return out;
}

bool load_config(const char* config_path) {
    AppConfig cfg;

    std::ifstream file(config_path);
    if (!file) {
        return false;
    }

    std::string section;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line[line.size() -1] == '\r') {
            line.resize(line.size() -1);
        }

        line = trim(line);

        if (line.empty()) {
            continue;
        }

        if (line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = to_upper_case(trim(line.substr(1, line.size() -2)));
            continue;
        }

        size_t pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = trim(line.substr(0, pos));
        std::string val = trim(line.substr(pos + 1));

        if (section == "FEED_CHANNELS") {
            if      (key == "mcast_ip") cfg.mcast_ip = val;
            else if (key == "mcast_port") cfg.mcast_port = (uint16_t)std::atoi(val.c_str());
            else if (key == "mcast_source_ip") cfg.mcast_source_ip = val;
            else if (key == "interface_ip") cfg.interface_ip = val;
            else if (key == "mcast_rerequester_ip") cfg.mcast_rerequester_ip = val;
            else if (key == "mcast_rerequester_port") cfg.mcast_rerequester_port = (uint16_t)std::atoi(val.c_str());
        }
        else if (section == "RECOVERY_SETTINGS") {
            if (key == "max_recovery_message_count") {
                cfg.max_recovery_message_count = (uint16_t)std::atoi(val.c_str());
            }
        }
    }

    if (cfg.mcast_ip.empty()) return false;
    if (cfg.mcast_port == 0) return false;
    if (cfg.interface_ip.empty()) return false;

    app_config = cfg;
    return true;
}

const AppConfig& config() {
    return app_config;
}
