#ifndef _TARASOV_DMTR_LAB1_CHILD_H
#define _TARASOV_DMTR_LAB1_CHILD_H

#include "args.h"
#include "pipe.h"
#include <sys/types.h>

void doChildJob(const PipeInfo *info, int parentPid, bool useCriticalSection);

#endif // _TARASOV_DMTR_LAB1_CHILD_H
