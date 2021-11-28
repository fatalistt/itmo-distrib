#include "args.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Arguments parseArgs(int argc, char *const *argv) {
    Arguments args = { 0 };

    if (argc < 3) {
        return args;
    }

    if (strcmp(argv[1], "-p") != 0) {
        return args;
    }

    long n = strtol(argv[2], NULL, 10);
    if (n > MAX_PROCESS_ID || n < 1) {
        return args;
    }
    if (argc > 3 + n) {
        return args;
    }
    long *s = calloc(n, sizeof(long));
    for (int i = 0; i < n; ++i) {
        s[i] = strtol(argv[3 + i], NULL, 10);
    }
    args.n = (int) n;
    args.s = calloc(n, sizeof(int));
    for (int i = 0; i < n; ++i) {
        args.s[i] = (int) s[i];
    }
    free(s);
    return args;
}

void printExampleUsage(FILE *__restrict stream) {
    fprintf(stream, "Example usage:\npa3 -p 3 10 20 30\n");
}

void validateArgs(const Arguments *args) {
    if (args->n < 2 || args->n > MAX_PROCESS_ID) {
        fprintf(stderr, "N constraint: 1 < n < %d\n", MAX_PROCESS_ID + 1);
        printExampleUsage(stderr);
        exit(1);
    }

    if (args->s == NULL) {
        fprintf(stderr, "S[] required. Count must match N");
        printExampleUsage(stderr);
        exit(1);
    }

    for (int i = 0; i < args->n; ++i) {
        long s = args->s[i];
        if (s < 1 || s > 99) {
            fprintf(stderr, "S constraint: 0 < n < 100\n");
            printExampleUsage(stderr);
            exit(1);
        }
    }
}
