#include "libs/xstats.h"
#include "proc_control.h"
#include "proc_cr.h"
#include <libgen.h>
#include <sys/types.h>

// stats
int get_memory_usage();
void fill_json_stats_buffers(void);
void dump_stats(void);
void dump_json_stats_to_file(int argc, char *argv[]);