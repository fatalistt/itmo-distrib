#define _POSIX_C_SOURCE 199309L
#include "ipc.h"
#include "pipe.h"
#include "lamport.h"
#include <unistd.h>
#include <time.h>
#include <errno.h>

const struct timespec sleepTimeout = {
        0,
        500000000
};

int send(void *self, local_id dst, const Message *msg) {
    char *buf = (void *)msg;
    const AccountInfo *info = (const AccountInfo *) self;
    size_t toWrite = sizeof(MessageHeader);
    ssize_t written = 0;
    while (written != toWrite) {
        ssize_t res = write(info->pipes[dst].write, buf + written, toWrite - written);
        if (res == -1) {
            return -1;
        }
        written += res;
    }

    toWrite = msg->s_header.s_payload_len;
    written = 0;
    while (written != toWrite) {
        ssize_t res = write(info->pipes[dst].write, msg->s_payload + written, toWrite - written);
        if (res == -1) {
            return -1;
        }
        written += res;
    }

    return 0;
}

int send_multicast(void *self, const Message *msg) {
    const AccountInfo *info = (const AccountInfo *) self;
    for (local_id i = 0; i < info->totalProcessesCount; i++) {
        if (i == info->selfId) {
            continue;
        }
        if (send(self, i, msg) == -1) {
            return -1;
        }
    }
    return 0;
}

int receive(void *self, local_id from, Message *msg) {
    char *buf = (void *)msg;
    const AccountInfo *info = (const AccountInfo *) self;
    size_t toRead = sizeof(MessageHeader);
    ssize_t wasRead = 0;
    while(wasRead != toRead) {
        ssize_t res = read(info->pipes[from].read, buf + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }

    toRead = msg->s_header.s_payload_len;
    wasRead = 0;
    while (wasRead != toRead) {
        ssize_t res = read(info->pipes[from].read, msg->s_payload + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }

    update_lamport_time(msg->s_header.s_local_time);

    return 0;
}

int receive_any(void *self, Message *msg) {
    char *buf = (void *)msg;
    const AccountInfo *info = (const AccountInfo *) self;
    int toRead = sizeof(MessageHeader);
    local_id pipe = -1;
    ssize_t wasRead = 0;
    do {
        if (++pipe == info->totalProcessesCount) {
            pipe = 0;
            nanosleep(&sleepTimeout, NULL);
        }
        if (pipe != info->selfId) {
            wasRead = read(info->pipes[pipe].read, buf, toRead);
            if (wasRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                return -1;
            }
        }
    } while (wasRead == 0 || wasRead == -1);
    while (wasRead != toRead) {
        ssize_t res = read(info->pipes[pipe].read, buf + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }
    toRead = msg->s_header.s_payload_len;
    wasRead = 0;
    while (wasRead != toRead) {
        ssize_t res = read(info->pipes[pipe].read, msg->s_payload + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }

    update_lamport_time(msg->s_header.s_local_time);

    return 0;
}
