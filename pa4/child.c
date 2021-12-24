#include "child.h"
#include "ipc.h"
#include "pa2345.h"
#include "lamport.h"
#include "banking.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int doneProcesses = 0;

void syncStart(const PipeInfo *info, int parentPid) {
    char buffer[MAX_PAYLOAD_LEN + 1];
    fprintf(stdout, log_started_fmt, get_lamport_time(), info->selfId, getpid(), parentPid, 0);
    fprintf(info->eventLog, log_started_fmt, get_lamport_time(), info->selfId, getpid(), parentPid, 0);
    int payloadLength = snprintf(buffer, MAX_PAYLOAD_LEN, log_started_fmt, get_lamport_time(), info->selfId,
                                 getpid(), parentPid, 0);

    Message message;
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = payloadLength;
    message.s_header.s_type = STARTED;
    message.s_header.s_local_time = increment_lamport_time();
    memcpy(&message.s_payload, buffer, MAX_PAYLOAD_LEN);
    send_multicast((void *) info, &message);
    for (local_id i = 1; i < info->totalProcessesCount; i++) {
        if (i == info->selfId) {
            continue;
        }
        memset(&message, 0, sizeof(Message));
        if (receive((void *) info, i, &message) || message.s_header.s_type != STARTED) {
            exit(-3);
        }
    }

    fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), info->selfId);
    fprintf(info->eventLog, log_received_all_started_fmt, get_lamport_time(), info->selfId);
}

void syncStop(const PipeInfo *info) {
    char buffer[MAX_PAYLOAD_LEN + 1];
    Message message = {{0}};
    fprintf(stdout, log_done_fmt, get_lamport_time(), info->selfId, 0);
    fprintf(info->eventLog, log_done_fmt, get_lamport_time(), info->selfId, 0);
    uint16_t payloadLength = snprintf(buffer, MAX_PAYLOAD_LEN, log_done_fmt, get_lamport_time(), info->selfId,
                                      0);
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = payloadLength;
    message.s_header.s_type = DONE;
    message.s_header.s_local_time = increment_lamport_time();
    memcpy(&message.s_payload, buffer, MAX_PAYLOAD_LEN);
    send_multicast((void *) info, &message);
    while (doneProcesses != info->totalProcessesCount - 2) {
        MessageInfo receivedInfo = { info, 0 };
        memset(&message, 0, sizeof(Message));
        receive_any((void *) &receivedInfo, &message);
        switch (message.s_header.s_type) {
            case DONE:
                ++doneProcesses;
                break;
            case CS_REQUEST:
                message.s_header.s_type = CS_REPLY;
                message.s_header.s_local_time = increment_lamport_time();
                send((void *) info, receivedInfo.sender, &message);
                break;
        }
//        if (receive((void *) info, i, &message) || message.s_header.s_type != DONE) {
//            exit(-4);
//        }
    }
    for (int i = 1; i < info->totalProcessesCount; i++) {
        if (i == info->selfId) {
            continue;
        }
    }

    fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), info->selfId);
    fprintf(info->eventLog, log_received_all_done_fmt, get_lamport_time(), info->selfId);
}

void printIteration(const PipeInfo *info, int iteration, int iterationsCount, bool useCriticalSection) {
    if (useCriticalSection) {
        request_cs((void *)info);
    }

    char buffer[MAX_PAYLOAD_LEN + 1];
    snprintf(buffer, MAX_PAYLOAD_LEN, log_loop_operation_fmt, info->selfId, iteration, iterationsCount);
    print(buffer);

    if (useCriticalSection) {
        release_cs((void *)info);
    }
}

void doChildJob(const PipeInfo *info, int parentPid, bool useCriticalSection) {
    syncStart(info, parentPid);

    int N = info->selfId * 5;
    for (int i = 1; i <= N; ++i) {
        printIteration(info, i, N, useCriticalSection);
    }

    syncStop(info);
}
