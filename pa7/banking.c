#include <memory.h>
#include <stdlib.h>
#include <stdbool.h>
#include "banking.h"
#include "lamport.h"
#include "pipe.h"
#include "ipc.h"

void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
    const AccountInfo *info = parent_data;
    const TransferOrder order = {src, dst, amount};
    Message message = {{0}};
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = sizeof(TransferOrder);
    message.s_header.s_type = TRANSFER;
    memcpy(message.s_header.s_local_timevector, increment_lamport_time(info->selfId),
           sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
    memcpy(message.s_payload, &order, message.s_header.s_payload_len);
    send(parent_data, src, &message);
    memset(&message, 0, sizeof(Message));
    if (receive((void *) info, dst, &message) || message.s_header.s_type != ACK) {
        exit(1);
    }
}

void total_sum_snapshot(void *parent_data) {
    const AccountInfo *info = parent_data;
    timestamp_t balanceTime = get_lamport_time()[info->selfId] + 1;

    Message message = {{0}};
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = sizeof(timestamp_t);
    message.s_header.s_type = SNAPSHOT_VTIME;
    memcpy(message.s_header.s_local_timevector, get_lamport_time(), sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
    memcpy(message.s_payload, &balanceTime, sizeof(timestamp_t));
    send_multicast(parent_data, &message);
//    printf("%d: sent VTIME %d\n", get_lamport_time()[info->selfId], balanceTime);
    for (local_id i = 1; i < info->totalProcessesCount; ++i) {
        memset(&message, 0, sizeof(Message));
//        printf("%d: reading ACK from %d\n", get_lamport_time()[info->selfId], i);
        if (receive(parent_data, i, &message) || message.s_header.s_type != SNAPSHOT_ACK) {
            exit(1);
        }
//        printf("%d: ACK read from %d\n", get_lamport_time()[info->selfId], i);
    }

    memset(&message, 0, sizeof(Message));
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_type = EMPTY;
    memcpy(message.s_header.s_local_timevector, increment_lamport_time(info->selfId),
           sizeof(timestamp_t) * (MAX_PROCESS_ID + 1));
    send_multicast(parent_data, &message);
//    printf("%d: sent EMPTY\n", get_lamport_time()[info->selfId]);

    bool isBalanceReceived[MAX_PROCESS_ID + 1] = {0};
    int receivedBalancesCount = 1;
    balance_t totalBalance = 0;
    balance_t totalPendingBalance = 0;
    while (receivedBalancesCount != info->totalProcessesCount) {
        for (local_id i = 1; i < info->totalProcessesCount; ++i) {
            if (isBalanceReceived[i]) {
//                printf("%d: balance from %d received already. Skipping\n", get_lamport_time()[info->selfId], i);
                continue;
            }
            memset(&message, 0, sizeof(Message));
            if (receive(parent_data, i, &message) == 0 && message.s_header.s_type == BALANCE_STATE) {
                BalanceState balance;
                memcpy(&balance, message.s_payload, sizeof(BalanceState));
                totalBalance += balance.s_balance;
                totalPendingBalance += balance.s_balance_pending_in;
                isBalanceReceived[i] = true;
                ++receivedBalancesCount;
//                printf("%d: received balance (%d) from %d\n", get_lamport_time()[info->selfId], balance.s_balance, i);
            } else {
//                printf("%d: received %d, not balance %d from %d\n", get_lamport_time()[info->selfId], message.s_header.s_type, BALANCE_STATE, i);
            }
        }
    }
    fprintf(stdout, "[%d]: %d %d\n", balanceTime, totalBalance, totalPendingBalance);
}
