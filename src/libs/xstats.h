/**
 * File              : xstats.h
 * Author            : Mohamed Alzayat <alzayat@mpi-sws.org>
 * Date              : 26.02.2020
 * Last Modified Date: 13.11.2020
 * Last Modified By  : Mohamed Alzayat <alzayat@mpi-sws.org>
 * V1 Author         : Aastha Mehta <aasthakm@mpi-sws.org>
 */

#ifndef __XSTATS_H__
#define __XSTATS_H__

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#include "xtime.h"
#include <stdbool.h>

typedef struct xstat {
  char name[64]; // variable name

  uint64_t start;
  uint64_t end;
  uint64_t step;

  uint64_t count;
  uint64_t sum;

  uint64_t min;
  uint64_t max;

  double mean;
  double std_dev;
  double m2;

  bool is_time;

  uint64_t *dist; // the distribution

} xstat;

int init_xstats(xstat *stat, char *name, uint64_t start, uint64_t end,
                uint64_t step);

int init_xstats_arr(xstat *stat, char *name, uint64_t start, uint64_t end,
                    uint64_t step, ...);

// take a value in ns
void add_point(xstat *stat, double value);

void add_custom_point(xstat *stat, int index, uint64_t value);

void print_distribution(xstat *stat);

void print_arr_distribution(xstat *stat, int index);

void serialize_distribution(xstat *stat, char *ser, int len);

void begin_json_stats();

void end_json_stats();

void write_json_stats(FILE *fp);

void print_json_stats();

void prep_stats_json(xstat *stat);

void free_xstats(xstat *stat);

#if XSTATS_CONFIG_STAT == 1

