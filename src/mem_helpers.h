#ifndef _MEM_HELPERS_
#define _MEM_HELPERS_

#include "libs/xstats.h"
#include "structs.h"
#include "print_helpers.h"
#include "proc_parsers.h"

void init_maps(map_t **maps, uint64_t initial_capacity);
void init_pages_info(page_t **pages, uint64_t num_pages);
void add_to_maps(process_mem_info_t *ps, map_t map);
void add_page_to_map_at_index(process_mem_info_t *ps, page_t page, int index);
void add_page_to_map(process_mem_info_t *ps, page_t page);
int get_map_index_by_name(process_mem_info_t *ps, char *libname,
                          bool start_low);

static inline uint64_t
parse_hex2dec_untill(char *buffer, char del, size_t *buf_pointer,
                     size_t buf_size); // __attribute__((always_inline));

static inline uint64_t
parse_dec_untill(char *buffer, char del, size_t *buf_pointer,
                 size_t buf_size); // __attribute__((always_inline));

size_t get_tids(pid_t **const listptr, size_t *const sizeptr, const pid_t pid);

int populate_page_info(int pagemaps_fd, process_mem_info_t *ps,
                       trackedprocess_t *tracked_proc);
int populate_memory_copy_info(int mem_fd, process_mem_info_t *ps,
                              trackedprocess_t *tracked_proc,
                              int only_writable);

void init_memory_copy_info(char **memory, uint64_t num_pages);
void init_process_mem_info(process_mem_info_t *process_state, int pid);

// cleanup
void *free_process_mem_diff(process_mem_diff_t *process_mem_diff);

void *free_process_mem_info(process_mem_info_t *process_mem_info,
                            int free_pointer);

void *free_tracked_proc(trackedprocess_t *tracked_proc);

#endif
