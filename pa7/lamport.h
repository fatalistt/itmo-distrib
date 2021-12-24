#ifndef _TARASOV_DMTR_LAB2_LAMPORT_H
#define _TARASOV_DMTR_LAB2_LAMPORT_H

#include "ipc.h"

timestamp_t *get_lamport_time();

timestamp_t *increment_lamport_time(local_id selfId);

timestamp_t *update_lamport_time(const timestamp_t *incoming);

#endif // _TARASOV_DMTR_LAB2_LAMPORT_H
