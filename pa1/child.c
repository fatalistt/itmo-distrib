#include "child.h"
#include "ipc.h"
#include "pa1.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void doChildJob(const PipeInfo *info, int parentPid)
{
  char buffer[MAX_PAYLOAD_LEN + 1];
  int payloadLength = snprintf(buffer, MAX_PAYLOAD_LEN, log_started_fmt, info->selfId, getpid(), parentPid);
  Message message;
  message.s_header.s_magic = MESSAGE_MAGIC;
  message.s_header.s_payload_len = payloadLength;
  message.s_header.s_type = STARTED;
  message.s_header.s_local_time = 0;
  memcpy(&message.s_payload, buffer, MAX_PAYLOAD_LEN);
  fprintf(stdout, log_started_fmt, info->selfId, getpid(), parentPid);
  fprintf(info->eventLog, log_started_fmt, info->selfId, getpid(), parentPid);
  send_multicast((void *)info, &message);
  for (int i = 1; i < info->totalProcessesCount; i++)
  {
    if (i == info->selfId)
    {
      continue;
    }
    memset(&message, 0, sizeof(Message));
    if (receive((void *)info, i, &message) || message.s_header.s_type != STARTED)
    {
      exit(1);
    }
  }

  fprintf(stdout, log_received_all_started_fmt, info->selfId);
  fprintf(info->eventLog, log_received_all_started_fmt, info->selfId);

  memset(buffer, 0, MAX_PAYLOAD_LEN + 1);
  payloadLength = snprintf(buffer, MAX_PAYLOAD_LEN, log_done_fmt, info->selfId);
  message.s_header.s_magic = MESSAGE_MAGIC;
  message.s_header.s_payload_len = payloadLength;
  message.s_header.s_type = DONE;
  message.s_header.s_local_time = 0;
  memcpy(&message.s_payload, buffer, MAX_PAYLOAD_LEN);
  fprintf(stdout, log_done_fmt, info->selfId);
  fprintf(info->eventLog, log_done_fmt, info->selfId);
  send_multicast((void *)info, &message);
  for (int i = 1; i < info->totalProcessesCount; i++)
  {
    if (i == info->selfId)
    {
      continue;
    }
    memset(&message, 0, sizeof(Message));
    if (receive((void *)info, i, &message) || message.s_header.s_type != DONE)
    {
      exit(1);
    }
  }

  fprintf(stdout, log_received_all_done_fmt, info->selfId);
  fprintf(info->eventLog, log_received_all_done_fmt, info->selfId);
}
