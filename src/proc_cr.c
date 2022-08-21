/*
 * functions:
 * - checkpoint(trackedprocess_t *tracked_proc)
 * - restore(trackedprocess_t *tracked_proc)
 * are the orchestrators of the operations.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "proc_cr.h"
int num_checkpoints = 0;

struct cr_operation {
  void (*clear_dirty_bits)(trackedprocess_t *tracked_proc);
  int (*restore_process)(trackedprocess_t *tracked_proc);
  process_mem_info_t *(*checkpoint_process)(trackedprocess_t *tracked_proc);
};

struct cr_operation cr_ops = {
    .clear_dirty_bits = clear_soft_dirty_bits,
    .restore_process = restore_process_sd,
    .checkpoint_process = checkpoint_process_sd,
};

void clear_soft_dirty_bits(trackedprocess_t *tracked_proc) {
  // clear in the PTEs
  int fd = tracked_proc->clear_fd;
  XSTATS_INIT_TIMER(clear_soft);
  XSTATS_START_TIMER(clear_soft);
  write(fd, "4", sizeof("4"));
  XSTATS_END_TIMER(clear_soft);

#if XDEBUG_LVL <= LVL_TEST
  jprint(LVL_TEST, "clear_soft_dirty_bits");
  print_sd_bits_externally(tracked_proc->pid);
#endif
  // clear our internal datastructures
  process_mem_info_t *ps = tracked_proc->p_checkpointed_state;
  if (!ps)
    return;
  for (int i = 0; i < ps->maps_count; i++) {
    ps->maps[i].soft_dirty_count = 0;
    ps->maps[i].freed_count = 0;
    ps->maps[i].dirty = 0;
  }
}

static int store_threads_registers(trackedprocess_t *tracked_proc) {
  XSTATS_INIT_TIMER(registers_store);
  XSTATS_START_TIMER(registers_store);
  int tid;
  // should parse /proc again, but for now
  // we assume that the number of threads didn't change
  for (int i = 0; i < tracked_proc->tid_count; i++) {
    tid = tracked_proc->tids[i];
    siginfo_t si;
    if (tid < 0)
      continue;

    memset(&tracked_proc->txsave[i], 0, sizeof(&tracked_proc->txsave[i]));
    tracked_proc->tiov[i].iov_base = &tracked_proc->txsave[i];
    tracked_proc->tiov[i].iov_len = sizeof(tracked_proc->txsave[i]);

    assert(!ptrace(PTRACE_GETFPREGS, tid, NULL, &tracked_proc->txsave[i]));
    // print_xstate(&tracked_proc->txsave[i]);
    // if avx supported uncomment the next line only
    assert(!ptrace(PTRACE_GETREGSET, tid, (unsigned int)NT_X86_XSTATE,
                   &tracked_proc->tiov[i]));

    assert(!ptrace(PTRACE_GETREGS, tid, NULL, &tracked_proc->tregs[i]));
    assert(!ptrace(PTRACE_GETSIGINFO, tid, NULL, &si));
    assert(!ptrace(PTRACE_SETSIGINFO, tid, NULL, &si));
    sigemptyset(&tracked_proc->tmask[i]);
    assert(!ptrace(PTRACE_GETSIGMASK, tid, 8, &tracked_proc->tmask[i]));
    tracked_proc->si[i] = si;
    errno = 0;
    unsigned long peekdata =
        ptrace(PTRACE_PEEKTEXT, tid,
               tracked_proc->tregs[i].rip - SIZEOF_SYSCALL, NULL);

    jprint(LVL_TEST,
           "We seized the thread %d, rip: %llx, our next instruction is: %lx ",
           tid, tracked_proc->tregs[i].rip, SYSCALL_MASK & peekdata);
    if (!errno && ((SYSCALL_MASK & peekdata) == SYSCALL)) {
      jprint(LVL_TEST, "We are entering the %s SYSCALL",
             name_of_syscall_number(tracked_proc->tregs[i].orig_rax));
    }
  }
  XSTATS_END_TIMER(registers_store);
}

static int restore_threads_registers(trackedprocess_t *tracked_proc) {
  XSTATS_INIT_TIMER(registers_restore);
  XSTATS_START_TIMER(registers_restore);
  int tid;
  for (int i = 0; i < tracked_proc->tid_count; i++) {
    tid = tracked_proc->tids[i];
    if (tid < 0)
      continue;
    siginfo_t si = tracked_proc->si[i];
    int status;

    // print_registers(tracked_proc->tregs[i]);
    assert(!ptrace(PTRACE_SETFPREGS, tid, NULL, &tracked_proc->txsave[i]));
    // assert(!ptrace(PTRACE_SETREGSET, tid, (unsigned int) NT_PRSTATUS,
    // &tracked_proc->tiov[i]));
    assert(!ptrace(PTRACE_SETSIGINFO, tid, NULL, &tracked_proc->si[i]));
    assert(!ptrace(PTRACE_SETSIGMASK, tid, 8, &tracked_proc->tmask[i]));
    // if avx supported, uncomment the next line only
    assert(!ptrace(PTRACE_SETREGSET, tid, (unsigned int)NT_X86_XSTATE,
                   &tracked_proc->tiov[i]));
    assert(!ptrace(PTRACE_SETREGS, tid, NULL, (void *)&tracked_proc->tregs[i]));
    // usleep(1e3);
    errno = 0;
    unsigned long peekdata =
        ptrace(PTRACE_PEEKTEXT, tid,
               tracked_proc->tregs[i].rip - SIZEOF_SYSCALL, NULL);
    jprint(LVL_TEST,
           "We will continue %d, rip: %llx, our next instruction is: %lx ", tid,
           tracked_proc->tregs[i].rip, SYSCALL_MASK & peekdata);
    if (!errno && ((SYSCALL_MASK & peekdata) == SYSCALL)) {
      if (tracked_proc->tregs[i].rax == -ENOSYS) {
        jprint(LVL_TEST, "We are entering the %s SYSCALL",
               name_of_syscall_number(tracked_proc->tregs[i].orig_rax));
      } else {
        jprint(LVL_TEST, "We are returning from the %s SYSCALL",
               name_of_syscall_number(tracked_proc->tregs[i].orig_rax));
      }
    }
  }
  XSTATS_END_TIMER(registers_restore);
}

int remove_added_memory(trackedprocess_t *tracked_proc,
                        process_mem_diff_t *p_diff) {
  // do not remove the [heap]/[stack] if it was extended, we will take care of
  // it seprately (brk)
  int count_syscalls = 0;
  XSTATS_INIT_TIMER(munmaps);
  for (int i = 0; i < p_diff->added.maps_count; i++) {
    if (!strncmp(p_diff->added.maps[i].name, "[heap]", strlen("[heap]")))
      continue;
    if (!strncmp(p_diff->added.maps[i].name, "[stack]", strlen("[stack]")))
      continue;

    jprint(LVL_TEST, "Will remove the mapping of %lx-%lx",
           p_diff->added.maps[i].addr_start, p_diff->added.maps[i].addr_end);
    XSTATS_START_TIMER(munmaps);
    do_munmap(tracked_proc, p_diff->added.maps[i].addr_start,
              p_diff->added.maps[i].addr_end -
                  p_diff->added.maps[i].addr_start);
    XSTATS_END_TIMER(munmaps);
    count_syscalls += 1;
  }
  XSTATS_ADD_COUNT_POINT(num_munmaps, count_syscalls);
}

int restore_removed_memory(trackedprocess_t *tracked_proc,
                           process_mem_diff_t *p_diff) {
  int count_syscalls = 0;
  XSTATS_INIT_TIMER(mmaps);
  for (int i = 0; i < p_diff->removed.maps_count; i++) {
    jprint(LVL_TEST, "Will remap the mapping of %lx",
           p_diff->removed.maps[i].addr_start);
    //[heap]/[stack] shouldn't have been removed anyway
    if (!strncmp(p_diff->removed.maps[i].name, "[heap]", strlen("[heap]")))
      continue;

    if (!strncmp(p_diff->removed.maps[i].name, "[stack]", strlen("[stack]")))
      continue;
    int prot_flags = 0;
    int prot_map = MAP_FIXED;
    if (p_diff->removed.maps[i].r)
      prot_flags |= PROT_READ;

    if (p_diff->removed.maps[i].w)
      prot_flags |= PROT_WRITE;

    if (p_diff->removed.maps[i].x)
      prot_flags |= PROT_EXEC;

    if (!(p_diff->removed.maps[i].r) && !(p_diff->removed.maps[i].w) &&
        !(p_diff->removed.maps[i].x))
      prot_flags = PROT_NONE;

    if (p_diff->removed.maps[i].p)
      prot_map |= MAP_PRIVATE;
    else
      prot_map |= MAP_SHARED;
    jprint(LVL_TEST,
           "Will remap the mapping of %lx-%lx, with flags 0x%x, and prot 0x%x",
           p_diff->removed.maps[i].addr_start, p_diff->removed.maps[i].addr_end,
           prot_flags, prot_map);
    jprint(LVL_TEST,
           "MAP_FIXED 0x%x MAP_PRIVATE 0x%x MAP_SHARED 0x%x PROT_READ 0x%x "
           "PROT_WRITE 0x%x PROT_NONE 0x%x, ",
           MAP_FIXED, MAP_PRIVATE, MAP_SHARED, PROT_READ, PROT_WRITE,
           PROT_NONE);
    XSTATS_START_TIMER(mmaps);
    do_mmap(tracked_proc, p_diff->removed.maps[i].addr_start,
            p_diff->removed.maps[i].addr_end -
                p_diff->removed.maps[i].addr_start,
            prot_flags, prot_map, -1);
    XSTATS_END_TIMER(mmaps);
    count_syscalls += 1;
  }
  XSTATS_ADD_COUNT_POINT(num_mmaps, count_syscalls);
}

int restore_memory_protection(trackedprocess_t *tracked_proc,
                              process_mem_diff_t *p_diff) {
  int count_syscalls = 0;
  XSTATS_INIT_TIMER(mprotects);
  for (int i = 0; i < p_diff->modified_protection.maps_count; i++) {
    jprint(LVL_TEST, "Will fix the protections of the mapping of %lx",
           p_diff->modified_protection.maps[i].addr_start);

    int prot_flags = 0;
    // int prot_map = MAP_FIXED;
    if (p_diff->modified_protection.maps[i].r)
      prot_flags |= PROT_READ;

    if (p_diff->modified_protection.maps[i].w)
      prot_flags |= PROT_WRITE;

    if (p_diff->modified_protection.maps[i].x)
      prot_flags |= PROT_EXEC;

    if (!(p_diff->modified_protection.maps[i].r) &&
        !(p_diff->modified_protection.maps[i].w) &&
        !(p_diff->modified_protection.maps[i].x))
      prot_flags = PROT_NONE;

    /* We are ignoring other protection flags such as PROT_SEM, PROT_SAO,
       PROT_GROWS*

            */
    jprint(LVL_TEST,
           "Will fix the prtection bits of the mapping of %lx-%lx, with flags "
           "0x%x",
           p_diff->modified_protection.maps[i].addr_start,
           p_diff->modified_protection.maps[i].addr_end, prot_flags);
    jprint(LVL_TEST,
           "MAP_FIXED 0x%x MAP_PRIVATE 0x%x MAP_SHARED 0x%x PROT_READ 0x%x "
           "PROT_WRITE 0x%x PROT_NONE 0x%x, ",
           MAP_FIXED, MAP_PRIVATE, MAP_SHARED, PROT_READ, PROT_WRITE,
           PROT_NONE);
    XSTATS_START_TIMER(mprotects);
    do_mprotect(tracked_proc, p_diff->modified_protection.maps[i].addr_start,
                p_diff->modified_protection.maps[i].addr_end -
                    p_diff->modified_protection.maps[i].addr_start,
                prot_flags);
    XSTATS_END_TIMER(mprotects);
    count_syscalls += 1;
  }
  XSTATS_ADD_COUNT_POINT(num_mprotects, count_syscalls);
}

