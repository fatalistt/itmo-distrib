#include "args.h"
#include "ipc.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

Arguments parseArgs(int argc, char *const *argv)
{
  Arguments args;

  int opt;
  do
  {
    opt = getopt(argc, argv, "p:");
    switch (opt)
    {
    case 'p':
      args.p = atoi(optarg);
      break;
    case '?':
      break;
    case ':':
      break;
    }
  } while (opt != -1);

  return args;
}

void printExampleUsage(FILE *__restrict stream)
{
  fprintf(stream, "Example usage:\nlab1 -p 10\n");
}

void validateArgs(const Arguments *args)
{
  if (args->p < 1 || args->p > MAX_PROCESS_ID)
  {
    fprintf(stderr, "p constraint: 0 < p < %d\n", MAX_PROCESS_ID + 1);
    printExampleUsage(stderr);
    exit(1);
  }
}
