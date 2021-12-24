#include "lamport.h"
#include "ipc.h"

timestamp_t times[MAX_PROCESS_ID + 1] = {0};

timestamp_t *get_lamport_time() {
    return times;
}

timestamp_t *increment_lamport_time(local_id id) {
    ++times[id];
    return times;
}

timestamp_t *update_lamport_time(const timestamp_t *incoming) {
    for (int i = 0; i <= MAX_PROCESS_ID; ++i) {
        if (incoming[i] > times[i]) {
            times[i] = incoming[i];
        }
    }
    return times;
}