int restore_brk(trackedprocess_t *tracked_proc, process_mem_diff_t *p_diff) {
  XSTATS_INIT_TIMER(brks);
  XSTATS_START_TIMER(brks);
  jprint(LVL_TEST, "Restoring the program break");
  int heap_index_checkpoint =
      get_map_index_by_name(tracked_proc->p_checkpointed_state, "[heap]", 1);

  int heap_index_currstate =
      get_map_index_by_name(tracked_proc->p_curr_state, "[heap]", 1);

  if (tracked_proc->p_checkpointed_state->maps[heap_index_checkpoint]
          .addr_end !=
      tracked_proc->p_curr_state->maps[heap_index_currstate].addr_end) {
    do_brk(tracked_proc,
           tracked_proc->p_checkpointed_state->maps[heap_index_checkpoint]
               .addr_end);
  }
  XSTATS_END_TIMER(brks);
  return 0;
}

// the stack grows down from hi to low, we must start zeroing from the current
// stack pointer till we reach the checkpointed stack pointer
int zero_stack(int mem_fd, trackedprocess_t *tracked_proc) {
  int stack_index_checkpoint =
      get_map_index_by_name(tracked_proc->p_checkpointed_state, "[stack]", 0);
  int stack_index_currstate =
      get_map_index_by_name(tracked_proc->p_curr_state, "[stack]", 0);
  // did the stack size increas on the page level?
  int size_to_remove =
      tracked_proc->p_checkpointed_state->maps[stack_index_checkpoint]
          .addr_start -
      tracked_proc->p_curr_state->maps[stack_index_currstate].addr_start;
  if (size_to_remove) {
    jprint(LVL_TEST, "Zeroing the rest of the stack %d, starting %lx",
           size_to_remove,
           tracked_proc->p_curr_state->maps[stack_index_currstate].addr_start);
    do_resetrlimit_stack(tracked_proc);
    do_munmap(
        tracked_proc,
        tracked_proc->p_curr_state->maps[stack_index_currstate].addr_start,
        size_to_remove);
  }
}

