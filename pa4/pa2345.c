#include <stdlib.h>
#include <memory.h>
#include "pa2345.h"
#include "pipe.h"
#include "lamport.h"

extern int doneProcesses;

typedef struct {
    local_id processId;
    timestamp_t timestamp;
} CSRequestInfo;

CSRequestInfo requestsQueue[MAX_PROCESS_ID] = { {0} };

int requestsCount = 0;

void enqueueRequest(CSRequestInfo request) {
    if (requestsCount == MAX_PROCESS_ID) {
        exit(-8);
    }
//    printf("%d: request: (%d, %d)\n", selfId, request.processId, request.timestamp);

    requestsQueue[requestsCount].processId = request.processId;
    requestsQueue[requestsCount++].timestamp = request.timestamp;
//    for (int i = 0; i < MAX_PROCESS_ID; ++i) {
//        if (i != 0) {
//            printf(", ");
//        }
//        printf("(%d, %d)", requestsQueue[i].processId, requestsQueue[i].timestamp);
//    }
//    printf("]\n");
}

void removeRequestFromProcess(local_id processId) {
    if (requestsCount == 0) {
        exit(-9);
    }
    CSRequestInfo buffer[MAX_PROCESS_ID] = { {0} };
    bool found = false;
    for (int i = 0; i < requestsCount; ++i) {
        if (requestsQueue[i].processId == processId) {
            found = true;
            continue;
        }
        buffer[i - found].processId = requestsQueue[i].processId;
        buffer[i - found].timestamp = requestsQueue[i].timestamp;
    }
    if (!found) {
        exit(-10);
    }
    memcpy(requestsQueue, buffer, sizeof(CSRequestInfo) * (--requestsCount));
}

bool requestHasLowestTimestamp(CSRequestInfo request) {
    CSRequestInfo minimumRequest = {INT8_MAX, INT16_MAX};
    for (int i = 0; i < requestsCount; ++i) {
        if (requestsQueue[i].timestamp < minimumRequest.timestamp) {
            minimumRequest.timestamp = requestsQueue[i].timestamp;
            minimumRequest.processId = requestsQueue[i].processId;
        } else if (requestsQueue[i].timestamp == minimumRequest.timestamp &&
                   requestsQueue[i].processId < minimumRequest.processId) {
            minimumRequest.processId = requestsQueue[i].processId;
        }
    }

    return request.processId == minimumRequest.processId && request.timestamp == minimumRequest.timestamp;
}


int request_cs(const void *self) {
    const PipeInfo *info = (PipeInfo *)self;
    CSRequestInfo request = {
            info->selfId,
            increment_lamport_time()
    };
//    printf("%d: request: (%d, %d)\n", info->selfId, request.processId, request.timestamp);

    Message message = {{0}};
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = 0;
    message.s_header.s_type = CS_REQUEST;
    message.s_header.s_local_time = request.timestamp;
    send_multicast((void *) info, &message);

//    printf("%d: queue: [", info->selfId);
    enqueueRequest(request);
    int requiredRepliesCount = info->totalProcessesCount - 2;
    while (requiredRepliesCount > 0 || !requestHasLowestTimestamp(request)) {
//        printf("%d: required: %d, isLowest: %d\n", selfId, requiredRepliesCount, requestHasLowestTimestamp(request));
        MessageInfo receivedInfo = {
                info,
                0
        };
        int res = receive_any(&receivedInfo, &message);
//        printf("%d: process %d received %d from %d\n", get_lamport_time(), info->selfId, message.s_header.s_type, receivedInfo.sender);
        if (res == -1) {
            exit(-2);
        }
        CSRequestInfo incomingRequest;
        switch (message.s_header.s_type) {
            case CS_REPLY:
//                printf("%d: receiving reply (%d, %d)\n", info->selfId, receivedInfo.sender, message.s_header.s_local_time);
                if (message.s_header.s_local_time > request.timestamp) {
                    --requiredRepliesCount;
                }
                break;
            case CS_RELEASE:
                removeRequestFromProcess(receivedInfo.sender);
                break;
            case CS_REQUEST:
                incomingRequest.processId = receivedInfo.sender;
                incomingRequest.timestamp = message.s_header.s_local_time;
//                printf("%d: receiving request (%d, %d)\n", info->selfId, incomingRequest.processId, incomingRequest.timestamp);
//                printf("%d: queue: [", info->selfId);
                enqueueRequest(incomingRequest);
                message.s_header.s_type = CS_REPLY;
                message.s_header.s_local_time = increment_lamport_time();
                send((void *) info, incomingRequest.processId, &message);
                break;
            case DONE:
                ++doneProcesses;
                break;
        }
    }
    return 0;
}

int release_cs(const void *self) {
    const PipeInfo *info = (PipeInfo *)self;
    removeRequestFromProcess(info->selfId);
    Message message = { {0} };
    message.s_header.s_magic = MESSAGE_MAGIC;
    message.s_header.s_payload_len = 0;
    message.s_header.s_type = CS_RELEASE;
    message.s_header.s_local_time = increment_lamport_time();
    send_multicast((void *) info, &message);
    return 0;
}
