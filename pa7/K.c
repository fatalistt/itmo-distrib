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
    memcpy(message.s_header.s_local_timevector, increment_lamport_time(info->selfId), sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
    send_multicast((void *)info, &message);
}

void doKJob(const AccountInfo *info) {
    wait_all(info, STARTED);
    fprintf(stdout, log_received_all_started_fmt, get_lamport_time()[info->selfId], info->selfId);
    fprintf(info->eventLog, log_received_all_started_fmt, get_lamport_time()[info->selfId], info->selfId);

    bank_robbery((void *)info, (local_id)(info->totalProcessesCount - 1));

    send_all(info, STOP);

    wait_all(info, DONE);
    fprintf(stdout, log_received_all_done_fmt, get_lamport_time()[info->selfId], info->selfId);
    fprintf(info->eventLog, log_received_all_done_fmt, get_lamport_time()[info->selfId], info->selfId);
}
