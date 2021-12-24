#define _POSIX_C_SOURCE 199309L

#include "ipc.h"
#include "pipe.h"
#include "lamport.h"
#include "banking.h"
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <error.h>

const struct timespec sleepTimeout = {
        0,
        500000000
};

int send(void *self, local_id dst, const Message *msg) {
    char *buf = (void *) msg;
    const PipeInfo *info = (const PipeInfo *) self;
    size_t toWrite = sizeof(MessageHeader);
    ssize_t written = 0;
//    printf("%d to %d (%d): starting with (%ld, %ld)\n", info->selfId, dst, msg->s_header.s_type, written, toWrite);
    while (written != toWrite) {
//        printf("%d: %d to %d (%d): write to %d, %ld, %ld\n", get_lamport_time(), info->selfId, dst, msg->s_header.s_type, info->pipes[dst].write, buf + written, toWrite - written);
        ssize_t res = write(info->pipes[dst].write, buf + written, toWrite - written);
        int errn = errno;
//        printf("%d to %d (%d): write result: (%ld, %d)\n", info->selfId, dst, msg->s_header.s_type, res, errn);
        if (res == -1) {
            error(0, errn, "%d: send", info->selfId);
            return -1;
        }
        written += res;
//        printf("%d to %d (%d): written %ld of %ld: [", info->selfId, dst, msg->s_header.s_type, written, toWrite);
//        for (int i = 0; i < res; ++i) {
//            if (i != 0) {
//                printf(", ");
//            }
//            printf("%d", *(buf + written - res + i));
//        }
//        printf("]\n");
    }

    toWrite = msg->s_header.s_payload_len;
    written = 0;
//    printf("%d to %d (%d): to write %ld\n", info->selfId, dst, msg->s_header.s_type, toWrite);
    while (written != toWrite) {
        ssize_t res = write(info->pipes[dst].write, msg->s_payload + written, toWrite - written);
        if (res == -1) {
            return -1;
        }
        written += res;
//        printf("%d: written %ld of %ld\n", info->selfId, written, toWrite);
    }
//    printf("%d to %d (%d): ending\n", info->selfId, dst, msg->s_header.s_type);

    return 0;
}

int send_multicast(void *self, const Message *msg) {
    const PipeInfo *info = (const PipeInfo *) self;
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
    char *buf = (void *) msg;
    const PipeInfo *info = (const PipeInfo *) self;
    size_t toRead = sizeof(MessageHeader);
    ssize_t wasRead = 0;
    while (wasRead != toRead) {
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
    char *buf = (void *) msg;
    MessageInfo *info = (MessageInfo *) self;
    int toRead = sizeof(MessageHeader);
    local_id pipe = 0;
    ssize_t wasRead = 0;
    do {
        if (++pipe == info->pipeInfo->totalProcessesCount) {
            pipe = 1;
            nanosleep(&sleepTimeout, NULL);
        }
        if (pipe != info->pipeInfo->selfId) {
//            printf("%d: process %d reading from %d (%d)\n", get_lamport_time(), info->pipeInfo->selfId, pipe, info->pipeInfo->pipes[pipe].read);
            wasRead = read(info->pipeInfo->pipes[pipe].read, buf, toRead);
            if (wasRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                error(0, errno, "%d receive_any_1: ", info->pipeInfo->selfId);
                return -1;
            }
//            printf("%d: read %ld of %d, %d\n", info->pipeInfo->selfId, wasRead, toRead, errno);
        }
    } while (wasRead == 0 || wasRead == -1);
//    printf("%d: was read %ld, to read: %d", info->pipeInfo->selfId, wasRead, toRead);
    while (wasRead != toRead) {
        ssize_t res = read(info->pipeInfo->pipes[pipe].read, buf + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            error(0, errno, "%d receive_any_2: ", info->pipeInfo->selfId);
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }
//    printf("%d: read: [", info->pipeInfo->selfId);
//    for (int i = 0; i < sizeof(MessageHeader); ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("%d", buf[i]);
//    }
//    printf("]\n");

    toRead = msg->s_header.s_payload_len;
    wasRead = 0;
//    printf("%d: to read: %ld\n", info->pipeInfo->selfId, toRead);
    while (wasRead != toRead) {
        ssize_t res = read(info->pipeInfo->pipes[pipe].read, msg->s_payload + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            error(0, errno, "%d receive_any_3: ", info->pipeInfo->selfId);
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }

//    printf("%d: read: [", info->pipeInfo->selfId);
//    for (int i = 0; i < sizeof(MessageHeader) + msg->s_header.s_payload_len; ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("%d", buf[i]);
//    }
//    printf("]\n");

    info->sender = pipe;
    update_lamport_time(msg->s_header.s_local_time);

    return 0;
}
