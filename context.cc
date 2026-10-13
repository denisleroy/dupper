#include <cstdio>
#include <cstdlib>
#include <string>
#include <getopt.h>

#include "context.h"

void Context::usage(int status)
{
    printf("Usage: %s [options] <path>\n", exec_name);
    printf("Options:\n");
    printf(" --minsize, -m <size> : ignore files smaller than <size>\n");
    printf(" --exclude-dir <name> : skip directories named <name> (repeatable)\n");
    printf(" --keep-shortest      : when all copies of a file are in the same directory,\n");
    printf("                        delete all but the one with the shortest name\n");
    printf("  -n                  : with --keep-shortest, don't delete anything; write\n");
    printf("                        the rm commands to ./DELETE.sh instead\n");
    printf("  -v                  : increase verbosity\n");
    printf("  -h                  : this message\n");
    exit(status);
}

Context::Context(int argc, char** argv)
{
    set_defaults();
    exec_name = argv[0];

    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            { "scan", no_argument, 0, 's' },
            { "minsize", required_argument, 0, 'm' },
            { "exclude-dir", required_argument, 0, 'x' },
            { "keep-shortest", no_argument, 0, 'k' },
            { 0, 0, 0, 0 }
        };

        int c = getopt_long(argc, argv, "hvnm:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 's':
            printf("--scan\n");
            break;
        case 'v':
            verbose++;
            break;
        case 'm':
            minsize = atoll(optarg);
            break;
        case 'k':
            keep_shortest = true;
            break;
        case 'n':
            dry_run = true;
            break;
        case 'x': {
            // Match on the folder name, so drop any trailing slashes.
            std::string name(optarg);
            while (name.size() > 1 && name.back() == '/')
                name.pop_back();
            exclude_dirs.insert(name);
            break;
        }
        case 'h':
            usage(0);
        default:
            usage(-1);
        }
    }

    if (optind >= argc) {
        usage(-1);
    }

    path = argv[optind];
}

