#ifndef _PROC_MEM_
#define _PROC_MEM_

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <sys/mman.h>
#include <sys/ptrace.h> /* for ptrace() */
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "libs/constants.h"
#include "libs/thpool.h"
#include "libs/xdebug.h"
#include "libs/xstats.h"

#include "config.h"
#include "structs.h"
#include "mem_diff.h"
#include "mem_helpers.h"
#include "print_helpers.h"
#include "proc_control.h"
#include "proc_parsers.h"
#include "stat_helper.h"
#include "syscall_injector.h"

typedef struct config_t {
  int pid;
  int dump;
  int compare;
} config_t;

// API

void clear_soft_dirty_bits(trackedprocess_t *tracked_proc);
// checkpointing = capture + save writeable pages + clear soft dirty bits

process_mem_info_t *capture_process_state(trackedprocess_t *tracked_proc,
                                          bool capture_pages);
process_mem_diff_t *diff_process_states(process_mem_info_t *initial_state,
                                        process_mem_info_t *final_state,
                                        bool diff_pages);

int restore_process_sd(trackedprocess_t *tracked_proc);
process_mem_info_t *checkpoint_process_sd(trackedprocess_t *tracked_proc);

int diff(trackedprocess_t *tracked_proc);
int restore(trackedprocess_t *tracked_proc);
int checkpoint(trackedprocess_t *tracked_proc);

void dump_process_mem_info_t_to_file(process_mem_info_t *ps, char *fname);
process_mem_info_t *read_process_mem_info_t_from_file(char *fname);

int dump_process_state_readable(trackedprocess_t *tracked_proc, char *fname);
// Functionality

#endif
