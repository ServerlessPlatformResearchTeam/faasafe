#ifndef _PROC_PARSERS_
#define _PRC_PARSERS_

#include "structs.h"
#include "mem_helpers.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
size_t get_tids(pid_t **const listptr, size_t *const sizeptr, const pid_t pid);

static inline uint64_t
parse_hex2dec_untill(char *buffer, char del, size_t *buf_pointer,
                     size_t buf_size); // __attribute__((always_inline));

static inline uint64_t
parse_dec_untill(char *buffer, char del, size_t *buf_pointer,
                 size_t buf_size); // __attribute__((always_inline));

int read_maps(int maps_fd, process_mem_info_t *ps);
int read_range_pagemap(struct read_range_pagemap_arguments *threadFuncArgs);
int populate_memory_copy_info_map(
    struct populate_memory_copy_info_arguments *threadFuncArgs);

#endif
