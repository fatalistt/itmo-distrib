#ifndef _TARASOV_DMTR_LAB1_ARGS_H
#define _TARASOV_DMTR_LAB1_ARGS_H

typedef struct
{
  int p;
} Arguments;

Arguments parseArgs(int argc, char *const *argv);

void validateArgs(const Arguments *args);

#endif // _TARASOV_DMTR_LAB1_ARGS_H
