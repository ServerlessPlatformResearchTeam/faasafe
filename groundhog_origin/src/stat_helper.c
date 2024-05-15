#include "stat_helper.h"

#if XSTATS_CONFIG_STAT
/*groundhog stats*/
XSTATS_DECL_STAT(num_keep_alives);
XSTATS_DECL_STAT(thread_pool);
XSTATS_DECL_STAT(groundhog_time);

XSTATS_DECL_STAT(groundhog_memory_footprint);

/*proc control*/
XSTATS_DECL_STAT(seizing);
XSTATS_DECL_STAT(attaching);
XSTATS_DECL_STAT(detaching);
XSTATS_DECL_STAT(interrupting);
XSTATS_DECL_STAT(num_threads);

/*mem stats*/
XSTATS_DECL_STAT(num_maps);
XSTATS_DECL_STAT(num_pages);
XSTATS_DECL_STAT(num_paged_pages);
XSTATS_DECL_STAT(num_w_pages);
XSTATS_DECL_STAT(num_softdirty_pages);
XSTATS_DECL_STAT(num_softdirty_paged_pages);
XSTATS_DECL_STAT(num_uffd_dirty_pages);
XSTATS_DECL_STAT(num_restored_pages);
XSTATS_DECL_STAT(num_mmaps);
XSTATS_DECL_STAT(num_mprotects);
XSTATS_DECL_STAT(num_munmaps);

XSTATS_DECL_STAT(all_inclusive_iter_time);
XSTATS_DECL_STAT(all_inclusive_checkpoint_time);
XSTATS_DECL_STAT(groundhog_warm_time);

XSTATS_DECL_STAT(read_map);
XSTATS_DECL_STAT(store_mem);
XSTATS_DECL_STAT(read_pages);
XSTATS_DECL_STAT(dump_disk);
XSTATS_DECL_STAT(read_disk);
XSTATS_DECL_STAT(snapshot);
XSTATS_DECL_STAT(anon);
XSTATS_DECL_STAT(diff);
XSTATS_DECL_STAT(restore);
XSTATS_DECL_STAT(syscalls);
XSTATS_DECL_STAT(brks);
XSTATS_DECL_STAT(mmaps);
XSTATS_DECL_STAT(mprotects);
XSTATS_DECL_STAT(munmaps);
XSTATS_DECL_STAT(madvises);
XSTATS_DECL_STAT(restore_mem);
XSTATS_DECL_STAT(clear_soft);

XSTATS_DECL_STAT(registers_store);
XSTATS_DECL_STAT(registers_restore);

#endif

