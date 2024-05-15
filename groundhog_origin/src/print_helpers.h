#ifndef _PRINT_HELPERS_
#define _PRINT_HELPERS_

#include "libs/xstats.h"
#include "proc_control.h"
#include <sys/types.h>

// printers
void print_map(map_t *map);

void print_maps(process_mem_info_t *process_mem_info);

void print_pages(process_mem_info_t *process_mem_info);

void print_maps_index(process_mem_info_t *process_mem_info, int index);

void print_pages_index(process_mem_info_t *process_mem_info, int index);

void print_page(page_t *page, char *name);

void print_diff_stats(process_mem_diff_t *process_mem_diff);

void print_registers(struct user_regs_struct regs);

void print_sd_bits_externally(int pid);

#endif