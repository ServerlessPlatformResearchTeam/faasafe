/**
 * File              : xtime.h
 * Author            : Mohamed Alzayat <alzayat@mpi-sws.org>
 * Date              : 26.02.2020
 * Last Modified Date: 11.03.2020
 * Last Modified By  : Mohamed Alzayat <alzayat@mpi-sws.org>
 * V1 Author         : Aastha Mehta <aasthakm@mpi-sws.org>
 */

#ifndef __XSTATS_TIME_H__
#define __XSTATS_TIME_H__

#include "../config.h"
#include <time.h>

enum { SCALE_NS = 0, SCALE_CC, SCALE_US, SCALE_MS };

#if defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void) {
  unsigned hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

static __inline__ unsigned long long rdtscp(void) {
  unsigned hi, lo;
  __asm__ volatile("rdtscp" : "=a"(lo), "=d"(hi));

  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
  ;
}

#endif

static inline uint64_t get_current_time(int scale) {
  struct timespec ts_ns;
  uint64_t curr_ts;
  if (scale == SCALE_CC) {
    curr_ts = rdtscp();
  } else {
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts_ns);
    if (scale == SCALE_NS) {
      curr_ts = ts_ns.tv_sec * 1e9 + ts_ns.tv_nsec;
    } else if (scale == SCALE_US) {
      curr_ts = ts_ns.tv_sec * 1e6 + ts_ns.tv_nsec / 1e3;
    } else {
      curr_ts = ts_ns.tv_sec * 1e9 + ts_ns.tv_nsec / 1e6;
    }
  }
  return (uint64_t)curr_ts;
}

static inline uint64_t NOW() { return (uint64_t)(get_current_time(SCALE_NS)); }

#endif /* __XSTATS_TIME_H__ */