int restore_registers(process_mem_info_t *initial_state) {
  // print_registers(initial_state->regs);
  jprint(LVL_TEST, "Restoring negisters for pid:%d", initial_state->pid);

  if (ptrace(PTRACE_SETREGS, initial_state->pid, NULL,
             &(initial_state->regs)) == -1)
    iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno),
            initial_state->pid);

  return 0;
}

struct restore_soft_dirty_memory_map_arguments {
  int mem_fd;
  process_mem_diff_t *ps_diff;
  process_mem_info_t *initial_state;
  process_mem_info_t *final_state;
  map_t *mod_init_map;
  uint64_t *count_restored;
  trackedprocess_t *tracked_proc;
  GArray *madvises_array;
};

/*
 * ptrace calls must be done from the main thread
 */

typedef struct madvises_t {
  uint64_t page_start;
  int num_pages;
} madvises_t;

void restore_soft_dirty_memory_map(
    struct restore_soft_dirty_memory_map_arguments *threadFuncArgs) {
  int mem_fd = threadFuncArgs->mem_fd;
  process_mem_diff_t *ps_diff = threadFuncArgs->ps_diff;
  process_mem_info_t *initial_state = threadFuncArgs->initial_state;
  process_mem_info_t *final_state = threadFuncArgs->final_state;
  map_t mod_init_map = *threadFuncArgs->mod_init_map;
  uint64_t *count_restored = threadFuncArgs->count_restored;
  trackedprocess_t *tracked_proc = threadFuncArgs->tracked_proc;
  GArray *madvises_array = threadFuncArgs->madvises_array;
  int r;
  uint64_t init_mem_start_byte;
  uint64_t page_offset_in_mem;
  page_t init_mod_page;
  page_t final_mod_page;

  init_mem_start_byte = mod_init_map.first_wmem_index * PSIZE;

  iprintl(LVL_TEST, "Restoring memory map starting %lx",
          mod_init_map.addr_start);
  // If the whole map was originally paged, and is more than 60% dirty, we just
  // restore it in one write operation
  if ((mod_init_map.w == 1 || mod_init_map.r == 1) &&
      (mod_init_map.soft_dirty_count + mod_init_map.freed_count) >=
          0.6 * mod_init_map.total_num_pages &&
      mod_init_map.total_num_paged_pages == mod_init_map.total_num_pages) {
    iprintl(LVL_TEST, "Restoring whole map memory of %lx",
            mod_init_map.addr_start);
    iprintl(LVL_TEST, "Restoring whole map memory of %lx from wmem %lx",
            mod_init_map.addr_start, init_mem_start_byte);
    r = 0;
    int new_r = 0;
    do {
      new_r = pwrite(mem_fd, initial_state->wmem + init_mem_start_byte + r,
                     PSIZE * mod_init_map.total_num_pages - r,
                     mod_init_map.addr_start + r);
      r += new_r;
    } while (r != mod_init_map.total_num_pages * PSIZE && new_r > 0);

    if (r != PSIZE * mod_init_map.total_num_pages) {
      iprintl(LVL_EXP,
              "PWRITE ERROR: %s pid:%d, page_start %lx, map_start %lx, "
              "wmem_addr %lu, wmem_size %lu, r = %d, new_r = %d",
              strerror(errno), initial_state->pid, mod_init_map.addr_start,
              mod_init_map.addr_start, init_mem_start_byte / PSIZE,
              PSIZE * initial_state->writable_pages_count, r, new_r);
      assert(0);
    }

    (*count_restored) = mod_init_map.total_num_pages;

    jprint(LVL_TEST, "number of restored pages for map starting at %lx is %ld",
           mod_init_map.addr_start, *count_restored);
    return;
  }

  int paged_index = -1;
  int count_madvise = 0;
  uint64_t madvise_start = 0;
  for (int j = 0; j < mod_init_map.total_num_pages; j++) {
    init_mod_page = initial_state->pages[mod_init_map.first_page_index + j];
    if (init_mod_page.pfn)
      paged_index += 1;
    // we use the same page index because the maps might change due to kernel
    // map merging
    // TODO: ADD CHECK
    final_mod_page = final_state->pages[mod_init_map.first_page_index + j];
    assert(final_mod_page.addr_start == init_mod_page.addr_start);
    if (!init_mod_page.pfn && final_mod_page.pfn) {
      // iprintl(LVL_DBG, "COMP: madvising memory page %lx",
      // final_mod_page.addr_start); pwrite(mem_fd, zeroPage, PSIZE,
      // init_mod_page.addr_start);
      if (!count_madvise)
        madvise_start = init_mod_page.addr_start;
      iprintl(LVL_DBG, "COMP: madvising memory page %p",
              (void *)final_mod_page.addr_start);
      count_madvise++;
      continue;
    }
    if (count_madvise) {
      iprintl(LVL_DBG, "COMP: START madvising memory page %p for %d pages",
              (void *)final_mod_page.addr_start, count_madvise);
      // do_madvise_dontneed(tracked_proc, madvise_start, PSIZE*count_madvise);
      //
      madvises_t madv_tuple = (madvises_t){madvise_start, count_madvise};
      g_array_append_val(madvises_array, madv_tuple);
      iprintl(LVL_DBG, "COMP: DONE madvising memory page %p for %d pages",
              (void *)final_mod_page.addr_start, count_madvise);
      count_madvise = 0;
    }
    if (!final_mod_page.soft_dirty && !final_mod_page.is_freed) {
      iprintl(LVL_DBG, "COMP: memory page %p skipped -- not dirty",
              (void *)final_mod_page.addr_start);
      continue;
    }
    if (!init_mod_page.pfn && !final_mod_page.pfn) {
      iprintl(LVL_DBG, "COMP: memory page %p skipped -- not paged",
              (void *)final_mod_page.addr_start);
      continue;
    }
    // page_offset_in_mem = init_mod_page.addr_start - mod_init_map.addir_start;
    if (paged_index == mod_init_map.total_num_paged_pages) {
      iprintl(LVL_DBG, "COMP: ERROR!");
      continue;
    }
    page_offset_in_mem = PSIZE * paged_index;
    // page_offset_in_mem = init_mod_page.addr_start - mod_init_map.addr_start;
    r = pwrite(mem_fd,
               initial_state->wmem + init_mem_start_byte + page_offset_in_mem,
               PSIZE, init_mod_page.addr_start);
    iprintl(LVL_DBG,
            "COMP: restored memory page %p of map %p, was stored at wmem %p ",
            (void *)init_mod_page.addr_start, (void *)mod_init_map.addr_start,
            (void *)initial_state->wmem + init_mem_start_byte +
                page_offset_in_mem);

    if (r != PSIZE) {
      iprintl(LVL_EXP,
              "PWRITE ERROR: %s pid:%d, page_start %p, map_start %p, wmem_addr "
              "%p, wmem_size %p",
              strerror(errno), initial_state->pid,
              (void *)init_mod_page.addr_start, (void *)mod_init_map.addr_start,
              (void *)init_mem_start_byte + page_offset_in_mem,
              (void *)(PSIZE * initial_state->paged_pages_count));
      assert(0);
    }
    (*count_restored)++;
  }
  if (count_madvise) {
    // do_madvise_dontneed(tracked_proc, madvise_start, PSIZE*count_madvise);
    madvises_t madv_tuple = (madvises_t){madvise_start, count_madvise};
    g_array_append_val(madvises_array, madv_tuple);
    count_madvise = 0;
  }
  jprint(LVL_TEST, "number of restored pages for map starting at %p is %ld",
         (void *)mod_init_map.addr_start, *count_restored);
}

