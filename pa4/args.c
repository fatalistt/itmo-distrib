#include "args.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

enum {
    USE_CRITICAL_SECTION = 0xABCD
};

Arguments parseArgs(int argc, char *const *argv) {
    Arguments args = {0};

    struct option longOptions[2] = { {0} };
    struct option mutexl = {
            "mutexl",
            no_argument,
            NULL,
            USE_CRITICAL_SECTION
    };
    longOptions[0] = mutexl;

    int opt;
    long n;
    do {
        opt = getopt_long(argc, argv, "p:", longOptions, NULL);
        switch (opt) {
            case 'p':
                n = strtol(optarg, NULL, 10);
                if (n > MAX_PROCESS_ID || n < 1) {
                    break;
                }
                args.n = (int) n;
                break;
            case USE_CRITICAL_SECTION:
                args.useCriticalSection = true;
                break;
            case '?':
            case ':':
            case -1:
                break;
            default:
                exit(-5);
        }
    } while (opt != -1);

    return args;
}

void printExampleUsage(FILE *__restrict stream) {
    fprintf(stream, "Example usage:\npa4 -n 3\n");
}

void validateArgs(const Arguments *args) {
    if (args->n < 2 || args->n > MAX_PROCESS_ID) {
        fprintf(stderr, "N constraint: 1 < n < %d\n", MAX_PROCESS_ID + 1);
        printExampleUsage(stderr);
        exit(-6);
    }
}
