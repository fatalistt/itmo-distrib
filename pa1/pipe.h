#ifndef _TARASOV_DMTR_LAB1_PIPE_H
#define _TARASOV_DMTR_LAB1_PIPE_H

#include "ipc.h"
#include <stdio.h>

typedef struct
{
    int read;
    int write;
} Pipe;

typedef struct
{
    int totalProcessesCount;
    local_id selfId;
    Pipe *pipes;
    FILE *eventLog;
} PipeInfo;

#endif // _TARASOV_DMTR_LAB1_PIPE_H