int restore_soft_dirty_memory(int mem_fd, process_mem_diff_t *ps_diff,
                              process_mem_info_t *initial_state,
                              process_mem_info_t *final_state,
                              trackedprocess_t *tracked_proc) {
  XSTATS_INIT_TIMER(madvises);
  XSTATS_INIT_TIMER(restore_mem);
  XSTATS_START_TIMER(restore_mem);
  uint64_t maps_count = ps_diff->modified.maps_count;
  // assert(initial_state->writable_pages_count ==
  // final_state->writable_pages_count);
  jprint(LVL_TEST, "Restoring Original Memory content for pid:%d for %ld maps",
         initial_state->pid, maps_count);

  struct restore_soft_dirty_memory_map_arguments threadFuncArgs[maps_count];
  uint64_t counts_restored[maps_count];
  GArray *madvises_array =
      g_array_sized_new(FALSE, FALSE, 16, sizeof(madvises_t));
  for (uint64_t i = 0; i < maps_count; i++) {
    // TODO: Optimize by checking final map number of soft dirty bits
    counts_restored[i] = 0;

    // print_map(&ps_diff->modified.maps[i]);
    if (!ps_diff->modified.maps[i].dirty ||
        (!ps_diff->modified.maps[i].r && !ps_diff->modified.maps[i].w)) {
      jprint(LVL_DBG, "COMP: SKIPPED!");
      continue;
    }
    threadFuncArgs[i] = (struct restore_soft_dirty_memory_map_arguments){
        mem_fd,
        ps_diff,
        initial_state,
        final_state,
        &ps_diff->modified.maps[i],
        &counts_restored[i],
        tracked_proc,
        madvises_array};
#if CONCURRENCY > 1
    thpool_add_work(tracked_proc->thpool,
                    (void *)&restore_soft_dirty_memory_map, &threadFuncArgs[i]);
#else
    restore_soft_dirty_memory_map(&threadFuncArgs[i]);
#endif
  }

#if CONCURRENCY > 1
  thpool_wait(tracked_proc->thpool);
#endif

  for (uint64_t i = 0; i < madvises_array->len; i++) {
    madvises_t madv_tuple = g_array_index(madvises_array, madvises_t, i);
    XSTATS_START_TIMER(madvises);
    do_madvise_dontneed(tracked_proc, madv_tuple.page_start,
                        PSIZE * madv_tuple.num_pages);
    XSTATS_END_TIMER(madvises);
  }
  g_array_free(madvises_array, 1);

  uint64_t count_restored = 0;
  for (uint64_t i = 0; i < maps_count; i++) {
    count_restored += counts_restored[i];
  }
  XSTATS_END_TIMER(restore_mem);
  XSTATS_ADD_COUNT_POINT(num_restored_pages, count_restored);
  jprint(LVL_TEST, "We restored %ld modified pages", count_restored);
  assert(count_restored <= initial_state->pages_count);
  return count_restored;
}

struct restore_memory_shard_arguments {
  int mem_fd;
  process_mem_diff_t *p_diff;
  process_mem_info_t *initial_state;
  uint64_t start_index;
  uint64_t end_index;
  uint64_t *count_restored;
};

