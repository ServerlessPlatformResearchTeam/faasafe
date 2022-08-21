#include "libs/xdebug.h"
#include "libs/xstats.h"
#include "structs.h"
#include <sys/types.h>

#if XSTATS_CONFIG_STAT

/*groundhog stats*/
XSTATS_DECL_STAT_EXTERN(num_keep_alives);
XSTATS_DECL_STAT_EXTERN(thread_pool);
XSTATS_DECL_STAT_EXTERN(groundhog_time);

XSTATS_DECL_STAT_EXTERN(groundhog_memory_footprint);
/*proc control*/
XSTATS_DECL_STAT_EXTERN(seizing);
XSTATS_DECL_STAT_EXTERN(attaching);
XSTATS_DECL_STAT_EXTERN(detaching);
XSTATS_DECL_STAT_EXTERN(interrupting);
XSTATS_DECL_STAT_EXTERN(num_threads);

/*mem stats*/
XSTATS_DECL_STAT_EXTERN(num_maps);
XSTATS_DECL_STAT_EXTERN(num_pages);
XSTATS_DECL_STAT_EXTERN(num_paged_pages);
XSTATS_DECL_STAT_EXTERN(num_w_pages);
XSTATS_DECL_STAT_EXTERN(num_softdirty_pages);
XSTATS_DECL_STAT_EXTERN(num_softdirty_paged_pages);
XSTATS_DECL_STAT_EXTERN(num_uffd_dirty_pages);
XSTATS_DECL_STAT_EXTERN(num_restored_pages);
XSTATS_DECL_STAT_EXTERN(num_mmaps);
XSTATS_DECL_STAT_EXTERN(num_mprotects);
XSTATS_DECL_STAT_EXTERN(num_munmaps);

XSTATS_DECL_STAT_EXTERN(all_inclusive_iter_time);
XSTATS_DECL_STAT_EXTERN(all_inclusive_checkpoint_time);
XSTATS_DECL_STAT_EXTERN(groundhog_warm_time);

XSTATS_DECL_STAT_EXTERN(read_map);
XSTATS_DECL_STAT_EXTERN(store_mem);
XSTATS_DECL_STAT_EXTERN(read_pages);
XSTATS_DECL_STAT_EXTERN(dump_disk);
XSTATS_DECL_STAT_EXTERN(read_disk);
XSTATS_DECL_STAT_EXTERN(snapshot);
XSTATS_DECL_STAT_EXTERN(anon);
XSTATS_DECL_STAT_EXTERN(diff);
XSTATS_DECL_STAT_EXTERN(restore);
XSTATS_DECL_STAT_EXTERN(syscalls);
XSTATS_DECL_STAT_EXTERN(brks);
XSTATS_DECL_STAT_EXTERN(mmaps);
XSTATS_DECL_STAT_EXTERN(mprotects);
XSTATS_DECL_STAT_EXTERN(munmaps);
XSTATS_DECL_STAT_EXTERN(madvises);
XSTATS_DECL_STAT_EXTERN(restore_mem);
XSTATS_DECL_STAT_EXTERN(clear_soft);

XSTATS_DECL_STAT_EXTERN(registers_store);
XSTATS_DECL_STAT_EXTERN(registers_restore);

#endif

void init_all_stats(void);
void cleanup_all_stats(void);
void write_all_stats_as_json(void);
void aggregate_dirty_stats(trackedprocess_t *tracked_proc);
