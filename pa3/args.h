#ifndef _TARASOV_DMTR_LAB2_ARGS_H
#define _TARASOV_DMTR_LAB2_ARGS_H

#include "banking.h"

typedef struct {
    int n;
    balance_t *s;
} Arguments;

Arguments parseArgs(int argc, char *const *argv);

void validateArgs(const Arguments *args);

#endif // _TARASOV_DMTR_LAB2_ARGS_H
