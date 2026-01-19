#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>

class Application {
public:
    Application();
    
    void set_max_messages(uint64_t max_messages);
    void set_verbose(bool verbose);
    void set_type_filter(char type);

    int run();

private:
    uint64_t max_messages;
    bool verbose;
    bool has_type_filter;
    bool type_allowed[256];
};

#endif
