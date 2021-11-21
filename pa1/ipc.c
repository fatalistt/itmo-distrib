#include "ipc.h"
#include "pipe.h"
#include <unistd.h>

int send_multicast(void *self, const Message *msg)
{
  const PipeInfo *info = (const PipeInfo *)self;
  for (size_t i = 0; i < info->totalProcessesCount; i++)
  {
    if (i == info->selfId)
    {
      continue;
    }

    int toWrite = sizeof(MessageHeader);
    while (toWrite > 0)
    {
      int res = write(info->pipes[i].write, msg, toWrite);
      if (res == -1)
      {
        return -1;
      }
      toWrite -= res;
    }

    toWrite = msg->s_header.s_payload_len;
    while (toWrite > 0)
    {
      int res = write(info->pipes[i].write, msg->s_payload, toWrite);
      if (res == -1)
      {
        return -1;
      }
      toWrite -= res;
    }
  }
  return 0;
}

int receive(void *self, local_id from, Message *msg)
{
  const PipeInfo *info = (const PipeInfo *)self;
  int toRead = sizeof(MessageHeader);
  while (toRead > 0)
  {
    int res = read(info->pipes[from].read, msg, toRead);
    if (res == -1)
    {
      return -1;
    }
    toRead -= res;
  }

  toRead = msg->s_header.s_payload_len;
  while (toRead > 0)
  {
    int res = read(info->pipes[from].read, msg->s_payload, toRead);
    if (res == -1)
    {
      return -1;
    }
    toRead -= res;
  }

  return 0;
}
