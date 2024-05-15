#include "print_helpers.h"

void print_map(map_t *map) {
  map_t tmp_map = *map;
  jprint(LVL_EXP, "%lx-%lx %d%d%d%d %u %u:%u %u %lu (%lu) %lu (%lu) hf:%d %s",
         tmp_map.addr_start, tmp_map.addr_end, tmp_map.r, tmp_map.w, tmp_map.x,
         tmp_map.p, tmp_map.offset, tmp_map.dev_major, tmp_map.dev_minor,
         tmp_map.inode, tmp_map.total_num_pages, tmp_map.total_num_paged_pages,
         tmp_map.soft_dirty_count, tmp_map.soft_dirty_paged_count,
         tmp_map.has_freed, tmp_map.name);
}

void print_map_short(map_t *map) {
  map_t tmp_map = *map;
  jprint(LVL_TEST, "%lx-%lx %d%d%d%d #pages:%lu #soft-dirty:%lu %s",
         tmp_map.addr_start, tmp_map.addr_end, tmp_map.r, tmp_map.w, tmp_map.x,
         tmp_map.p, tmp_map.total_num_pages, tmp_map.soft_dirty_count,
         tmp_map.name);
}

void print_map_soft_dirtied_vmindexes(int map_index, process_mem_info_t *ps) {
  map_t map = ps->maps[map_index];
  page_t *ps_pages = ps->pages;
  uint64_t addr_start = map.addr_start;
  for (int i = 0; i < map.total_num_pages; i++) {
    if (ps_pages[map.first_page_index + i].soft_dirty &&
        ps_pages[map.first_page_index + i].pfn) {
      jprint(LVL_EXP, "%d | %ld | SD-VM-INDEX %ld", map_index,
             addr_start / PSIZE, (addr_start / PSIZE) + i);
    }
  }
}

void print_registers(struct user_regs_struct regs) {
  jprint(LVL_EXP, "RAX: 0x%lx RBX: 0x%lx RCX: 0x%lx RDX: 0x%lx", (long)regs.rax,
         (long)regs.rbx, (long)regs.rcx, (long)regs.rdx);
  jprint(LVL_EXP, "R15: 0x%lx R14: 0x%lx R13: 0x%lx R12: 0x%lx", (long)regs.r15,
         (long)regs.r14, (long)regs.r13, (long)regs.r12);
  jprint(LVL_EXP, "R11: 0x%lx R10: 0x%lx R9:  0x%lx R8:  0x%lx", (long)regs.r11,
         (long)regs.r10, (long)regs.r9, (long)regs.r8);
  jprint(LVL_EXP, "RSP: 0x%ld RBP: 0x%lx RSI: 0x%lx RDI: 0x%lx", (long)regs.rsp,
         (long)regs.rbp, (long)regs.rsi, (long)regs.rdi);
  jprint(LVL_EXP, "RIP: 0x%lx CS: 0x%lx EFLAGS: 0x%lx", (long)regs.rip,
         (long)regs.cs, (long)regs.eflags);
  jprint(LVL_EXP, "SS: 0x%lx DS: 0x%lx ES: 0x%lx FS: 0x%lx GS: 0x%lx",
         (long)regs.ss, (long)regs.ds, (long)regs.es, (long)regs.fs,
         (long)regs.gs);
  jprint(LVL_EXP, "ORIG_RAX: 0x%lx", (long)regs.orig_rax);
}

void print_xstate(struct xsave_struct *regs) {
  jprint(LVL_EXP, "cwd: 0x%x swd: 0x%x twd: 0x%x fop: 0x%x", regs->i387.cwd,
         regs->i387.swd, regs->i387.twd, regs->i387.fop);
  jprint(LVL_EXP, "fip: 0x%x fcs: 0x%x foo: 0x%x fos: 0x%x", regs->i387.fip,
         regs->i387.fcs, regs->i387.foo, regs->i387.fos);
  uint64_t sum_st_space = 0;
  uint64_t sum_xmm_space = 0;
  for (int i = 0; i < 64; i++) {
    if (i < 32)
      sum_st_space += regs->i387.st_space[i];
    sum_xmm_space += regs->i387.xmm_space[i];
  }
  jprint(LVL_EXP,
         "mcsr: 0x%x mcsr_mask: 0x%x sum_st_space: 0x%lx sum_xmm_space: 0x%lx",
         regs->i387.mxcsr, regs->i387.mxcsr_mask, sum_st_space, sum_xmm_space);
  uint64_t sum_exs = 0;
  for (int i = 0; i < EXTENDED_STATE_AREA_SIZE; i++)
    sum_exs += regs->extended_state_area[i];
  jprint(LVL_EXP, "xstate_bv: 0x%lx xcomp_bv: 0x%lx sum_exs: %lx",
         regs->xsave_hdr.xstate_bv, regs->xsave_hdr.xcomp_bv, sum_exs);
}

void print_page(page_t *page, char *name) {
  fprintf(stderr,
          "start_addr %lx : pfn %lx soft-dirty %d file/shared %d "
          "exclusive %d swapped %d present %d library %s\n",
          (uint64_t)page->addr_start, (uint64_t)page->pfn,
          (uint32_t)page->soft_dirty, (uint32_t)page->file_or_anon,
          (uint32_t)page->exclusive, (uint32_t)page->swapped,
          (uint32_t)page->present, name);
}

void print_maps_index(process_mem_info_t *process_mem_info, int index) {
  print_map(&(process_mem_info->maps[index]));
}

void print_pages_index(process_mem_info_t *process_mem_info, int index) {
  page_t tmp_page = process_mem_info->pages[index];
  map_t tmp_map = process_mem_info->maps[tmp_page.map_index];
  print_map(&tmp_map);
  print_page(&tmp_page, tmp_map.name);
}

