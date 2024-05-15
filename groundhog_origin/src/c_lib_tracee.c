/*
 * This is a simple optional library that communicates the checkpoint/restore
 * timings with Groundhog. For FaaS, the communication of checkpoint/restore
 * times can alternatively be inferred by intercepting the functions inputs and
 * outputs.
 *
 */

#include "c_lib_tracee.h"

#if RESTORE_TIME
struct timespec checkpoint_time;
#endif
static char *acks = NULL;
static size_t ackssize = 0;
static FILE *stdcmdin = NULL;
static FILE *stdcmdout = NULL;
static void open_pipes() {
  if (stdcmdin == NULL) {
    // acks = (char d*) malloc(BUFSIZ * sizeof(char));
    // fflush(stdout);
    stdcmdin = fdopen(4, "r");
    stdcmdout = fdopen(5, "w");
  }
}
static void close_pipes() {
  if (stdcmdin != NULL) {
    fclose(stdcmdin);
    fclose(stdcmdout);
    free(acks);
  }
}

int checkpoint_me() {
  open_pipes();
#if RESTORE_TIME
  if (clock_gettime(CLOCK_REALTIME, &checkpoint_time) == -1) {
    perror("clock gettime");
    exit(EXIT_FAILURE);
  }
#endif
  fprintf(stdcmdout, "checkpoint.me\n");
  fflush(stdcmdout);
  getline((char **)&acks, &ackssize, stdcmdin);
#if RESTORE_TIME
  if (clock_settime(CLOCK_REALTIME, &checkpoint_time) == -1) {
    perror("clock settime");
    exit(EXIT_FAILURE);
  }
#endif
  // assert(strncmp(*acks, "NO!\n",3) == 0);
  return 0;
}

int nop_me() {
  fprintf(stdcmdout, "nop.me\n");
  fflush(stdcmdout);
  getline((char **)&acks, &ackssize, stdcmdin);
  return 0;
}

int restore_me() {
  fprintf(stdcmdout, "restore.me\n");
  fflush(stdcmdout);
  getline((char **)&acks, &ackssize, stdcmdin);
  return 0;
}

int dump_stats_me() {
  fprintf(stdcmdout, "dump_stats.me\n");
  fflush(stdcmdout);
  getline((char **)&acks, &ackssize, stdcmdin);
  return 0;
}