void restore_memory_shard(
    struct restore_memory_shard_arguments *threadFuncArgs) {

  int mem_fd = threadFuncArgs->mem_fd;
  process_mem_diff_t *p_diff = threadFuncArgs->p_diff;
  process_mem_info_t *initial_state = threadFuncArgs->initial_state;
  uint64_t start_index = threadFuncArgs->start_index;
  uint64_t end_index = threadFuncArgs->end_index;
  uint64_t *count_restored = threadFuncArgs->count_restored;

  page_t mod_page;
  map_t mod_map;
  map_t init_map;
  uint64_t init_map_index;
  uint64_t init_mem_start_byte;
  uint64_t page_offset_in_mem;
  int r;
  for (uint64_t i = start_index; i < end_index; i++) {
    mod_page = p_diff->modified.pages[i];
    mod_map = p_diff->modified.maps[mod_page.map_index];

    if (mod_map.w && mod_page.soft_dirty) {
      // jprint(LVL_INFO, "RESTORING MEMORY PAGE: %lu", mod_page.addr_start);
      // get the corresponding map in the initial state
      // TODO:
      init_map_index = 0;
      // the map must exist
      if (init_map_index == -1)
        jprint(LVL_EXP, "ERROR: starting with address %p not identified",
               (void *)mod_map.addr_start);
      assert(init_map_index != -1);
      init_map = initial_state->maps[init_map_index];
      assert(init_map.first_wmem_index > -1);
      init_mem_start_byte = init_map.first_wmem_index * PSIZE;
      page_offset_in_mem = mod_page.addr_start - init_map.addr_start;

      r = pwrite(mem_fd,
                 initial_state->wmem + init_mem_start_byte + page_offset_in_mem,
                 PSIZE, mod_page.addr_start);
      if (r != PSIZE) {
        iprintl(LVL_EXP,
                "PWRITE ERROR: %s pid:%d, page_start %p, map_start %p, "
                "wmem_addr %p, wmem_size %p",
                strerror(errno), initial_state->pid,
                (void *)mod_page.addr_start, (void *)init_map.addr_start,
                (void *)init_mem_start_byte + page_offset_in_mem,
                (void *)(PSIZE * initial_state->writable_pages_count));
      } else {
#if XDEBUG_LVL <= LVL_DBG
        iprintl(LVL_DBG,
                "PWRITE SUCCESS: %s pid:%d, page_start %p, map_start %p, "
                "wmem_addr %p, wmem_size %p",
                strerror(errno), initial_state->pid,
                (void *)mod_page.addr_start, (void *)init_map.addr_start,
                (void *)init_mem_start_byte + page_offset_in_mem,
                (void *)(PSIZE * initial_state->writable_pages_count));
#endif
        (*count_restored) += 1;
      }
    }
  }
}

int restore_process_sd(trackedprocess_t *tracked_proc) {
  XSTATS_INIT_TIMER(restore);
  XSTATS_INIT_TIMER(syscalls);
  XSTATS_START_TIMER(restore);

  int total_sd = 0;

  process_mem_info_t *initial_state = tracked_proc->p_checkpointed_state;

  if (tracked_proc->p_curr_state) {
    tracked_proc->p_curr_state =
        free_process_mem_info(tracked_proc->p_curr_state, 1);
  }

  tracked_proc->p_curr_state = capture_process_state(tracked_proc, NO_PAGES);
  process_mem_diff_t *p_diff = diff_process_states(
      tracked_proc->p_checkpointed_state, tracked_proc->p_curr_state, NO_PAGES);
#if XDEBUG_LVL <= LVL_TEST
  print_diff_stats(p_diff);
#endif
  assert(initial_state->pid == tracked_proc->p_curr_state->pid);

  XSTATS_START_TIMER(syscalls);
  remove_added_memory(tracked_proc, p_diff);
  restore_removed_memory(tracked_proc, p_diff);
  restore_memory_protection(tracked_proc, p_diff);
  restore_brk(tracked_proc, p_diff);
  XSTATS_END_TIMER(syscalls);

  zero_stack(tracked_proc->mem_fd, tracked_proc);

  jprint(LVL_TEST, "Now that we restored the mappings, and the program break, "
                   "we need to restore their contents too");
  //	do{
  free_process_mem_diff(p_diff);
  tracked_proc->p_curr_state =
      free_process_mem_info(tracked_proc->p_curr_state, 1);
  tracked_proc->p_curr_state = capture_process_state(tracked_proc, WITH_PAGES);
  p_diff = diff_process_states(tracked_proc->p_checkpointed_state,
                               tracked_proc->p_curr_state, WITH_PAGES);

#if XDEBUG_LVL <= LVL_TEST
  print_diff_stats(p_diff);
#endif

  restore_soft_dirty_memory(tracked_proc->mem_fd, p_diff, initial_state,
                            tracked_proc->p_curr_state, tracked_proc);
  free_process_mem_diff(p_diff);

#if XDEBUG_LVL <= LVL_TEST
  free_process_mem_info(tracked_proc->p_curr_state, 1);
  tracked_proc->p_curr_state = capture_process_state(tracked_proc, WITH_PAGES);
#endif

  aggregate_dirty_stats(tracked_proc);

#if XDEBUG_LVL <= LVL_TEST
  jprint(LVL_EXP, "FINAL CHECK BEFORE LEAVING:");
  diff_memory_verify_integrity(tracked_proc);
#endif

  // restoration is done, we need to clear soft dirty bits
  // This has to be done here because the tests populate the memory which
  // somehow dirties everything
  cr_ops.clear_dirty_bits(tracked_proc);

#if XDEBUG_LVL <= LVL_TEST
  total_sd = 0;
  process_mem_info_t *tmp_state =
      capture_process_state(tracked_proc, WITH_PAGES);
  for (uint64_t i = 0; i < tmp_state->maps_count; i++) {
    total_sd += tmp_state->maps[i].soft_dirty_count;
  }
  free_process_mem_info(tmp_state, 1);

  assert(total_sd == 0);
#endif

  restore_threads_registers(tracked_proc);
  XSTATS_END_TIMER(restore);
  return 0;
}

