/**
 * File              : xdebug.h
 * Author            : Mohamed Alzayat <alzayat@mpi-sws.org>
 * Date              : 26.02.2020
 * Last Modified Date: 01.12.2020
 * Last Modified By  : Mohamed Alzayat <alzayat@mpi-sws.org>
 * V1 Author         : Aastha Mehta <aasthakm@mpi-sws.org>
 */

#ifndef __XDEBUG_H__
#define __XDEBUG_H__

#include <linux/types.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "../config.h"
#include <execinfo.h>

#define PFX ""

#define LVL_DBG 0
#define LVL_INFO 1
#define LVL_ERR 2
#define LVL_TEST 3
#define LVL_EXP 4
#define LVL_PAPER 5

int sched_getcpu(void);

#define jprint(LVL, F, ...)                                                    \
  do {                                                                         \
    if (XDEBUG_LVL <= LVL) {                                                   \
      fprintf(stderr, F "\n", ##__VA_ARGS__);                                  \
      fflush(stderr);                                                          \
    }                                                                          \
  } while (0)

#define iprintl(LVL, F, ...)                                                   \
  do {                                                                         \
    if (XDEBUG_LVL <= LVL) {                                                   \
      fprintf(stderr, PFX " (%d, core:%d) %s:%d " F "\n", getpid(),            \
              sched_getcpu(), __func__, __LINE__, ##__VA_ARGS__);              \
      fflush(stderr);                                                          \
    }                                                                          \
  } while (0)

#define iprint(LVL, F, ...)                                                    \
  do {                                                                         \
    if (XDEBUG_LVL <= LVL) {                                                   \
      fprintf(stderr, PFX " (%d, core:%d) " F "\n", getpid(), sched_getcpu(),  \
              ##__VA_ARGS__);                                                  \
      fflush(stderr);                                                          \
    }                                                                          \
  } while (0)

static void check(int test, const char *message, ...) {
  if (test) {
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
  }
}

static void print_trace(void) {
  void *array[10];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace(array, 10);
  strings = backtrace_symbols(array, size);

  printf("Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++)
    printf("%s\n", strings[i]);

  free(strings);
}

#endif /* __XDEBUG_H__ */
