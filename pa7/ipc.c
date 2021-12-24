#define _POSIX_C_SOURCE 199309L
#include "ipc.h"
#include "pipe.h"
#include "lamport.h"
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <error.h>

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
//        printf("%d: %d to %d (%d): write header to %d, %ld, %ld\n", get_lamport_time()[info->selfId], info->selfId, dst, msg->s_header.s_type, info->pipes[dst].write, buf + written, toWrite - written);
        ssize_t res = write(info->pipes[dst].write, buf + written, toWrite - written);
        int errn = errno;
        if (res == -1) {
            error(0, errn, "%d: send", info->selfId);
            return -1;
        }
        written += res;
//        printf("%d: %d to %d (%d): written header %ld of %ld: [", get_lamport_time()[info->selfId], info->selfId, dst, msg->s_header.s_type, written, toWrite);
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
    while (written != toWrite) {
        ssize_t res = write(info->pipes[dst].write, msg->s_payload + written, toWrite - written);
        if (res == -1) {
            return -1;
        }
        written += res;
//        printf("%d: %d to %d (%d): written payload %ld of %ld: [", get_lamport_time()[info->selfId], info->selfId, dst, msg->s_header.s_type, written, toWrite);
//        for (int i = 0; i < res; ++i) {
//            if (i != 0) {
//                printf(", ");
//            }
//            printf("%d", *(msg->s_payload + written - res + i));
//        }
//        printf("]\n");
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
//    printf("%d: process %d reading from %d (%d)\n", get_lamport_time()[info->selfId], info->selfId, from, info->pipes[from].read);
    while(wasRead != toRead) {
        ssize_t res = read(info->pipes[from].read, buf + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            error(0, errno, "%d: reading header", get_lamport_time()[info->selfId]);
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }

//    printf("%d: process %d read header: [", get_lamport_time()[info->selfId], info->selfId);
//    for (int i = 0; i < sizeof(MessageHeader); ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("%d", buf[i]);
//    }
//    printf("]\n");

    toRead = msg->s_header.s_payload_len;
    wasRead = 0;
//    printf("%d: process %d reading payload of len: %ld\n", get_lamport_time()[info->selfId], info->selfId, toRead);
    while (wasRead != toRead) {
        ssize_t res = read(info->pipes[from].read, msg->s_payload + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            error(0, errno, "%d: reading payload", get_lamport_time()[info->selfId]);
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }

//    printf("%d: process %d read payload: [", get_lamport_time()[info->selfId], info->selfId);
//    for (int i = 0; i < toRead; ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("%d", msg->s_payload[i]);
//    }
//    printf("]\n");

    update_lamport_time(msg->s_header.s_local_timevector);

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
//            printf("%d: process %d reading from %d (%d)\n", get_lamport_time()[info->selfId], info->selfId, pipe, info->pipes[pipe].read);
            wasRead = read(info->pipes[pipe].read, buf, toRead);
            if (wasRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                return -1;
            }
        }
    } while (wasRead == 0 || wasRead == -1);
    while (wasRead != toRead) {
        ssize_t res = read(info->pipes[pipe].read, buf + wasRead, toRead - wasRead);
        if (res == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            error(0, errno, "%d receive_any_2: ", info->selfId);
            return -1;
        } else if (res > 0) {
            wasRead += res;
        } else {
            nanosleep(&sleepTimeout, NULL);
        }
    }
//    printf("%d: read: [", info->selfId);
//    for (int i = 0; i < sizeof(MessageHeader); ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("%d", buf[i]);
//    }
//    printf("]\n");

    toRead = msg->s_header.s_payload_len;
    wasRead = 0;
//    printf("%d: process %d to read: %d\n", get_lamport_time()[info->selfId], info->selfId, toRead);
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

//    printf("%d: read: [", info->selfId);
//    for (int i = 0; i < sizeof(MessageHeader) + msg->s_header.s_payload_len; ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("%d", buf[i]);
//    }
//    printf("]\n");

//    timestamp_t *times = get_lamport_time();
//    printf("%d: times of %d: [", times[info->selfId], info->selfId);
//    for (int i = 0; i <= MAX_PROCESS_ID; ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("%d", times[i]);
//    }
//    printf("]\n");

    update_lamport_time(msg->s_header.s_local_timevector);

//    times = get_lamport_time();
//    printf("%d: times of %d: [", times[info->selfId], info->selfId);
//    for (int i = 0; i <= MAX_PROCESS_ID; ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("%d", times[i]);
//    }
//    printf("]\n");

//    printf("%d: %d read ending\n", get_lamport_time()[info->selfId], info->selfId);

    return 0;
}