void print_maps(process_mem_info_t *process_mem_info) {
  int i = 0;
  for (i = 0; i < process_mem_info->maps_count; i++) {
    print_maps_index(process_mem_info, i);
  }
  jprint(LVL_EXP, "A total of %d maps", i);
}

void print_pages(process_mem_info_t *process_mem_info) {
  for (int i = 0; i < process_mem_info->pages_count; i++) {
    print_pages_index(process_mem_info, i);
  }
}

void print_diff_stats(process_mem_diff_t *process_mem_diff) {
  jprint(LVL_EXP,
         "Diff of memory regions:\n"
         "\t#modified memory regions = %lu\n"
         "\t#added memory regions = %lu\n"
         "\t#removed memory regions = %lu",
         process_mem_diff->modified.maps_count,
         process_mem_diff->added.maps_count,
         process_mem_diff->removed.maps_count);
  jprint(LVL_EXP,
         "Diff of memory pages:\n"
         "\t#modified memory pages = %lu\n"
         "\t#added memory pages = %lu\n"
         "\t#removed memory pages = %lu",
         process_mem_diff->modified.pages_count,
         process_mem_diff->added.pages_count,
         process_mem_diff->removed.pages_count);
}

void print_soft_dirty_stats(process_mem_info_t *ps) {
  uint64_t total_soft_dirtied = 0;
  uint64_t total_soft_dirtied_paged = 0;
  uint64_t total_shared_pages = 0;
  uint64_t total_paged_pages = ps->paged_pages_count;
  for (int i = 0; i < ps->maps_count; i++) {
    int curr_map_soft_dirty_count = ps->maps[i].soft_dirty_count;
    int curr_map_soft_dirty_paged_count = ps->maps[i].soft_dirty_paged_count;
    if (!ps->maps[i].p) {
      total_shared_pages += ps->maps[i].total_num_pages;
    }
    if (curr_map_soft_dirty_count > 0) {
      total_soft_dirtied += curr_map_soft_dirty_count;
      total_soft_dirtied_paged += curr_map_soft_dirty_paged_count;
      print_map_short(&(ps->maps[i]));
      print_map_soft_dirtied_vmindexes(i, ps);
    }
  }

  jprint(LVL_EXP,
         "Total number of soft-dirtied pages is: %lu ( %lu ) out of %lu ( p: "
         "%lu ) %.2f%% ( p: %.2f%% sdp: %.2f%%)",
         total_soft_dirtied, total_soft_dirtied_paged, ps->pages_count,
         total_paged_pages, 100.0 * total_soft_dirtied / ps->pages_count,
         100.0 * total_soft_dirtied / total_paged_pages,
         100.0 * total_soft_dirtied_paged / total_paged_pages);
  jprint(LVL_EXP, "%lu out of %lu (%.2f%%) are shared", total_shared_pages,
         ps->pages_count, 100.0 * total_shared_pages / ps->pages_count);
}

void print_soft_dirty_stats_and_diff(process_mem_info_t *ps,
                                     process_mem_diff_t *process_mem_diff,
                                     uint64_t pages_count,
                                     uint64_t total_paged_pages) {
  uint64_t total_soft_dirtied = 0;
  uint64_t total_soft_dirtied_paged = 0;
  uint64_t total_shared_pages = 0;
  for (int i = 0; i < ps->maps_count; i++) {
    int curr_map_soft_dirty_count = ps->maps[i].soft_dirty_count;
    int curr_map_soft_dirty_paged_count = ps->maps[i].soft_dirty_paged_count;
    if (!ps->maps[i].p) {
      total_shared_pages += ps->maps[i].total_num_pages;
    }
    if (curr_map_soft_dirty_count > 0) {
      total_soft_dirtied += curr_map_soft_dirty_count;
      total_soft_dirtied_paged += curr_map_soft_dirty_paged_count;
      print_map_short(&(ps->maps[i]));
      print_map_soft_dirtied_vmindexes(i, ps);
    }
  }

  jprint(LVL_EXP,
         "Total number of soft-dirtied pages is: %lu ( %lu ) out of %lu ( p: "
         "%lu ) %.2f%% ( p: %.2f%% sdp: %.2f%%) %lu %lu %lu %lu %lu %lu",
         total_soft_dirtied, total_soft_dirtied_paged, pages_count,
         total_paged_pages, 100.0 * total_soft_dirtied / pages_count,
         100.0 * total_soft_dirtied / total_paged_pages,
         100.0 * total_soft_dirtied_paged / total_paged_pages,
         process_mem_diff->modified.maps_count,
         process_mem_diff->added.maps_count,
         process_mem_diff->removed.maps_count,
         process_mem_diff->modified.pages_count,
         process_mem_diff->added.pages_count,
         process_mem_diff->removed.pages_count);
  jprint(LVL_EXP, "%lu out of %lu (%.2f%%) are shared", total_shared_pages,
         pages_count, 100.0 * total_shared_pages / ps->pages_count);
}

void print_sd_bits_externally(int pid) {
  char cmd[BUFSIZ];
  sprintf(cmd, "./build/dump_sd_pages %d", pid);
  FILE *fp;
  char buf[BUFSIZ];

  if ((fp = popen(cmd, "r")) == NULL) {
    fprintf(stderr, "Error opening pipe!\n");
  }

  while (fgets(buf, BUFSIZ, fp) != NULL) {
    // Do whatever you want here...
    fprintf(stderr, "DUMPING SD BITS");
    fprintf(stderr, "CHECKING: \n %s", buf);
    fflush(stderr);
  }

  if (pclose(fp)) {
    fprintf(stderr, "Command not found or exited with error status\n");
  }
}
