#ifndef _TARASOV_DMTR_LAB2_PIPE_H
#define _TARASOV_DMTR_LAB2_PIPE_H

#include "ipc.h"
#include "banking.h"
#include <stdio.h>

typedef struct {
    int read;
    int write;
} Pipe;

typedef struct {
    int totalProcessesCount;
    local_id selfId;
    Pipe *pipes;
    FILE *eventLog;
} AccountInfo;

#endif // _TARASOV_DMTR_LAB2_PIPE_H
