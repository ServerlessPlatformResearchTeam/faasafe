#include "mem_helpers.h"

void init_maps(map_t **maps, uint64_t initial_capacity) {
  *maps = (map_t *)calloc(initial_capacity, sizeof(map_t));
  if (maps == NULL) {
    iprintl(LVL_EXP, "CALLOC ERROR: %s", strerror(errno));
    exit(-1);
  }
}

void init_pages_info(page_t **pages, uint64_t num_pages) {
  *pages = (page_t *)calloc(num_pages, sizeof(page_t));
  if (pages == NULL) {
    iprintl(LVL_EXP, "CALLOC ERROR: %s", strerror(errno));
    exit(-1);
  }
}

// not thread safe
void add_to_maps(process_mem_info_t *ps, map_t map) {
  int original_maps_max_capacity = ps->maps_max_capacity;
  int index = ps->maps_count;
  if (index >= original_maps_max_capacity) {
    ps->maps_max_capacity *= 2;
    ps->maps =
        (map_t *)realloc(ps->maps, ps->maps_max_capacity * sizeof(map_t));
    // clear the rest of the memory
    memset(ps->maps + original_maps_max_capacity, 0,
           sizeof(map_t) * (original_maps_max_capacity));
  }
  ps->pages_max_capacity += map.total_num_pages;
  //__atomic_fetch_add(&ps->pages_max_capacity,  map.total_num_pages,
  //__ATOMIC_SEQ_CST);
  ps->max_num_pages_in_mem_region =
      MAX(map.total_num_pages, ps->max_num_pages_in_mem_region);
  ps->maps[index] = map;
  ps->maps_count += 1;
  if (map.w) {
    ps->writable_pages_count += map.total_num_pages;
  }
  ps->paged_pages_count += map.total_num_paged_pages;
}

void add_page_to_map_at_index(process_mem_info_t *ps, page_t page, int index) {
  assert(ps->pages_max_capacity > index);

  ps->pages[index] = page;
  map_t *map = &(ps->maps[page.map_index]);
  if (page.pfn > 0) {
    map->total_num_paged_pages += 1;
  }

  // map_t *map = &(ps->maps[page.map_index]);
  map->populated_num_pages += 1;
  if (page.is_freed) {
    map->has_freed = 1;
    map->freed_count += 1;
  }
  if (page.soft_dirty) {
    map->soft_dirty_count += 1;
    if (page.pfn > 0) {
      map->soft_dirty_paged_count += 1;
      if (IS_MAP_WRITEABLE_ANON(map)) {
        map->uffd_dirty_count += 1;
      }
    }
  }
}

void add_page_to_map(process_mem_info_t *ps, page_t page) {
  int original_pages_max_capacity = ps->pages_max_capacity;
  // int index = ps->pages_count;
  uint64_t index = ps->maps[page.map_index].first_page_index +
                   ps->maps[page.map_index].populated_num_pages;
  /*
  if (index >= ps->pages_max_capacity)
          jprint(LVL_EXP, "%ld %ld map_index %ld out of %ld",
  ps->pages_max_capacity, index, page.map_index, ps->maps_count);
  fflush(stdout);
  assert(ps->pages_max_capacity > index);
  */
  if (index >= original_pages_max_capacity) {
    // TODO: make sure we do not go here if multithreaded
    jprint(LVL_TEST, "RESIZING pagemap");
    ps->pages_max_capacity *= 2;
    ps->pages =
        (page_t *)realloc(ps->pages, ps->pages_max_capacity * sizeof(page_t));
    // clear the rest of the memory
    memset(ps->pages + original_pages_max_capacity, 0,
           sizeof(page_t) * (original_pages_max_capacity));
  }
  add_page_to_map_at_index(ps, page, index);
}

int get_map_index_by_name(process_mem_info_t *ps, char *libname,
                          bool start_low) {
  if (start_low) { // heap quickly
    for (int i = 0; i < ps->maps_count; i++) {
      // we want the highest heap, we start from bottom
      if (!strncmp(ps->maps[i].name, libname, strlen(libname)) &&
          i < ps->maps_count - 1 &&
          strncmp(ps->maps[i + 1].name, libname, strlen(libname)))
        return i;
    }
  } else { // stack quickly
    for (int i = ps->maps_count - 1; i >= 0; i--) {
      if (!strncmp(ps->maps[i].name, libname, strlen(libname)))
        return i;
    }
  }
  return -1;
}

