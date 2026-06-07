#include <cstdio>
#include <cstdlib>
#include <getopt.h>

#include "context.h"

void Context::usage(int status)
{
    printf("Usage: %s [options] <path>\n", exec_name);
    printf("Options:\n");
    printf(" --minsize, -m <size> : ignore files smaller than <size>\n");
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
            { "minsize", required_argument, 0, 'm' }
        };

        int c = getopt_long(argc, argv, "hvm:", long_options, &option_index);
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

