#ifndef _TARASOV_DMTR_LAB1_PIPE_H
#define _TARASOV_DMTR_LAB1_PIPE_H

#include "ipc.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    int read;
    int write;
} Pipe;

typedef struct {
    int totalProcessesCount;
    local_id selfId;
    Pipe *pipes;
    FILE *eventLog;
    bool useCriticalSection;
} PipeInfo;

typedef struct {
    const PipeInfo *pipeInfo;
    local_id sender;
} MessageInfo;

#endif // _TARASOV_DMTR_LAB1_PIPE_H