int populate_page_info(int pagemaps_fd, process_mem_info_t *ps,
                       trackedprocess_t *tracked_proc) {
  struct read_range_pagemap_arguments threadFuncArgs[ps->maps_count];
  for (int i = 0; i < ps->maps_count; i++) {
#if XDEBUG_LVL <= LVL_DBG
    print_maps_index(ps, i);
#endif
    threadFuncArgs[i] = (struct read_range_pagemap_arguments){
        pagemaps_fd, ps, &(ps->maps[i]), i};
#if CONCURRENCY > 1
    thpool_add_work(tracked_proc->thpool, (void *)&read_range_pagemap,
                    &threadFuncArgs[i]);
#else
    read_range_pagemap(&threadFuncArgs[i]);
#endif
  }
#if CONCURRENCY > 1
  thpool_wait(tracked_proc->thpool);
#endif
  jprint(LVL_INFO, "Populated %lu pages", ps->pages_count);
  return -1;
}

int populate_memory_copy_info(int mem_fd, process_mem_info_t *ps,
                              trackedprocess_t *tracked_proc,
                              int only_writable) {
  // TODO: Optimize the saved pages, we need not save all read only pages.
  struct populate_memory_copy_info_arguments threadFuncArgs[ps->maps_count];
  uint64_t memory_index = 0;
  for (int i = 0; i < ps->maps_count; i++) {
    // print_map(&ps->maps[i]);
    if (only_writable && !ps->maps[i].w)
      continue;
    ps->maps[i].first_wmem_index = memory_index;
    // memory_index += ps->maps[i].total_num_pages;
    memory_index += ps->maps[i].total_num_paged_pages;
    threadFuncArgs[i] = (struct populate_memory_copy_info_arguments){
        mem_fd, ps, &(ps->maps[i])};
#if CONCURRENCY > 1
    thpool_add_work(tracked_proc->thpool,
                    (void *)&populate_memory_copy_info_map, &threadFuncArgs[i]);
#else
    if (ps->maps[i].total_num_paged_pages > 0)
      populate_memory_copy_info_map(&threadFuncArgs[i]);
#endif
  }
#if CONCURRENCY > 1
  thpool_wait(tracked_proc->thpool);
#endif
}

void init_memory_copy_info(char **memory, uint64_t num_pages) {
  *memory = (char *)calloc(PSIZE * num_pages, sizeof(char));
  if (memory == NULL) {
    iprintl(LVL_EXP, "CALLOC ERROR: %s", strerror(errno));
    exit(-1);
  }
}
void init_process_mem_info(process_mem_info_t *process_state, int pid) {
  process_state->pid = pid;
  process_state->maps_count = 0;
  process_state->pages_count = 0;
  process_state->writable_pages_count = 0;
  process_state->paged_pages_count = 0;
  process_state->max_num_pages_in_mem_region = 0;
  process_state->maps_max_capacity = INITIAL_NUMBER_OF_REGIONS;
  process_state->pages_max_capacity = 0;
  init_maps(&(process_state->maps), process_state->maps_max_capacity);
  process_state->pages = NULL;
}

void *free_process_mem_info(process_mem_info_t *process_mem_info,
                            int free_pointer) {
  if (process_mem_info->maps != NULL)
    free(process_mem_info->maps);
  if (process_mem_info->pages != NULL)
    free(process_mem_info->pages);
  if (process_mem_info->wmem != NULL)
    free(process_mem_info->wmem);
  if (free_pointer) {
    free(process_mem_info);
  }
  return NULL;
}

void *free_process_mem_diff(process_mem_diff_t *ps_diff) {
  free_process_mem_info(&(ps_diff->added), 0);
  free_process_mem_info(&(ps_diff->removed), 0);
  free_process_mem_info(&(ps_diff->modified), 0);
  free_process_mem_info(&(ps_diff->modified_protection), 0);
  free(ps_diff);
  return NULL;
}

void *free_tracked_proc(trackedprocess_t *tracked_proc) {
  if (tracked_proc->p_checkpointed_state)
    free_process_mem_info(tracked_proc->p_checkpointed_state, 1);
  if (tracked_proc->p_curr_state)
    free_process_mem_info(tracked_proc->p_curr_state, 1);
  free(tracked_proc);
  return NULL;
}
