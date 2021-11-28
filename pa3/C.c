#include "C.h"
#include "ipc.h"
#include "pa2345.h"
#include "banking.h"
#include "lamport.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

balance_t currentBalance = 0;

void updateBalanceHistoryPending(BalanceHistory *history, balance_t balance, balance_t pending, timestamp_t sentAt) {
    uint8_t length = history->s_history_len;
    timestamp_t currentLamportTime = get_lamport_time();
    if (length > 0) {
        BalanceState lastState = history->s_history[length - 1];
        if (lastState.s_time == currentLamportTime) {
            length -=1;
        }
        while (lastState.s_time < currentLamportTime) {
            lastState.s_time += 1;
            history->s_history[length++] = lastState;
        }
    }
    if (pending > 0) {
        for (int i = 0; i < length; ++i) {
            if (history->s_history[i].s_time >= sentAt) {
                history->s_history[i].s_balance_pending_in += pending;
            }
        }
    }

    history->s_history[length].s_balance = balance;
    history->s_history[length].s_time = currentLamportTime;
    history->s_history_len = ++length;
}

void updateBalanceHistory(BalanceHistory *history, balance_t balance) {
    updateBalanceHistoryPending(history, balance, 0, 0);
}

void syncStart(const AccountInfo *info, int parentPid) {
    char buffer[MAX_PAYLOAD_LEN + 1];
    fprintf(stdout, log_started_fmt, get_lamport_time(), info->selfId, getpid(), parentPid, currentBalance);
    fprintf(info->eventLog, log_started_fmt, get_lamport_time(), info->selfId, getpid(), parentPid, currentBalance);
    int payloadLength = snprintf(buffer, MAX_PAYLOAD_LEN, log_started_fmt, get_lamport_time(), info->selfId,
                                 getpid(), parentPid, currentBalance);

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
            exit(1);
        }
    }

    fprintf(stdout, log_received_all_started_fmt, get_lamport_time(), info->selfId);
    fprintf(info->eventLog, log_received_all_started_fmt, get_lamport_time(), info->selfId);
}

void sendTransfer(const AccountInfo *info, const TransferOrder *order, const Message *message, BalanceHistory *history) {
    fprintf(stdout, log_transfer_out_fmt, get_lamport_time(), info->selfId, order->s_amount, order->s_dst);
    fprintf(info->eventLog, log_transfer_out_fmt, get_lamport_time(), info->selfId, order->s_amount, order->s_dst);

    Message copy;
    memcpy(&copy, message, sizeof(Message));
    copy.s_header.s_local_time = increment_lamport_time();
    currentBalance -= order->s_amount;
    updateBalanceHistory(history, currentBalance);
    send((void *) info, order->s_dst, &copy);
}

void receiveTransfer(const AccountInfo *info, TransferOrder *order, timestamp_t sentAt, BalanceHistory *history) {
    fprintf(stdout, log_transfer_in_fmt, get_lamport_time(), info->selfId, order->s_amount, order->s_src);
    fprintf(info->eventLog, log_transfer_in_fmt, get_lamport_time(), info->selfId, order->s_amount, order->s_src);

    currentBalance += order->s_amount;
    updateBalanceHistoryPending(history, currentBalance, order->s_amount, sentAt);

    Message message = { { 0 } };
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = 0;
    message.s_header.s_type = ACK;
    message.s_header.s_local_time = increment_lamport_time();
    send((void *) info, PARENT_ID, &message);
}

void doTransfer(const AccountInfo *info, const Message *message, BalanceHistory *history) {
    TransferOrder order = { 0 };
    memcpy(&order, message->s_payload, sizeof(TransferOrder));
    if (order.s_src == info->selfId) {
        sendTransfer(info, &order, message, history);
    } else if (order.s_dst == info->selfId) {
        receiveTransfer(info, &order, message->s_header.s_local_time, history);
    } else {
        exit(1);
    }
}

void doStop(const AccountInfo *info, BalanceHistory *history) {
    char buffer[MAX_PAYLOAD_LEN + 1];
    Message message = { { 0 } };
    fprintf(stdout, log_done_fmt, get_lamport_time(), info->selfId, currentBalance);
    fprintf(info->eventLog, log_done_fmt, get_lamport_time(), info->selfId, currentBalance);
    uint16_t payloadLength = snprintf(buffer, MAX_PAYLOAD_LEN, log_done_fmt, get_lamport_time(), info->selfId,
                                      currentBalance);
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = payloadLength;
    message.s_header.s_type = DONE;
    message.s_header.s_local_time = increment_lamport_time();
    memcpy(&message.s_payload, buffer, MAX_PAYLOAD_LEN);
    send_multicast((void *) info, &message);

    int totalDone = 1;
    while (totalDone != info->totalProcessesCount - 1) {
        memset(&message, 0, sizeof(Message));
        receive_any((void *) info, &message);
        switch (message.s_header.s_type) {
            case TRANSFER:
                doTransfer(info, &message, history);
                break;
            case DONE:
                ++totalDone;
                break;
            default:
                exit(1);
        }
    }
    fprintf(stdout, log_received_all_done_fmt, get_lamport_time(), info->selfId);
    fprintf(info->eventLog, log_received_all_done_fmt, get_lamport_time(), info->selfId);

    updateBalanceHistory(history, currentBalance);

    memset(&message, 0, sizeof(Message));
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = sizeof(local_id) + sizeof(uint8_t) + history->s_history_len * sizeof(BalanceState);
    message.s_header.s_type = BALANCE_HISTORY;
    message.s_header.s_local_time = increment_lamport_time();
    memcpy(&message.s_payload, history, message.s_header.s_payload_len);
    send((void *) info, PARENT_ID, &message);
}

void doCJob(const AccountInfo *info, int parentPid, balance_t initialBalance) {
    currentBalance = initialBalance;
    BalanceHistory history = { 0 };
    history.s_id = info->selfId;
    updateBalanceHistory(&history, initialBalance);
    syncStart(info, parentPid);

    Message message = { { 0 } };
    while (1) {
        receive_any((void *) info, &message);
        switch (message.s_header.s_type) {
            case TRANSFER:
                doTransfer(info, &message, &history);
                break;
            case STOP:
                doStop(info, &history);
                return;
            default:
                exit(1);
        }
        memset(&message, 0, sizeof(Message));
    }
}