#define XSTATS_DECL_STAT_EXTERN(name) extern xstat *stats_##name
#define XSTATS_DECL_STAT(name) xstat *stats_##name = NULL
#define XSTATS_INIT_STAT(name, desc, sta, end, st)                             \
  stats_##name = calloc(1, sizeof(xstat));                                     \
  if (!stats_##name) {                                                         \
    return;                                                                    \
  }                                                                            \
  if (init_xstats(stats_##name, desc, sta, end, st))                           \
  return

#define XSTATS_DEST_STAT(name, toprint)                                        \
  if (stats_##name != NULL) {                                                  \
    if (toprint) {                                                             \
      print_distribution(stats_##name);                                        \
    }                                                                          \
    free_xstats(stats_##name);                                                 \
    free(stats_##name);                                                        \
  }

#define XSTATS_PRINT_STAT(name)                                                \
  if (stats_##name != NULL) {                                                  \
    print_distribution(stats_##name);                                          \
  }
#define XSTATS_INIT_VARS(name)                                                 \
  uint64_t start_##name;                                                       \
  uint64_t end_##name;

#define XSTATS_INIT_TIMER(name)                                                \
  uint64_t start_##name;                                                       \
  uint64_t end_##name;                                                         \
  stats_##name->is_time = 1

#define XSTATS_INIT_TIMER_ARR(name, N)                                         \
  uint64_t start_##name;                                                       \
  uint64_t end_##name;                                                         \
  for (int stati = 0; stati < N; stati++) {                                    \
    stats_##name[stati]->is_time = 1;                                          \
  }

#define XSTATS_START_TIMER(name) start_##name = NOW()
#define XSTATS_START_TIME(name) start_##name
#define XSTATS_END_TIME(name) end_##name
#define XSTATS_SPENT_TIME(name)                                                \
  (XSTATS_END_TIME(name) - XSTATS_START_TIME(name))

#define XSTATS_ADD_TIME_POINT(name)                                            \
  (add_point(stats_##name, XSTATS_SPENT_TIME(name)))

#define XSTATS_ADD_TIME_POINT_AMORT(name, count)                               \
  (add_point(stats_##name, XSTATS_SPENT_TIME(name) / count))

#define XSTATS_END_TIMER(name)                                                 \
  end_##name = NOW();                                                          \
  XSTATS_ADD_TIME_POINT(name)

#define XSTATS_END_TIMER_AMORT(name, count)                                    \
  end_##name = NOW();                                                          \
  XSTATS_ADD_TIME_POINT_AMORT(name, count)

#define XSTATS_END_TIMER_DO_NOT_ADD_POINT(name) end_##name = NOW();

#define XSTATS_ADD_COUNT_POINT(name, val) (add_point(stats_##name, val))

#define XSTATS_ADD_COUNT_POINT_AMORT(name, val, count)                         \
  (add_point(stats_##name, val / count))

#define XSTATS_ADD_CUSTOM_POINT(name, index, val)                              \
  (add_custom_point(stats_##name, index, val))

#define XSTATS_DECL_STAT_ARR(name, N)                                          \
  xstat *stats_##name[N] //	__cacheline_aligned_in_smp

#define XSTATS_DECL_STAT_ARR_EXTERN(name, N) extern xstat *stats_##name[N]

#define XSTATS_INIT_STAT_ARR(name, desc, sta, end, st, N, arg...)              \
  do {                                                                         \
    int stati;                                                                 \
    for (stati = 0; stati < N; stati++) {                                      \
      stats_##name[stati] = calloc(1, sizeof(xstat));                          \
      if (init_xstats_arr(stats_##name[stati], desc, sta, end, st, stati)) {   \
        printf("BUG: cannot init stat %s \n", desc);                           \
        return;                                                                \
      }                                                                        \
    }                                                                          \
  } while (0)

#define XSTATS_DEST_STAT_ARR(name, toprint, N)                                 \
  do {                                                                         \
    int stati;                                                                 \
    for (stati = 0; stati < N; stati++) {                                      \
      if (stats_##name[stati] != NULL) {                                       \
        if (toprint) {                                                         \
          print_arr_distribution(stats_##name[stati], stati);                  \
        }                                                                      \
        free_xstats(stats_##name[stati]);                                      \
        free(stats_##name[stati]);                                             \
      }                                                                        \
    }                                                                          \
  } while (0)

#define XSTATS_ADD_TIME_POINT_ARR(name, stati)                                 \
  (add_point(stats_##name[stati], XSTATS_SPENT_TIME(name)))

#define XSTATS_ADD_TIME_POINT_ARR_AMORT(name, count, stati)                    \
  (add_point(stats_##name[stati], XSTATS_SPENT_TIME(name) / count))

#if 0
#define XSTATS_END_TIMER_ARR(name, stati)                                      \
  getnstimeofday(&end_##name);                                                 \
  XSTATS_ADD_TIME_POINT_ARR(name, stati)
#endif

#define XSTATS_END_TIMER_ARR(name, stati) end_##name = NOW();

#define XSTATS_END_TIMER_ARR_AMORT(name, count, stati)                         \
  end_##name = NOW();                                                          \
  XSTATS_ADD_TIME_POINT_ARR_AMORT(name, count, stati)

#define XSTATS_ADD_COUNT_POINT_ARR(name, val, stati)                           \
  (add_point(stats_##name[stati], val))

#else /* XSTATS_CONFIG_STAT == 0 */
#define XSTATS_DECL_STAT_EXTERN(name)
#define XSTATS_DECL_STAT(name)
#define XSTATS_INIT_STAT(name, desc, sta, end, st) n
#define XSTATS_DEST_STAT(name, toprint)
#define XSTATS_PRINT_STAT(name)
#define XSTATS_INIT_VARS(name)
#define XSTATS_INIT_TIMER(name)
#define XSTATS_INIT_TIMER_ARR(name, N)
#define XSTATS_START_TIMER(name)
#define XSTATS_START_TIME(name)
#define XSTATS_END_TIME(name)
#define XSTATS_SPENT_TIME(name)
#define XSTATS_ADD_TIME_POINT(name)
#define XSTATS_END_TIMER(name)
#define XSTATS_END_TIMER_AMORT(name, count)
#define XSTATS_END_TIMER_DO_NOT_ADD_POINT(name)
#define XSTATS_ADD_COUNT_POINT(name, val)
#define XSTATS_ADD_COUNT_POINT_AMORT(name, val, count)
#define XSTATS_ADD_CUSTOM_POINT(name, index, val)

#define XSTATS_DECL_STAT_ARR_EXTERN(name, N)
#define XSTATS_DECL_STAT_ARR(name, N)
#define XSTATS_INIT_STAT_ARR(name, desc, sta, end, st, N, arg...)
#define XSTATS_DEST_STAT_ARR(name, toprint, N)
#define XSTATS_ADD_TIME_POINT_ARR(name, stati)
#define XSTATS_ADD_TIME_POINT_ARR_AMORT(name, count, stati)
#define XSTATS_END_TIMER_ARR(name, stati)
#define XSTATS_END_TIMER_ARR_AMORT(name, count, stati)
#define XSTATS_ADD_COUNT_POINT_ARR(name, val, stati)
#endif

#endif /* __XSTATS_H__ */