void init_all_stats(void) {
#if XSTATS_CONFIG_STAT
  uint64_t start, end, step, dist_len;

  start = 0;
  end = 1e9;
  dist_len = 20;
  step = (end - start) / dist_len;

  XSTATS_INIT_STAT(num_keep_alives, "number of restores (Keepalive)", start,
                   end, step);
  XSTATS_INIT_STAT(thread_pool, "Time for creating the thread pool (ns)", start,
                   end, step);
  XSTATS_INIT_STAT(groundhog_time, "Time for end-end benchmark (ns)", start, end,
                   step);

  XSTATS_INIT_STAT(groundhog_memory_footprint, "SEC-FaaS memory footprint (KB)",
                   start, end, step);

  XSTATS_INIT_STAT(seizing, "Time for seizing (ns)", start, end, step);
  XSTATS_INIT_STAT(attaching, "Time for attaching (ns)", start, end, step);
  XSTATS_INIT_STAT(detaching, "Time for detaching (ns)", start, end, step);
  XSTATS_INIT_STAT(interrupting, "Time for interrupting (ns)", start, end,
                   step);
  XSTATS_INIT_STAT(num_threads, "number of threads tracked", start, end, step);

  XSTATS_INIT_STAT(num_maps, "Number of mapped regions", start, end, step);
  XSTATS_INIT_STAT(num_pages, "Number of pages", start, end, step);
  XSTATS_INIT_STAT(num_paged_pages, "Number of paged pages", start, end, step);
  XSTATS_INIT_STAT(num_w_pages, "Number of writable pages", 550000, 700000,
                   10000);
  XSTATS_INIT_STAT(num_softdirty_pages, "Number soft-dirty pages", start, end,
                   step);
  XSTATS_INIT_STAT(num_softdirty_paged_pages,
                   "Number of soft-dirty paged pages", start, end, step);
  XSTATS_INIT_STAT(num_uffd_dirty_pages, "Number of uffd dirty pages", start,
                   end, step);
  XSTATS_INIT_STAT(num_restored_pages, "Number of restored pages", 100000,
                   200000, 5000);
  XSTATS_INIT_STAT(num_mmaps, "Number of mmap syscalls", start, end, step);
  XSTATS_INIT_STAT(num_mprotects, "Number of mprotect syscalls", start, end,
                   step);
  XSTATS_INIT_STAT(num_munmaps, "Number of munmap syscalls", start, end, step);

  XSTATS_INIT_STAT(all_inclusive_iter_time,
                   "Sec-FaaS all inclusive restore iteratinon time (ns)", start,
                   end, step);
  XSTATS_INIT_STAT(all_inclusive_checkpoint_time,
                   "Sec-FaaS all inclusive checkpoint iteratinon time (ns)",
                   start, end, step);
  XSTATS_INIT_STAT(groundhog_warm_time,
                   "Time for end-end benchmark after checkpoint (ns)", start,
                   end, step);

  XSTATS_INIT_STAT(read_map, "Time for reading maps (ns)", start, end, step);
  XSTATS_INIT_STAT(store_mem, "Time for saving a copy of writable memory (ns)",
                   start, end, step);
  XSTATS_INIT_STAT(read_pages, "Time for reading pages metadata (ns)", start,
                   end, step);

  XSTATS_INIT_STAT(dump_disk, "Time for dumping process state to disk (ns)",
                   start, end, step);
  XSTATS_INIT_STAT(read_disk, "Time for reading process state from disk (ns)",
                   start, end, step);
  XSTATS_INIT_STAT(snapshot, "Time for snapshotting (ns)", start, end, step);
  XSTATS_INIT_STAT(anon, "Time for anonimizing maps (ns)", start, end, step);
  XSTATS_INIT_STAT(diff, "Time for diffing 2 snapshots (ns)", start, end, step);
  XSTATS_INIT_STAT(restore, "Time for restoring a process (ns)", start, end,
                   step);
  XSTATS_INIT_STAT(syscalls,
                   "Time for restore syscalls: brk+mmaps+munmaps (ns)", start,
                   end, step);
  XSTATS_INIT_STAT(brks, "Time for brk syscall (ns)", start, end, step);
  XSTATS_INIT_STAT(mmaps, "Time for mmaping a region (ns)", start, end, step);
  XSTATS_INIT_STAT(mprotects, "Time for mprotecting a region (ns)", start, end,
                   step);
  XSTATS_INIT_STAT(munmaps, "Time for unmmaping a region (ns)", start, end,
                   step);
  XSTATS_INIT_STAT(madvises, "Time for madvising pages (ns)", start, end, step);
  XSTATS_INIT_STAT(restore_mem, "Time for restoring writable memory (ns)",
                   start, end, step);
  XSTATS_INIT_STAT(clear_soft, "Time for clearing soft dirty bits (ns)", start,
                   end, step);

  XSTATS_INIT_STAT(registers_store, "Time for storing registers (ns)", start,
                   end, step);
  XSTATS_INIT_STAT(registers_restore, "Time for restoring registers (ns)",
                   start, end, step);

#endif
}

void write_all_stats_as_json(void) {
#if XSTATS_CONFIG_STAT
  prep_stats_json(stats_num_keep_alives);
  prep_stats_json(stats_thread_pool);
  prep_stats_json(stats_groundhog_time);

  prep_stats_json(stats_groundhog_memory_footprint);

  prep_stats_json(stats_seizing);
  prep_stats_json(stats_attaching);
  prep_stats_json(stats_detaching);
  prep_stats_json(stats_interrupting);
  prep_stats_json(stats_num_threads);

  prep_stats_json(stats_num_maps);
  prep_stats_json(stats_num_pages);
  prep_stats_json(stats_num_paged_pages);
  prep_stats_json(stats_num_w_pages);
  prep_stats_json(stats_num_softdirty_pages);
  prep_stats_json(stats_num_softdirty_paged_pages);
  prep_stats_json(stats_num_uffd_dirty_pages);
  prep_stats_json(stats_num_restored_pages);
  prep_stats_json(stats_num_mmaps);
  prep_stats_json(stats_num_mprotects);
  prep_stats_json(stats_num_munmaps);

  prep_stats_json(stats_all_inclusive_iter_time);
  prep_stats_json(stats_all_inclusive_checkpoint_time);
  prep_stats_json(stats_groundhog_warm_time);

  prep_stats_json(stats_read_map);
  prep_stats_json(stats_store_mem);
  prep_stats_json(stats_read_pages);
  prep_stats_json(stats_dump_disk);
  prep_stats_json(stats_read_disk);

  prep_stats_json(stats_snapshot);
  prep_stats_json(stats_anon);
  prep_stats_json(stats_diff);
  prep_stats_json(stats_restore);
  prep_stats_json(stats_syscalls);
  prep_stats_json(stats_brks);
  prep_stats_json(stats_mmaps);
  prep_stats_json(stats_mprotects);
  prep_stats_json(stats_munmaps);
  prep_stats_json(stats_madvises);
  prep_stats_json(stats_restore_mem);
  prep_stats_json(stats_clear_soft);

  prep_stats_json(stats_registers_store);
  prep_stats_json(stats_registers_restore);

#endif
}

