#ifndef _MEM_DIFF_
#define _MEM_DIFF_

#include "libs/xstats.h"
#include "structs.h"
#include "proc_cr.h"

enum state {
  // Append at end, do not change order
  IDENTICAL = 0,
  MODIFIED,
  MODIFIED_PROTECTION,
  ADDED_AND_REMOVED,
  ADDED,
  REMOVED,
  DONE
};
static inline int map_regions_cmp(map_t r1,
                                  map_t r2); //  __attribute__((always_inline));

static inline int
page_entries_cmp(page_t p1, page_t p2); // __attribute__((always_inline));

int diff_process_maps_iterator(uint64_t *index_initial_state,
                               uint64_t *index_final_state,
                               process_mem_info_t *initial_state,
                               process_mem_info_t *final_state);

int diff_memory_verify_integrity(trackedprocess_t *tracked_proc);

#endif
