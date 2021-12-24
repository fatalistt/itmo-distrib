#include "C.h"
#include "ipc.h"
#include "pa2345.h"
#include "banking.h"
#include "lamport.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

balance_t currentBalance = 0;
struct {
    bool hasValue;
    timestamp_t value;
} sendBalanceTimestamp;

void syncStart(const AccountInfo *info, int parentPid) {
    char buffer[MAX_PAYLOAD_LEN + 1];
    fprintf(stdout, log_started_fmt, get_lamport_time()[info->selfId], info->selfId, getpid(), parentPid,
            currentBalance);
    fprintf(info->eventLog, log_started_fmt, get_lamport_time()[info->selfId], info->selfId, getpid(), parentPid,
            currentBalance);
    int payloadLength = snprintf(buffer, MAX_PAYLOAD_LEN, log_started_fmt, get_lamport_time()[info->selfId],
                                 info->selfId,
                                 getpid(), parentPid, currentBalance);

    Message message;
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = payloadLength;
    message.s_header.s_type = STARTED;
    memcpy(message.s_header.s_local_timevector, increment_lamport_time(info->selfId),
           sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
    memcpy(&message.s_payload, buffer, MAX_PAYLOAD_LEN);
    send_multicast((void *) info, &message);
    for (local_id i = 1; i < info->totalProcessesCount; i++) {
        if (i == info->selfId) {
            continue;
        }
        memset(&message, 0, sizeof(Message));
        if (receive((void *) info, i, &message) || message.s_header.s_type != STARTED) {
            exit(1);
        }
    }

    fprintf(stdout, log_received_all_started_fmt, get_lamport_time()[info->selfId], info->selfId);
    fprintf(info->eventLog, log_received_all_started_fmt, get_lamport_time()[info->selfId], info->selfId);
}

void sendTransfer(const AccountInfo *info, const TransferOrder *order, const Message *message) {
    fprintf(stdout, log_transfer_out_fmt, get_lamport_time()[info->selfId], info->selfId, order->s_amount,
            order->s_dst);
    fprintf(info->eventLog, log_transfer_out_fmt, get_lamport_time()[info->selfId], info->selfId, order->s_amount,
            order->s_dst);

    Message copy;
    memcpy(&copy, message, sizeof(Message));
    memcpy(copy.s_header.s_local_timevector, increment_lamport_time(info->selfId),
           sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
    currentBalance -= order->s_amount;
    send((void *) info, order->s_dst, &copy);
}

void receiveTransfer(const AccountInfo *info, TransferOrder *order) {
    fprintf(stdout, log_transfer_in_fmt, get_lamport_time()[info->selfId], info->selfId, order->s_amount, order->s_src);
    fprintf(info->eventLog, log_transfer_in_fmt, get_lamport_time()[info->selfId], info->selfId, order->s_amount,
            order->s_src);

    currentBalance += order->s_amount;

    Message message = {{0}};
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = 0;
    message.s_header.s_type = ACK;
    memcpy(message.s_header.s_local_timevector, increment_lamport_time(info->selfId),
           sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
    send((void *) info, PARENT_ID, &message);
}

void doTransfer(const AccountInfo *info, const Message *message) {
    TransferOrder order = {0};
    memcpy(&order, message->s_payload, sizeof(TransferOrder));
    printf("%d: process %d transfer %d from %d to %d\n", get_lamport_time()[info->selfId], info->selfId, order.s_amount,
           order.s_src, order.s_dst);
    if (order.s_src == info->selfId) {
        sendTransfer(info, &order, message);
    } else if (order.s_dst == info->selfId) {
        receiveTransfer(info, &order);
    } else {
        exit(1);
    }
}

void doStop(const AccountInfo *info) {
    char buffer[MAX_PAYLOAD_LEN + 1];
    Message message = {{0}};
    fprintf(stdout, log_done_fmt, get_lamport_time()[info->selfId], info->selfId, currentBalance);
    fprintf(info->eventLog, log_done_fmt, get_lamport_time()[info->selfId], info->selfId, currentBalance);
    uint16_t payloadLength = snprintf(buffer, MAX_PAYLOAD_LEN, log_done_fmt, get_lamport_time()[info->selfId],
                                      info->selfId,
                                      currentBalance);
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = payloadLength;
    message.s_header.s_type = DONE;
    memcpy(message.s_header.s_local_timevector, increment_lamport_time(info->selfId),
           sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
    memcpy(&message.s_payload, buffer, MAX_PAYLOAD_LEN);
    send_multicast((void *) info, &message);

    int totalDone = 1;
    while (totalDone != info->totalProcessesCount - 1) {
        memset(&message, 0, sizeof(Message));
        receive_any((void *) info, &message);
        switch (message.s_header.s_type) {
            case TRANSFER:
                doTransfer(info, &message);
                break;
            case DONE:
                ++totalDone;
                break;
            default:
                exit(1);
        }
    }
    fprintf(stdout, log_received_all_done_fmt, get_lamport_time()[info->selfId], info->selfId);
    fprintf(info->eventLog, log_received_all_done_fmt, get_lamport_time()[info->selfId], info->selfId);
}

void doCJob(const AccountInfo *info, int parentPid, balance_t initialBalance) {
    currentBalance = initialBalance;
    syncStart(info, parentPid);

    Message message;
    while (true) {
        memset(&message, 0, sizeof(Message));
        receive_any((void *) info, &message);
        switch (message.s_header.s_type) {
            case TRANSFER:
                doTransfer(info, &message);
                break;
            case STOP:
                doStop(info);
                return;
            case SNAPSHOT_VTIME:
                sendBalanceTimestamp.hasValue = true;
                memcpy(&sendBalanceTimestamp.value, message.s_payload, sizeof(timestamp_t));
                memset(&message, 0, sizeof(Message));
                message.s_header.s_magic = MESSAGE_MAGIC;
                message.s_header.s_type = SNAPSHOT_ACK;
                memcpy(message.s_header.s_local_timevector, get_lamport_time(),
                       sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
                send((void *) info, PARENT_ID, &message);
                break;
            case EMPTY:
                break;
            default:
                exit(1);
        }
        if (sendBalanceTimestamp.hasValue && get_lamport_time()[PARENT_ID] >= sendBalanceTimestamp.value) {
//            printf("%d: process %d sent balance %d to %d\n", get_lamport_time()[info->selfId], info->selfId, currentBalance, PARENT_ID);
            memset(&message, 0, sizeof(Message));
            sendBalanceTimestamp.hasValue = false;
            BalanceState state = {
                    currentBalance,
                    0,
                    0,
                    {0},
            };
            memcpy(state.s_timevector, get_lamport_time(), sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
            message.s_header.s_magic = MESSAGE_MAGIC;
            message.s_header.s_type = BALANCE_STATE;
            message.s_header.s_payload_len = sizeof(BalanceState);
            memcpy(message.s_header.s_local_timevector, increment_lamport_time(info->selfId),
                   sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
            memcpy(message.s_payload, &state, sizeof(BalanceState));
            send((void *) info, PARENT_ID, &message);
        }
    }
}