void cleanup_all_stats(void) {
#if XSTATS_CONFIG_STAT
  XSTATS_DEST_STAT(num_keep_alives, 1);
  XSTATS_DEST_STAT(thread_pool, 1);
  XSTATS_DEST_STAT(groundhog_time, 1);

  XSTATS_DEST_STAT(groundhog_memory_footprint, 1);

  XSTATS_DEST_STAT(seizing, 1);
  XSTATS_DEST_STAT(attaching, 1);
  XSTATS_DEST_STAT(detaching, 1);
  XSTATS_DEST_STAT(interrupting, 1);
  XSTATS_DEST_STAT(num_threads, 1);

  XSTATS_DEST_STAT(num_maps, 1);
  XSTATS_DEST_STAT(num_pages, 1);
  XSTATS_DEST_STAT(num_paged_pages, 1);
  XSTATS_DEST_STAT(num_w_pages, 1);
  XSTATS_DEST_STAT(num_softdirty_pages, 1);
  XSTATS_DEST_STAT(num_softdirty_paged_pages, 1);
  XSTATS_DEST_STAT(num_uffd_dirty_pages, 1);
  XSTATS_DEST_STAT(num_restored_pages, 1);
  XSTATS_DEST_STAT(num_mmaps, 1);
  XSTATS_DEST_STAT(num_mprotects, 1);
  XSTATS_DEST_STAT(num_munmaps, 1);

  XSTATS_DEST_STAT(all_inclusive_iter_time, 1);
  XSTATS_DEST_STAT(all_inclusive_checkpoint_time, 1);
  XSTATS_DEST_STAT(groundhog_warm_time, 1);

  XSTATS_DEST_STAT(read_map, 1);
  XSTATS_DEST_STAT(store_mem, 1);
  XSTATS_DEST_STAT(read_pages, 1);
  XSTATS_DEST_STAT(dump_disk, 1);
  XSTATS_DEST_STAT(read_disk, 1);
  XSTATS_DEST_STAT(snapshot, 1);
  XSTATS_DEST_STAT(anon, 1);
  XSTATS_DEST_STAT(diff, 1);
  XSTATS_DEST_STAT(restore, 1);
  XSTATS_DEST_STAT(syscalls, 1);
  XSTATS_DEST_STAT(brks, 1);
  XSTATS_DEST_STAT(mmaps, 1);
  XSTATS_DEST_STAT(mprotects, 1);
  XSTATS_DEST_STAT(munmaps, 1);
  XSTATS_DEST_STAT(madvises, 1);
  XSTATS_DEST_STAT(restore_mem, 1);
  XSTATS_DEST_STAT(clear_soft, 1);
  XSTATS_DEST_STAT(registers_store, 1);
  XSTATS_DEST_STAT(registers_restore, 1);

#endif
}

void aggregate_dirty_stats(trackedprocess_t *tracked_proc) {
  uint64_t count_soft_dirty = 0;
  uint64_t count_soft_dirty_paged = 0;
  uint64_t count_non_anon = 0;
  for (uint64_t i = 0; i < tracked_proc->p_curr_state->maps_count; i++) {
    count_soft_dirty += tracked_proc->p_curr_state->maps[i].soft_dirty_count;
    count_soft_dirty_paged +=
        tracked_proc->p_curr_state->maps[i].soft_dirty_paged_count;
    if (IS_MAP_WRITEABLE_NON_ANON(&tracked_proc->p_curr_state->maps[i]))
      count_non_anon += tracked_proc->p_curr_state->maps[i].total_num_pages;
  }

  XSTATS_ADD_COUNT_POINT(num_softdirty_pages, count_soft_dirty);
  XSTATS_ADD_COUNT_POINT(num_softdirty_paged_pages, count_soft_dirty_paged);

  jprint(LVL_TEST, "SD: %ld, SD-paged %ld, Non-Anon %ld", count_soft_dirty,
         count_soft_dirty_paged, count_non_anon);
}
