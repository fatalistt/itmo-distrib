#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>
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
    bool shouldSkip = true;
    if (shouldSkip) {
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

int calculateAvailableForks(const PipeInfo * info) {
    int availableForks = 0;
    for (int i = 1; i < info->totalProcessesCount; ++i) {
        if (i != info->selfId && forks[i].isMine) {
            if (forks[i].isClean || !isAwaitingFork[i]) {
                ++availableForks;
            }
        }
    }

    return availableForks;
}

void initialize(const PipeInfo *info) {
    currentState = Thinking;
    for (int i = 1; i < info->totalProcessesCount; ++i) {
        forks[i].isMine = i >= info->selfId;
        forks[i].isClean = !forks[i].isMine;
        isAwaitingFork[i] = !forks[i].isMine;
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
    Message message = {{0}};
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = 0;
    message.s_header.s_type = CS_REQUEST;
    message.s_header.s_local_time = increment_lamport_time();
    for (local_id i = 1; i < info->totalProcessesCount; ++i) {
        if (i != info->selfId && !forks[i].isMine) {
//            printf("%d: process %d requesting fork from %d\n", get_lamport_time(), info->selfId, i);
            send((void *) self, i, &message);
            isAwaitingFork[i] = false;
        }
    }

    do {
        int availableForks = calculateAvailableForks(info);
        int neededForks = info->totalProcessesCount - 2;
        MessageInfo received_info = { info, 0 };
        memset(&message, 0, sizeof(Message));

        if (neededForks == availableForks && doneProcesses == info->totalProcessesCount - 2) {
            break;
        }

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

//                    printf("%d: process %d transferred fork to %d\n", get_lamport_time(), info->selfId, received_info.sender);

                    message2.s_header.s_type = CS_REQUEST;
                    send((void *) self, received_info.sender, &message2);

//                    printf("%d: process %d requesting fork from %d\n", get_lamport_time(), info->selfId, received_info.sender);
                }
                break;
            case CS_REPLY:
//                printf("%d: process %d received fork from %d\n", get_lamport_time(), info->selfId, received_info.sender);
                forks[received_info.sender].isMine = true;
                forks[received_info.sender].isClean = true;
                printForks(info);
                break;
        }
        availableForks = calculateAvailableForks(info);
        if (availableForks == neededForks) {
            break;
        } else {
            printForks(info);
        }
    } while (true);
//    printf("%d: start eating in %d\n", get_lamport_time(), info->selfId);

    currentState = Eating;

    return 0;
}

int release_cs(const void *self) {
    const PipeInfo *info = (PipeInfo *)self;
//    printf("%d: releasing %d\n", get_lamport_time(), info->selfId);
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
    message.s_header.s_local_time = increment_lamport_time();
    for (local_id i = 1; i < info->totalProcessesCount; ++i) {
        if (i != info->selfId && isAwaitingFork[i]) {
            forks[i].isMine = false;
            send((void *) info, i, &message);
            isAwaitingFork[i] = false;

//            printf("%d: process %d transferred fork to %d\n", get_lamport_time(), info->selfId, i);
        }
    }
    printForks(info);

    return 0;
}