void reset_map_metadata(map_t *map) {
  map->first_page_index = -1;
  map->total_num_pages = 0;
  map->populated_num_pages = 0;
  map->soft_dirty_count = 0;
  map->has_freed = 0;
  map->soft_dirty_paged_count = 0;
  map->uffd_dirty_count = 0;
}

void diff_process_maps(process_mem_diff_t *ps_diff,
                       process_mem_info_t *initial_state,
                       process_mem_info_t *final_state) {
  init_process_mem_info(&(ps_diff->added), initial_state->pid);
  init_process_mem_info(&(ps_diff->removed), initial_state->pid);
  init_process_mem_info(&(ps_diff->modified), initial_state->pid);
  init_process_mem_info(&(ps_diff->modified_protection), initial_state->pid);

  map_t map;
  uint64_t index_initial_state = 0;
  uint64_t index_final_state = 0;
  int r;
  do {
    r = diff_process_maps_iterator(&index_initial_state, &index_final_state,
                                   initial_state, final_state);
    switch (r) {
      /*
                      case IDENTICAL:
                              map = initial_state->maps[index_initial_state -
         1]; map.dirty = 1; map.uffd_dirty_count = map.total_num_pages;
                              map.soft_dirty_count = map.total_num_pages;
                              add_to_maps(&(ps_diff->modified), map);
                              if ((map.name[0] == '[' && map.name[1] == 'h') ||
                                              (map.name[0] == '[' && map.name[1]
         == 's') )//|| map.w == 1)
                              {
                                      // we want to restore all of the heap and
         stack
                                      //reset_map_metadata(&map);
                                      map.dirty = 1;
                                      add_to_maps(&(ps_diff->modified), map);
                              }
                              break;
      */
    case MODIFIED:
      // We add the original map to make restoring the memory contents straight
      // forward -- restore_soft_dirty_memory
      map = initial_state->maps[index_initial_state - 1];
      // reset_map_metadata(&map);
      map.dirty = 1;
      map.soft_dirty_count =
          final_state->maps[index_final_state - 1].soft_dirty_count;
      map.freed_count = final_state->maps[index_final_state - 1].freed_count;
      // map.uffd_dirty_count = map.total_num_pages;
      // map.soft_dirty_count = map.total_num_pages;
      add_to_maps(&(ps_diff->modified), map);
#if XDEBUG_LVL <= LVL_TEST
      jprint(LVL_TEST, "Maps are modified");
      print_maps_index(initial_state, index_initial_state - 1);
      print_maps_index(final_state, index_final_state - 1);
#endif
      break;
    case MODIFIED_PROTECTION:
      // We add the original map to make restoring the memory contents straight
      // forward -- restore_soft_dirty_memory
      map = initial_state->maps[index_initial_state - 1];
      map.dirty = 1;
      map.soft_dirty_count =
          final_state->maps[index_final_state - 1].soft_dirty_count;
      add_to_maps(&(ps_diff->modified_protection), map);
      add_to_maps(&(ps_diff->modified), map);
#if XDEBUG_LVL <= LVL_TEST
      jprint(LVL_TEST, "Pages with modified protection");
      print_maps_index(initial_state, index_initial_state - 1);
      print_maps_index(final_state, index_final_state - 1);
#endif
      break;
    case ADDED_AND_REMOVED:
      map = final_state->maps[index_final_state - 1];
      reset_map_metadata(&map);
      add_to_maps(&(ps_diff->added), map);

      map = initial_state->maps[index_initial_state - 1];
      // We add the original map to make restoring the memory contents straight
      // forward -- restore_soft_dirty_memory
      map.dirty = 1;
      map.uffd_dirty_count = map.total_num_pages;
      map.soft_dirty_count = map.total_num_pages;
      add_to_maps(&(ps_diff->modified), map);
      reset_map_metadata(&map);
      add_to_maps(&(ps_diff->removed), map);
      break;
    case ADDED:
      /* TODO: we need to check that the added maps (before memory content
       * restoration) are due to kernem vma_merge and not somthing else*/
      map = final_state->maps[index_final_state - 1];
      reset_map_metadata(&map);
      add_to_maps(&(ps_diff->added), map);
      break;
    case REMOVED:
      map = initial_state->maps[index_initial_state - 1];
      /*Removed maps need to be also marked as modified because the kernel
       might have merged their vma_structs, and
       we still need to restore their content anyway*/
      // We restore the full map if it is dirty and its count is 0.
      map.dirty = 1;
      map.uffd_dirty_count = map.total_num_pages;
      map.soft_dirty_count = map.total_num_pages;
      add_to_maps(&(ps_diff->modified), map);
      reset_map_metadata(&map);
      add_to_maps(&(ps_diff->removed), map);
      break;
    default:
      break;
    }
    if (index_initial_state > initial_state->maps_count ||
        index_final_state > final_state->maps_count) {
      iprintl(LVL_EXP, "ERROR: We went beyond the maps memory");
      exit(-1);
    }

  } while (r != DONE);
}

/*
 * The region is modified if:
 *  -	same start address but any other attribute have changed
 * A region is added if it didn't exist in the initial state
 * A region is removed if it doesn't exist in the final state
 */
process_mem_diff_t *diff_process_states(process_mem_info_t *initial_state,
                                        process_mem_info_t *final_state,
                                        bool diff_pages) {

  XSTATS_INIT_TIMER(diff);
  XSTATS_START_TIMER(diff);

  process_mem_diff_t *ps_diff =
      (process_mem_diff_t *)calloc(1, sizeof(process_mem_diff_t));

  assert(initial_state->pid == final_state->pid);

  diff_process_maps(ps_diff, initial_state, final_state);

#if XDEBUG_LVL <= LVL_TEST
  jprint(LVL_EXP, "Modified maps");
  print_maps(&ps_diff->modified);
  jprint(LVL_EXP, "Modified protection maps");
  print_maps(&ps_diff->modified_protection);
  jprint(LVL_EXP, "Added maps");
  print_maps(&ps_diff->added);
  jprint(LVL_EXP, "Removed maps");
  print_maps(&ps_diff->removed);
  jprint(LVL_EXP, "End of maps section");
#endif

  XSTATS_END_TIMER(diff);
  return ps_diff;
}

