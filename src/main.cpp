#include "application.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <unistd.h>

static void usage(const char* prog) {
    std::fprintf(stderr,
            "Usage: %s [-g] [-s <seq>] [-n <count>] [-v]\n\n"
            "Options:\n"
            "   -g              gap-fill mode\n"
            "   -s <seq>        get data starting at <seq>\n"
            "   -n <count>      stops after decoding <count> msg\n"
            "   -v              verbose mode\n"
            "   -h              show help\n",
            prog);
}

int main(int argc, char** argv) {
    uint64_t max_messages = 0;
    bool verbose = false;
    bool enable_recovery = false;
    uint64_t start_seq = 0;
    bool has_start_seq = false;

    int opt;
    while ((opt = getopt(argc, argv, "gs:n:vh")) != -1) {
        switch (opt) {
            case 'g':
                enable_recovery = true;
                break;

            case 's': {
                char* end = 0;
                unsigned long long v = std::strtoull(optarg, &end, 10);
                if (end == optarg || *end != '\0') {
                    std::fprintf(stderr, "Invalid -s value: %s\n", optarg);
                    usage(argv[0]);
                    return 1;
                }
                start_seq = (uint64_t)v;
                has_start_seq = true;
                break;
            }
                
            case 'n': {
                char* end = 0;
                unsigned long long v = std::strtoull(optarg, &end, 10);
                if (end == optarg || *end != '\0') {
                    std::fprintf(stderr, "Invalid -n value: %s\n", optarg);
                    usage(argv[0]);
                    return 1;
                }
                max_messages = (uint64_t)v;
                break;
            }

            case 'v':
                verbose = true;
                break;

            case 'h':
                usage(argv[0]);
                return 0;

            default:
                usage(argv[0]);
                return 1;
        }
    }

    Application app;
    app.set_max_messages(max_messages);
    app.set_verbose(verbose);

    return app.run();
}
