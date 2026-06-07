#ifndef CONTEXT_H
#define CONTEXT_H

#include "common.h"

typedef struct Context
{
    Context(int argc, char** argv);

    int verbose;
    const char* exec_name;
    const char* path;
    u64 minsize;
    bool ignore_empty_files;

    void set_defaults() {
        verbose = 0;
        minsize = 0LL;
        ignore_empty_files = false;
    }

    void usage(int status);
} Context;

#endif