process_mem_info_t *checkpoint_process_sd(trackedprocess_t *tracked_proc) {
  XSTATS_INIT_TIMER(snapshot);
  XSTATS_INIT_TIMER(store_mem);
  XSTATS_START_TIMER(snapshot);

  process_mem_info_t *process_state;

  cr_ops.clear_dirty_bits(tracked_proc);
  process_state = capture_process_state(tracked_proc, WITH_PAGES);

  tracked_proc->p_checkpointed_state = process_state;

  // get stack rlimit
  do_getrlimit_stack(tracked_proc, &(tracked_proc->rlim)); // TODO
  jprint(LVL_TEST, "rlimits: %lx, %lx", tracked_proc->rlim.rlim_cur,
         tracked_proc->rlim.rlim_max);

  XSTATS_START_TIMER(store_mem);
  bool store_wpages_only = 0;
  uint64_t num_pages_to_store = process_state->paged_pages_count;
  jprint(LVL_DBG, "COMP: STORAGE SIZE #%lu (%lu bytes)", num_pages_to_store,
         PSIZE * num_pages_to_store);
  // uint64_t num_pages_to_store =
  //	(store_wpages_only) ? process_state->writable_pages_count :
  //process_state->pages_count;
  init_memory_copy_info(&(process_state->wmem), num_pages_to_store);
  // In kernel 5.7+, it seems that copying the memory set the soft-dirt bit on,
  // WHY??!!!
  populate_memory_copy_info(tracked_proc->mem_fd, process_state, tracked_proc,
                            store_wpages_only);
  XSTATS_END_TIMER(store_mem);

  cr_ops.clear_dirty_bits(tracked_proc);

#if XDEBUG_LVL <= LVL_TEST
  process_mem_info_t *tmp_state =
      capture_process_state(tracked_proc, WITH_PAGES);

#if XDEBUG_LVL <= LVL_TEST
  print_sd_bits_externally(tracked_proc->pid);
#endif

  for (int i = 0; i < tmp_state->maps_count; i++) {
    assert(tmp_state->maps[i].soft_dirty_count == 0);
  }
  free_process_mem_info(tmp_state, 1);
#endif

  jprint(LVL_TEST,
         "From %ld maps, we know of %lu pages (of which %ld are writable)",
         process_state->maps_count, process_state->pages_count,
         process_state->writable_pages_count);

  // Store registers values last thing, as it is set just before yielding to the
  // process
  store_threads_registers(tracked_proc);

#if XDEBUG_LVL <= LVL_TEST
  diff_memory_verify_integrity(tracked_proc);
#endif

  XSTATS_END_TIMER(snapshot);
  XSTATS_ADD_COUNT_POINT(num_paged_pages, process_state->paged_pages_count);
  return tracked_proc->p_checkpointed_state;
}

/* Alllocates and returns a pointer to a process_mem_info struct
 *
 *
 *
 */
process_mem_info_t *capture_process_state(trackedprocess_t *tracked_proc,
                                          bool capture_pages) {

  XSTATS_INIT_TIMER(read_map);
  XSTATS_INIT_TIMER(read_pages);

  process_mem_info_t *process_state =
      (process_mem_info_t *)calloc(1, sizeof(process_mem_info_t));

  uint64_t npages_in_maps = -1;

  XSTATS_START_TIMER(read_map);
  init_process_mem_info(process_state, tracked_proc->pid);
  npages_in_maps = read_maps(tracked_proc->maps_fd, process_state);
  XSTATS_END_TIMER(read_map);

  if (capture_pages) {
    XSTATS_START_TIMER(read_pages);
    init_pages_info(&(process_state->pages), process_state->pages_max_capacity);
    populate_page_info(tracked_proc->pagemap_fd, process_state, tracked_proc);
    XSTATS_END_TIMER(read_pages);

    assert(npages_in_maps == process_state->pages_count);
    XSTATS_ADD_COUNT_POINT(num_maps, process_state->maps_count);
    XSTATS_ADD_COUNT_POINT(num_pages, process_state->pages_count);
    XSTATS_ADD_COUNT_POINT(num_w_pages, process_state->writable_pages_count);
  }
#if XDEBUG_LVL <= LVL_TEST
  print_maps(process_state);
#endif

  return process_state;
}

int diff(trackedprocess_t *tracked_proc) {
  ptrace_attach(tracked_proc);

  if (!tracked_proc->p_checkpointed_state) {
    jprint(LVL_EXP, "We are going to read the initial snapshot from disk");
    tracked_proc->p_checkpointed_state =
        read_process_mem_info_t_from_file(tracked_proc->dump_file_name);
  }

  if (tracked_proc->p_curr_state) {
    tracked_proc->p_curr_state =
        free_process_mem_info(tracked_proc->p_curr_state, 1);
  }

  tracked_proc->p_curr_state = capture_process_state(tracked_proc, NO_PAGES);

  process_mem_diff_t *p_diff = diff_process_states(
      tracked_proc->p_checkpointed_state, tracked_proc->p_curr_state, NO_PAGES);
  print_diff_stats(p_diff);
  free_process_mem_diff(p_diff);

  fflush(stdout);
  ptrace_detach(tracked_proc);

  return 0;
}

int restore(trackedprocess_t *tracked_proc) {
  XSTATS_INIT_TIMER(all_inclusive_iter_time);
  XSTATS_START_TIMER(all_inclusive_iter_time);
  if (!tracked_proc->p_checkpointed_state) {
    jprint(LVL_EXP, "We are going to read the initial snapshot from disk");
    tracked_proc->p_checkpointed_state =
        read_process_mem_info_t_from_file(tracked_proc->dump_file_name);
  }

  ptrace_attach(tracked_proc);

  cr_ops.restore_process(tracked_proc);

  ptrace_detach(tracked_proc);
  XSTATS_END_TIMER(all_inclusive_iter_time);
  return 0;
}

