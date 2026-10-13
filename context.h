#ifndef CONTEXT_H
#define CONTEXT_H

#include <set>
#include <string>

#include "common.h"

typedef struct Context
{
    Context(int argc, char** argv);

    int verbose;
    const char* exec_name;
    const char* path;
    u64 minsize;
    bool ignore_empty_files;
    bool keep_shortest;
    bool dry_run;
    bool interactive;
    std::set<std::string> exclude_dirs;

    void set_defaults() {
        verbose = 0;
        minsize = 0LL;
        ignore_empty_files = false;
        keep_shortest = false;
        dry_run = false;
        interactive = false;
    }

    void usage(int status);
} Context;

#endif
