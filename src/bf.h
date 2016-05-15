#ifndef BF_H_
#define BF_H_

#include <stdbool.h>

struct _options {
    bool debug;
    bool dump_jit;
    bool pretty_print;
    int optimize_level;
};

struct _options bf_options;


#endif