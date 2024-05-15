#include "mem_diff.h"

// return 1 if identical 0 otherwise
static inline int map_regions_cmp(map_t r1, map_t r2) {
  // return memcmp(&r1, &r2, sizeof(map_t) - NAME_SIZE - 5*sizeof(uint64_t));
  return !((r1.addr_start == r2.addr_start) && (r1.addr_end == r2.addr_end) &&
           (r1.r == r2.r && r1.w == r2.w) && (r1.x == r2.x && r1.p == r2.p) &&
           (r1.offset == r2.offset) && (r1.dev_major == r2.dev_major) &&
           (r1.dev_minor == r2.dev_minor) && (r1.inode == r2.inode) &&
           (r1.total_num_pages == r2.total_num_pages) &&
           (r1.dirty == r2.dirty) && (r1.dirty == r2.dirty) &&
           (r1.soft_dirty_count == r2.soft_dirty_count) &&
           (r1.freed_count == r2.freed_count) &&
           (!strncmp(r1.name, r2.name, NAME_SIZE)));
}

static inline int page_entries_cmp(page_t p1, page_t p2) {
  return memcmp(&p1, &p2, 2 * sizeof(uint64_t));
}

// relies on changes in the metadata of the process maps
int diff_process_maps_iterator(uint64_t *index_initial_state,
                               uint64_t *index_final_state,
                               process_mem_info_t *initial_state,
                               process_mem_info_t *final_state) {

  if (*index_initial_state == initial_state->maps_count &&
      *index_final_state == final_state->maps_count) {
    return DONE;
  }

  if (*index_initial_state >= initial_state->maps_count) {
    *index_final_state += 1;
    return ADDED;
  }

  if (*index_final_state >= final_state->maps_count) {
    *index_initial_state += 1;
    return REMOVED;
  }
  if (map_regions_cmp(initial_state->maps[*index_initial_state],
                      final_state->maps[*index_final_state]) == 0) {
    *index_initial_state += 1;
    *index_final_state += 1;
    return IDENTICAL;
  }

  if (initial_state->maps[*index_initial_state].addr_start ==
          final_state->maps[*index_final_state].addr_start &&
      initial_state->maps[*index_initial_state].addr_end ==
          final_state->maps[*index_final_state].addr_end) {

    *index_initial_state += 1;
    *index_final_state += 1;

    if (initial_state->maps[*index_initial_state - 1].r !=
            final_state->maps[*index_final_state - 1].r ||
        initial_state->maps[*index_initial_state - 1].w !=
            final_state->maps[*index_final_state - 1].w ||
        initial_state->maps[*index_initial_state - 1].x !=
            final_state->maps[*index_final_state - 1].x)
      return MODIFIED_PROTECTION;
    return MODIFIED;
  }

  if (initial_state->maps[*index_initial_state].addr_start ==
          final_state->maps[*index_final_state].addr_start ||
      initial_state->maps[*index_initial_state].addr_end ==
          final_state->maps[*index_final_state].addr_end) {

    *index_initial_state += 1;
    *index_final_state += 1;
    // the map is shrunk or grown
    return ADDED_AND_REMOVED;
  }

  if (initial_state->maps[*index_initial_state].addr_start <
      final_state->maps[*index_final_state].addr_start) {

    *index_initial_state += 1;
    return REMOVED;
  } else {
    *index_final_state += 1;
    return ADDED;
  }
}

int diff_process_pages_iterator(uint64_t *index_initial_state,
                                uint64_t *index_final_state,
                                process_mem_info_t *initial_state,
                                process_mem_info_t *final_state) {

  if (*index_initial_state == initial_state->pages_count &&
      *index_final_state == final_state->pages_count) {
    return DONE;
  }

  if (*index_initial_state >= initial_state->pages_count) {
    *index_final_state += 1;
    return ADDED;
  }

  if (*index_final_state >= final_state->pages_count) {
    *index_initial_state += 1;
    return REMOVED;
  }
  if (page_entries_cmp(initial_state->pages[*index_initial_state],
                       final_state->pages[*index_final_state]) == 0) {
    *index_initial_state += 1;
    *index_final_state += 1;
    return IDENTICAL;
  }
}

