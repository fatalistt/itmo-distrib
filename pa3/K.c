#include "K.h"
#include "ipc.h"
#include "pa2345.h"
#include "banking.h"
#include "lamport.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

void wait_all(const AccountInfo *info, int16_t expectedType) {
    Message message = { { 0 } };
    for (local_id i = 1; i < info->totalProcessesCount; ++i) {
        if (i == info->selfId) {
            continue;
        }
        memset(&message, 0, sizeof(Message));
        if (receive((void *) info, i, &message) || message.s_header.s_type != expectedType) {
            exit(1);
        }
    }
}

void send_all(const AccountInfo *info, int16_t type) {
    Message message = { { 0 } };
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = 0;
    message.s_header.s_type = type;
    message.s_header.s_local_time = increment_lamport_time();
    send_multicast((void *)info, &message);
}

void collect_all_history(const AccountInfo *info) {
    AllHistory allHistory = { 0 };
    allHistory.s_history_len = info->totalProcessesCount - 1;
    Message message = { { 0 } };
    for (local_id i = 1; i < info->totalProcessesCount; ++i) {
        if (i == info->selfId) {
            continue;
        }
        memset(&message, 0, sizeof(Message));
        if (receive((void *) info, i, &message) || message.s_header.s_type != BALANCE_HISTORY) {
            exit(1);
        }
        memcpy(&allHistory.s_history[i - 1], message.s_payload, message.s_header.s_payload_len);
    }
    print_history(&allHistory);
}

void doKJob(const AccountInfo *info) {
    wait_all(info, STARTED);
    fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), info->selfId);
    fprintf(info->eventLog, log_received_all_started_fmt, get_lamport_time(), info->selfId);

    bank_robbery((void *)info, (local_id)(info->totalProcessesCount - 1));

    send_all(info, STOP);

    wait_all(info, DONE);
    fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), info->selfId);
    fprintf(info->eventLog, log_received_all_done_fmt, get_lamport_time(), info->selfId);

    collect_all_history(info);
}
