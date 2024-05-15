/**
 * File              : xstats.c
 * Author            : Mohamed Alzayat <alzayat@mpi-sws.org>
 * Date              : 26.02.2020
 * Last Modified Date: 01.12.2020
 * Last Modified By  : Mohamed Alzayat <alzayat@mpi-sws.org>
 * V1 Author         : Aastha Mehta <aasthakm@mpi-sws.org>
 */
#include "xstats.h"
#include "../config.h"
#include "jWrite.h"
#include "xdebug.h"

char json_buffer[16384];
unsigned int json_buflen = 16384;
int json_err;

int init_xstats(xstat *stat, char *name, uint64_t start, uint64_t end,
                uint64_t step) {
  int size;
  memcpy(stat->name, name, strlen(name));
  stat->start = start;
  stat->end = end;
  stat->step = MAX(step, 1);

  stat->count = 0;
  stat->min = 99999999999;
  stat->max = 0;
  size = (end - start) / stat->step + 1;
  stat->dist = calloc(size, sizeof(uint64_t));

  if (!stat->dist)
    return -ENOMEM;

  return 0;
}

int init_xstats_arr(xstat *stat, char *name, uint64_t start, uint64_t end,
                    uint64_t step, ...) {
  int size;
  va_list args;

  memcpy(stat->name, name, strlen(name));
  // va_start(args, name);
  // vsnprintf(stat->name, sizeof(stat->name), name, args);
  // va_end(args);
  stat->start = start;
  stat->end = end;
  stat->step = MAX(step, 1);
  ;

  stat->count = 0;
  stat->min = 99999999999;
  stat->max = 0;

  size = (end - start) / stat->step + 1;
  stat->dist = calloc(size, sizeof(uint64_t));

  if (!stat->dist)
    return -ENOMEM;

  return 0;
}

void add_point(xstat *stat, double value) {
  int index;
  double delta = 0;
  if (value >= stat->max)
    stat->max = value;

  if (value < stat->min)
    stat->min = value;

  index = (value - stat->start) / stat->step;

  if (value >= stat->end) {
    // append to the last bucket
    index = ((stat->end - stat->start) / stat->step) - 1;
  } else if (value < stat->start) {
    // append to the first bucket
    index = 0;
  }

  stat->dist[index]++;
  stat->count++;
  stat->sum += value;
  delta = value - stat->mean;
  stat->mean += delta / stat->count;
  stat->m2 += delta * (value - stat->mean);
}

void add_custom_point(xstat *stat, int index, uint64_t value) {
  int max_index = (stat->end - stat->start) / stat->step;
  int delta = 0;
  if (value >= stat->max)
    stat->max = value;

  if (value < stat->min)
    stat->min = value;

  if (index < 0)
    index = stat->start;
  else if (index > max_index)
    index = max_index;

#if 0
	index = (value - stat->start) / stat->step;

	if (value >= stat->end) {
		//append to the last bucket
		index = ((stat->end - stat->start) / stat->step) - 1;
	} else if (value < stat->start) {
		//append to the first bucket
		index = 0;
	}
#endif

  stat->dist[index]++;
  stat->count++;
  stat->sum += value;
  delta = value - stat->mean;
  stat->mean += delta / stat->count;
  stat->m2 += delta * (value - stat->mean);
}

void print_arr_distribution(xstat *stat, int index) {
  sprintf(stat->name + strlen(stat->name), "_%d", index);
  print_distribution(stat);
}

void begin_json_stats() {
  jwOpen(json_buffer, json_buflen, JW_OBJECT, JW_PRETTY);
  jwObj_array("xstats");
}

void end_json_stats(void) {
  jwEnd();
  int err = jwClose();
  if (err)
    jwErrorToString(err);
}

void print_json_stats(void) {
  fprintf(stderr, "%s\n", json_buffer);
  fflush(stderr);
}

void write_json_stats(FILE *fp) { fprintf(fp, "%s", json_buffer); }

void prep_stats_json(xstat *stat) {
  jwArr_object();
  jwObj_string("name", stat->name);
  jwObj_double("sum", stat->sum);
  jwObj_double("count", stat->count);
  jwObj_double("mean", stat->mean);
  jwObj_double("min", stat->min);
  jwObj_double("max", stat->max);
  if (stat->count >= 2) {
    stat->std_dev = sqrt(stat->m2 / (stat->count - 1));
  }
  jwObj_double("stddev", stat->std_dev);
  jwEnd();
}

void print_distribution(xstat *stat) {

  int i;
  uint64_t bucket;
  int size = (stat->end - stat->start) / stat->step;

  // double avg = 0;
  uint64_t avg_div = 0;
  // uint64_t avg_mod = 0;
  if (stat->count != 0) {
    avg_div = stat->sum;
    // avg_mod = avg_div % stat->count;
    avg_div = (stat->sum / (uint64_t)stat->count);
  }
  if (stat->count >= 2) {
    stat->std_dev = sqrt(stat->m2 / (stat->count - 1));
  }
  jprint(LVL_PAPER, "%s", "");
  jprint(LVL_PAPER, "%s", "-----------------------------------");
  jprint(LVL_PAPER, "%s", stat->name);

  if (stat->count == 0)
    return;

  if (stat->is_time) {
    jprint(LVL_PAPER, "%s Total sum: %lu (%.3f us)", stat->name, stat->sum,
           stat->sum / 1e3);
  } else {
    jprint(LVL_PAPER, "%s Total sum: %lu ", stat->name, stat->sum);
  }

  jprint(LVL_PAPER, "%s Total count: %ld", stat->name, stat->count);

  if (stat->count > 0) {
    if (stat->is_time) {
      jprint(LVL_PAPER, "%s avg: %lu (%.3f us) [%lu, %lu, std= %.2f]",
             stat->name, avg_div, avg_div / 1e3, stat->min, stat->max,
             stat->std_dev);
    } else {
      jprint(LVL_PAPER, "%s avg: %lu [%lu, %lu, std= %.2f]", stat->name,
             avg_div, stat->min, stat->max, stat->std_dev);
    }
    // jprint(LVL_PAPER, "%s avg: %lu %lu [%lu, %lu]", stat->name, avg_div,
    //     avg_mod, stat->min, stat->max);

    for (i = 0; i < size; i++) {
      if (stat->dist[i] == 0)
        continue;

      bucket = stat->start + i * stat->step;
      jprint(LVL_PAPER, "%-*lu\t%-lu", 7, bucket, stat->dist[i]);
    }

    prep_stats_json(stat);
  }
}

void serialize_distribution(xstat *stat, char *ser, int len) {
  char *pivot;
  int written = 0;
  int size;
  int i;

  if (stat == NULL) {
    ((int *)ser)[0] = (-1);
    return;
  }

  // reserve for index
  written += sizeof(int);

  pivot = ser + written;
  memcpy(pivot, stat, sizeof(xstat));
  written += sizeof(xstat);

  pivot = ser + written;
  size = (stat->end - stat->start) / stat->step;
  for (i = 0; i < size; i++) {
    if (written + sizeof(unsigned int) > len)
      break;

    ((unsigned int *)pivot)[i] = stat->dist[i];
  }

  ((int *)ser)[0] = i;
}

void free_xstats(xstat *stat) { free(stat->dist); }
