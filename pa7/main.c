#define _GNU_SOURCE

#include "args.h"
#include "C.h"
#include "ipc.h"
#include "K.h"
#include "pipe.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>

local_id startChildProcesses(int count) {
    for (int i = 1; i <= count; ++i) {
        pid_t newPid = fork();
        if (newPid == -1) {
            fprintf(stderr, "unable to create child process\n");
            return -1;
        }
        if (newPid == 0) {
            return (local_id) i;
        }
    }
    return PARENT_ID;
}

void close_pipe(int *pipe) {
    close(*pipe);
    *pipe = 0;
}

int main(int argc, char *argv[]) {
    Arguments args = parseArgs(argc, argv);
    validateArgs(&args);
    FILE *pipesLog = fopen("pipes.log", "w");
    FILE *eventsLog = fopen("events.log", "w");
    int totalProcessesCount = (int) args.n + 1;
    Pipe pipes[totalProcessesCount][totalProcessesCount];
    for (int i = 0; i < totalProcessesCount; ++i) {
        int created[2];
        for (int j = 0; j < totalProcessesCount; ++j) {
            if (i == j) {
                pipes[i][j].read = 0;
                pipes[i][j].write = 0;
                continue;
            }
            int res = pipe2(created, O_NONBLOCK);
            if (res == -1) {
                fprintf(stderr, "unable to create pipe");
                exit(-1);
            }
            pipes[i][j].read = created[0];
            pipes[i][j].write = created[1];
        }
    }

    for (int i = 0; i < totalProcessesCount; i++) {
        for (int j = 0; j < totalProcessesCount; j++) {
            fprintf(pipesLog, "pipe: %d %d | read: %d | write: %d\n", i, j, pipes[i][j].read, pipes[i][j].write);
            fprintf(stdout, "pipe: %d %d | read: %d | write: %d\n", i, j, pipes[i][j].read, pipes[i][j].write);
        }
    }

    int parentId = getpid();
    local_id id = startChildProcesses(args.n);
    bool isParent = id == PARENT_ID;
    Pipe myPipes[totalProcessesCount];

    for (int i = 0; i < totalProcessesCount; ++i) {
        for (int j = 0; j < totalProcessesCount; ++j) {
            if (j == id) {
                myPipes[i].read = pipes[i][id].read;
            } else {
                close_pipe(&pipes[i][j].read);
            }

            if (i == id) {
                myPipes[j].write = pipes[id][j].write;
            } else {
                close_pipe(&pipes[i][j].write);
            }
        }
    }

    for (int i = 0; i < totalProcessesCount; i++) {
        fprintf(pipesLog, "id: %d | my %d | read: %d | write: %d\n", id, i, myPipes[i].read, myPipes[i].write);
        fprintf(stdout, "id: %d | my %d | read: %d | write: %d\n", id, i, myPipes[i].read, myPipes[i].write);
    }

    if (isParent) {
        AccountInfo info = {
                totalProcessesCount,
                id,
                myPipes,
                eventsLog };
        doKJob(&info);
        for (int i = 1; i < totalProcessesCount; ++i) {
            wait(NULL);
        }
    } else {
        AccountInfo info = {
                totalProcessesCount,
                id,
                myPipes,
                eventsLog };
        doCJob(&info, parentId, args.s[id - 1]);
    }

    free(args.s);
    args.s = NULL;
}
