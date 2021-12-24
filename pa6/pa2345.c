#include <stdbool.h>
#include <stdlib.h>
#include "pa2345.h"
#include "pipe.h"
#include "lamport.h"
#include "banking.h"

extern int doneProcesses;

typedef enum {
    Thinking = 0,
    Hungry = 1,
    Eating = 2
} State;

typedef struct {
    bool isMine;
    bool isClean;
} Fork;

bool isInitialized = false;

State currentState;
Fork forks[MAX_PROCESS_ID + 1];
bool isAwaitingFork[MAX_PROCESS_ID + 1];

void printForks(const PipeInfo *info) {
    if (true) {
        return;
    }

    printf("%d: %d forks [", get_lamport_time(), info->selfId);
    for (int i = 1; i < info->totalProcessesCount; ++i) {
        if (i > 1) {
            printf(", ");
        }
        printf("(%d, %d)", forks[i].isMine, forks[i].isClean);
    }
    printf("] awaitings: [");
    for (int i = 1; i < info->totalProcessesCount; ++i) {
        if (i > 1) {
            printf(", ");
        }
        printf("%d", isAwaitingFork[i]);
    }
    printf("]\n");
}

void initialize(const PipeInfo *info) {
    currentState = Thinking;
    for (int i = 1; i < info->totalProcessesCount; ++i) {
        if (i >= info->selfId) {
            forks[i].isMine = true;
            forks[i].isClean = true;
        } else {
            forks[i].isMine = false;
            forks[i].isClean = false;
        }
        isAwaitingFork[i] = false;
    }
    isInitialized = true;
}

int request_cs(const void *self) {
    const PipeInfo *info = (PipeInfo *)self;
    if (!isInitialized) {
        initialize(info);
    }
    printForks(info);

    currentState = Hungry;
    for (local_id i = 1; i < info->totalProcessesCount; ++i) {
        if (i == info->selfId) {
            continue;
        }
        if (!forks[i].isMine) {
            Message message = {{0}};
            message.s_header.s_magic = MESSAGE_MAGIC;
            message.s_header.s_payload_len = 0;
            message.s_header.s_type = CS_REQUEST;
            message.s_header.s_local_time = increment_lamport_time();
            send((void *) self, i, &message);
            isAwaitingFork[i] = false;
        }
    }

    int total;
    do {
        total = 1;
        for (int i = 1; i < info->totalProcessesCount; ++i) {
            if (i != info->selfId && forks[i].isMine) {
                if (forks[i].isClean || !isAwaitingFork[i]) {
                    ++total;
                }
            }
        }
        if (total == info->totalProcessesCount - 1) {
            break;
        } else {
            printForks(info);
        }

        MessageInfo received_info = { info, 0 };
        Message message = { 0 };
        receive_any(&received_info, &message);
        switch (message.s_header.s_type) {
            case DONE:
                ++doneProcesses;
                break;
            case CS_REQUEST:
                isAwaitingFork[received_info.sender] = true;
                if (!forks[received_info.sender].isClean) {
                    forks[received_info.sender].isMine = false;
                    forks[received_info.sender].isClean = true;

                    Message message2 = {{0}};
                    message2.s_header.s_magic = MESSAGE_MAGIC;
                    message2.s_header.s_payload_len = 0;
                    message2.s_header.s_type = CS_REPLY;
                    message2.s_header.s_local_time = increment_lamport_time();
                    send((void *) self, received_info.sender, &message2);
                    isAwaitingFork[received_info.sender] = false;

                    message2.s_header.s_type = CS_REQUEST;
                    message2.s_header.s_local_time = increment_lamport_time();
                    send((void *) self, received_info.sender, &message2);
                }
                break;
            case CS_REPLY:
                forks[received_info.sender].isMine = true;
                forks[received_info.sender].isClean = true;
                break;
        }
    } while (total < info->totalProcessesCount - 1);
//    printf("%d: start eating in %d\n", get_lamport_time(), info->selfId);

    currentState = Eating;

    return 0;
}

int release_cs(const void *self) {
    const PipeInfo *info = (PipeInfo *)self;
    printForks(info);
    currentState = Thinking;
    for (int i = 1; i < info->totalProcessesCount; ++i) {
        if (i != info->selfId) {
            forks[i].isClean = false;
        }
    }
    Message message = { { 0 } };
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = 0;
    message.s_header.s_type = CS_REPLY;
    message.s_header.s_type = increment_lamport_time();
    for (local_id i = 1; i < info->totalProcessesCount; ++i) {
        if (i != info->selfId && isAwaitingFork[i]) {
            forks[i].isMine = false;
            send((void *) info, i, &message);
            isAwaitingFork[i] = false;
        }
    }
    printForks(info);

    return 0;
}