int diff_memory_verify_integrity(trackedprocess_t *tracked_proc) {
  // we need to clear the soft dirty bits before comparing, since restoration is
  // done!
  clear_soft_dirty_bits(tracked_proc);
  process_mem_info_t *orig_state = tracked_proc->p_checkpointed_state;
  process_mem_info_t *tmp_state =
      capture_process_state(tracked_proc, WITH_PAGES);
  process_mem_diff_t *p_diff_tmp = diff_process_states(
      tracked_proc->p_checkpointed_state, tmp_state, WITH_PAGES);
  print_diff_stats(p_diff_tmp);
  for (uint64_t i = 0; i < tmp_state->maps_count; i++) {
    if (tmp_state->maps[i].soft_dirty_count)
      print_maps_index(tmp_state, i);
    // assert(tmp_state->maps[i].soft_dirty_count == 0);
  }

  jprint(LVL_EXP, "Asserting that memory pages are identical");
  fflush(stderr);

  bool store_wpages_only = 0;
  uint64_t num_pages_to_store = (store_wpages_only)
                                    ? tmp_state->writable_pages_count
                                    : tmp_state->pages_count;
  init_memory_copy_info(&(tmp_state->wmem), num_pages_to_store);
  populate_memory_copy_info(tracked_proc->mem_fd, tmp_state, tracked_proc,
                            store_wpages_only);
  int count = 0;

  assert(tracked_proc->p_checkpointed_state->pages_count ==
         tmp_state->pages_count);
  /*
          print_sd_bits_externally();
  */

  if (memcmp(tracked_proc->p_checkpointed_state->wmem, tmp_state->wmem,
             tracked_proc->p_checkpointed_state->paged_pages_count * PSIZE) !=
      0) {
    jprint(LVL_EXP, "Memory pages are not identical!!!!");

    for (uint64_t i = 0; i < orig_state->maps_count; i++) {
      map_t stored_map = orig_state->maps[i];
      if (map_regions_cmp(stored_map, tmp_state->maps[i]) != 0) {
        jprint(LVL_EXP, "MAPS not identical!!!!");
        print_maps_index(orig_state, i);
        print_maps_index(tmp_state, i);
      }

      // if (!tmp_state->maps[i].w)//||
      // !tracked_proc->p_checkpointed_state->maps[i].w)
      //	continue;

      uint64_t wmem_index = orig_state->maps[i].first_wmem_index * PSIZE;
      uint64_t unmatching_pages_count = 0;

      if (memcmp(tracked_proc->p_checkpointed_state->wmem + wmem_index,
                 tmp_state->wmem + wmem_index,
                 orig_state->maps[i].total_num_pages * PSIZE) != 0) {
        for (int p = 0; p < orig_state->maps[i].total_num_paged_pages; p++) {
          if (memcmp(tracked_proc->p_checkpointed_state->wmem + wmem_index +
                         p * PSIZE,
                     tmp_state->wmem + wmem_index + p * PSIZE, PSIZE) != 0) {
            unmatching_pages_count += 1;

            if (unmatching_pages_count == 1) {
              jprint(LVL_EXP, "Memory not identical!!!!");

              int r = 0;
              int new_r = 0;
              char *mem_buf =
                  malloc(orig_state->maps[i].total_num_paged_pages * PSIZE);
              do {
                new_r =
                    pread(tracked_proc->mem_fd, mem_buf + r,
                          orig_state->maps[i].total_num_paged_pages * PSIZE - r,
                          orig_state->maps[i].addr_start + r);
                r += new_r;
              } while (r != orig_state->maps[i].total_num_paged_pages * PSIZE &&
                       new_r > 0);

              if (memcmp(tracked_proc->p_checkpointed_state->wmem + wmem_index,
                         mem_buf,
                         orig_state->maps[i].total_num_paged_pages * PSIZE) !=
                  0) {
                jprint(LVL_EXP,
                       "Original Memory (%lu pages) starting at %p not "
                       "identical!!!!",
                       orig_state->maps[i].total_num_paged_pages,
                       (void *)orig_state->maps[i].addr_start);
              } else {
                jprint(LVL_EXP,
                       "Original Memory (%lu pages) starting at %p  IS "
                       "identical!!!!",
                       orig_state->maps[i].total_num_paged_pages,
                       (void *)orig_state->maps[i].addr_start);
              }
            }
            if (tmp_state->pages[stored_map.first_page_index + p].addr_start !=
                tracked_proc->p_checkpointed_state
                    ->pages[stored_map.first_page_index + p]
                    .addr_start) {

              jprint(LVL_EXP,
                     "Indexing broken ... tmpstate map addr_start %p, chkstate "
                     "map add_start %p",
                     (void *)tmp_state->pages[stored_map.first_page_index + p]
                         .addr_start,
                     (void *)tracked_proc->p_checkpointed_state
                         ->pages[stored_map.first_page_index + p]
                         .addr_start);
              // assert(0);
            }

            print_pages_index(tracked_proc->p_checkpointed_state,
                              stored_map.first_page_index + p);
            print_pages_index(tmp_state, stored_map.first_page_index + p);
          }
        }
        jprint(LVL_EXP, "Memory not identical!!!!, #pages %ld",
               unmatching_pages_count);
        print_maps_index(tracked_proc->p_checkpointed_state, i);
        print_maps_index(tmp_state, i);
        if (unmatching_pages_count > 0)
          count += 1;
      }
    }
    jprint(LVL_EXP, "Number of unmatching memory maps = %d", count);
    fflush(stdout);
    fflush(stderr);
    assert(0);
  } else {
    jprint(LVL_EXP, "Memory pages are IDENTICAL!");
  }
  free_process_mem_diff(p_diff_tmp);
  free_process_mem_info(tmp_state, 1);
  fflush(stdout);
}
