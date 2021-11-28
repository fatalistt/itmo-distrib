#include <memory.h>
#include <stdlib.h>
#include "banking.h"
#include "lamport.h"
#include "pipe.h"

timestamp_t time = 0;

timestamp_t get_lamport_time() {
    return time;
}

timestamp_t increment_lamport_time() {
    return ++time;
}

timestamp_t update_lamport_time(timestamp_t incoming) {
    if (incoming > time) {
        time = incoming;
    }
    return ++time;
}

void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
    const AccountInfo *info = parent_data;
    const TransferOrder order = {src, dst, amount};
    Message message = { { 0 } };
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = sizeof(TransferOrder);
    message.s_header.s_type = TRANSFER;
    message.s_header.s_local_time = increment_lamport_time();
    memcpy(message.s_payload, &order, message.s_header.s_payload_len);
    send(parent_data, src, &message);
    memset(&message, 0, sizeof(Message));
    if (receive((void *) info, dst, &message) || message.s_header.s_type != ACK) {
        exit(1);
    }
}