int checkpoint(trackedprocess_t *tracked_proc) {
  // jprint(LVL_EXP, "STARTING CHKPT");
  XSTATS_INIT_TIMER(all_inclusive_checkpoint_time);
  XSTATS_START_TIMER(all_inclusive_checkpoint_time);
  // seize first time
  if (!num_checkpoints++)
    seize_process(tracked_proc, 1);
  interrupt_process(tracked_proc);

  if (tracked_proc->p_checkpointed_state) {
    tracked_proc->p_checkpointed_state =
        free_process_mem_info(tracked_proc->p_checkpointed_state, 1);
  }
  cr_ops.checkpoint_process(tracked_proc);
  // jprint(LVL_EXP, "DONE CHKPT");
  for (int i = 0; i < tracked_proc->p_checkpointed_state->maps_count; i++) {
    assert(tracked_proc->p_checkpointed_state->maps[i].soft_dirty_count == 0);
  }
  fflush(stdout);
  ptrace_detach(tracked_proc);

  XSTATS_END_TIMER(all_inclusive_checkpoint_time);
  return 0;
}

/**
 * For debugging purposes
 */

void dump_process_mem_info_t_to_file(process_mem_info_t *ps, char *fname) {
  FILE *outfile;
  outfile = fopen(fname, "w");
  if (outfile == NULL) {
    fprintf(stderr, "\nError opening file\n");
    exit(1);
  }
  // Write metadata, pointers will be garbage anyway
  fwrite(ps, sizeof(process_mem_info_t), 1, outfile);
  fwrite(ps->maps, sizeof(map_t), ps->maps_count, outfile);
  fwrite(ps->pages, sizeof(page_t), ps->pages_count, outfile);
  fwrite(ps->wmem, sizeof(char), PSIZE * ps->writable_pages_count, outfile);

  fclose(outfile);
}

int dump_process_state_readable(trackedprocess_t *tracked_proc, char *fname) {
  FILE *outfile;
  char of_maps[512];
  char of_pages[512];
  char of_mem[512];
  sprintf(of_maps, "%s_dump_%s", fname, "maps");
  outfile = fopen(of_maps, "w");
  if (outfile == NULL) {
    fprintf(stderr, "\nError opening file %s\n", of_maps);
    exit(1);
  }

  process_mem_info_t *ps_mem =
      (process_mem_info_t *)calloc(1, sizeof(process_mem_info_t));
  init_process_mem_info(ps_mem, tracked_proc->pid);
  read_maps(tracked_proc->maps_fd, ps_mem);

  jprint(LVL_EXP, "Dumping %ld memory maps to file %s", ps_mem->maps_count,
         of_maps);
  for (int i = 0; i < ps_mem->maps_count; i++) {
    map_t tmp_map = ps_mem->maps[i];
    fprintf(outfile, "%lx-%lx %d%d%d%d %u %u:%u %u %lu %lu %s\n",
            tmp_map.addr_start, tmp_map.addr_end, tmp_map.r, tmp_map.w,
            tmp_map.x, tmp_map.p, tmp_map.offset, tmp_map.dev_major,
            tmp_map.dev_minor, tmp_map.inode, tmp_map.total_num_pages,
            tmp_map.soft_dirty_count, tmp_map.name);
  }
  fclose(outfile);

  sprintf(of_pages, "%s_dump_%s", fname, "pages");
  outfile = fopen(of_pages, "w");
  if (outfile == NULL) {
    fprintf(stderr, "\nError opening file\n");
    exit(1);
  }

  init_pages_info(&(ps_mem->pages), ps_mem->pages_max_capacity);
  populate_page_info(tracked_proc->pagemap_fd, ps_mem, tracked_proc);

  jprint(LVL_EXP, "Dumping %ld memory pages metadata to file %s",
         ps_mem->pages_count, of_pages);
  for (int i = 0; i < ps_mem->pages_count; i++) {
    page_t page = ps_mem->pages[i];
    fprintf(outfile,
            "start_addr %lx : pfn %lx soft-dirty %d file/shared %d "
            "exclusive %d swapped %d present %d \n",
            (uint64_t)page.addr_start, (uint64_t)page.pfn,
            (uint32_t)page.soft_dirty, (uint32_t)page.file_or_anon,
            (uint32_t)page.exclusive, (uint32_t)page.swapped,
            (uint32_t)page.present);
  }
  fclose(outfile);

  sprintf(of_mem, "%s_dump_%s", fname, "mem");
  outfile = fopen(of_mem, "w");
  if (outfile == NULL) {
    fprintf(stderr, "\nError opening file\n");
    exit(1);
  }
  bool store_wpages_only = 0;
  uint64_t num_pages_to_store =
      (store_wpages_only) ? ps_mem->writable_pages_count : ps_mem->pages_count;
  init_memory_copy_info(&(ps_mem->wmem), num_pages_to_store);
  populate_memory_copy_info(tracked_proc->mem_fd, ps_mem, tracked_proc,
                            store_wpages_only);
  jprint(LVL_TEST, "Dumping %ld pages of memory content to file %s",
         ps_mem->pages_count, of_mem);
  fwrite(ps_mem->wmem, sizeof(char), PSIZE * ps_mem->pages_count, outfile);
  fclose(outfile);

  free_process_mem_info(ps_mem, 1);

  return 0;
}

// allocates memory for ps and reads the dump from disk
process_mem_info_t *read_process_mem_info_t_from_file(char *fname) {
  FILE *infile;
  infile = fopen(fname, "r");
  if (infile == NULL) {
    fprintf(stderr, "\nError opening file\n");
    exit(1);
  }
  process_mem_info_t *ps =
      (process_mem_info_t *)calloc(1, sizeof(process_mem_info_t));

  // read metadata, pointers will be garbage anyway
  fread(ps, sizeof(process_mem_info_t), 1, infile);

  // allocate memory for maps and pages
  init_maps(&(ps->maps), ps->maps_count);
  init_pages_info(&(ps->pages), ps->pages_max_capacity);
  init_memory_copy_info(&(ps->wmem), ps->writable_pages_count);

  // read maps and pages
  fread(ps->maps, sizeof(map_t), ps->maps_count, infile);
  fread(ps->pages, sizeof(page_t), ps->pages_count, infile);
  fread(ps->wmem, sizeof(char), PSIZE * ps->writable_pages_count, infile);

  fclose(infile);
  jprint(
      LVL_INFO,
      "Process metadata read, will be reading %lu maps (%p) and %lu pages (%p)",
      ps->maps_count, ps->maps, ps->pages_count, ps->pages);
  // initialize maps and pages hashmaps
  return ps;
}
