#include "parent.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

void doParentJob(const PipeInfo *info) {
    int stoppedCount = 0;
    while (++stoppedCount < info->totalProcessesCount) {
        int status;
        pid_t pid = wait(&status);
        if (pid == -1) {
            fprintf(stderr, "unable to wait\n");
            exit(-7);
        }
    }
}
