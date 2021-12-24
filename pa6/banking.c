#include <stdlib.h>
#include "banking.h"
#include "lamport.h"

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
