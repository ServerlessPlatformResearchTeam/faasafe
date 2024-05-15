#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// LVL_DBG independently dumps both Groundhog's view of the soft-dirty bits as
// well as
// the view from another program. To allow an external program access the /proc
// file system the ptrace_scope must be more permissive than the default (1). It
// should be set to 0 $ echo 0|sudo tee /proc/sys/kernel/yama/ptrace_scope
// XDEBUG_LVL: LVL_EXP LVL_INFO LVL_TEST LVL_DBG
#define XDEBUG_LVL LVL_EXP
#define XSTATS_CONFIG_STAT 1
#define CONCURRENCY 1
#define PRINT_JSON_STATS 1
#define GH_NOP 0
// To enable resetting the time to the snapshotted time
// Clone the libfaketime repo (https://github.com/wolfcw/libfaketime.git) and
// Update the compilation flags in libfaketime/src/Makefile by adding the following two
// flags at the end:  -DFORCE_MONOTONIC_FIX -DFAKE_SETTIME
// build it (make) copy libfaketime/src/libfaketime.so.1 to the /bin/build/
// directory set RESTORE_TIME to 1
#define RESTORE_TIME 0
