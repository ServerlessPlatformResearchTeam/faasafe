/**
 * Datastructur for storing /proc/PID/maps and /proc/PID/pagemap
 * https://github.com/cirosantilli/linux-kernel-module-cheat/blob/25f9913e0c1c5b4a3d350ad14d1de9ac06bfd4be/kernel_module/user/common.h
 * http://www.catb.org/esr/structure-packing/
 */
#ifndef _MAPS_H_
#define _MAPS_H_
#include <linux/userfaultfd.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/user.h> /* for struct user_regs_struct */

#include "config.h"
#include "libs/fpu.h"
#include "libs/thpool.h"

#define PSIZE 4096
#define NAME_SIZE 128
#define MAX_THREADS 1024

#define NO_PAGES 0
#define WITH_PAGES 1

#define SYSCALL 0x050f
#define SYSCALL_MASK 0x000000000000ffff
#define SIZEOF_SYSCALL 2

#define GET_PAGE_PFN(D) (D & (((uint64_t)1 << 54) - 1))
#define IS_PAGE_PRESENT(D) ((D >> 63) & 1U)
#define IS_PAGE_IN_SWAP(D) ((D >> 62) & 1U)
#define IS_PAGE_FILE_OR_ANON(D) ((D >> 61) & 1U)
#define IS_PAGE_EXCLUSIVE(D) ((D >> 56) & 1U)
#define IS_PAGE_SOFT_DIRTY(D) ((D >> 55) & 1U)
#define IS_PAGE_FREED(D)                                                       \
  (!IS_PAGE_PRESENT(D) && !IS_PAGE_IN_SWAP(D) && !IS_PAGE_EXCLUSIVE(D) &&      \
   !IS_PAGE_SOFT_DIRTY(D))
#define GET_NUM_PAGES_IN_MAP(M) ((M->addr_end - M->addr_start) / PSIZE)
#define GET_NUM_PAGES(START_ADDR, END_ADDR) ((END_ADDR - START_ADDR) / PSIZE)

#define IS_MAP_SHMEM(map) (!memcmp((map)->name, "/dev/shm/", 9))
#define IS_MAP_WRITEABLE_ANON(map)                                             \
  ((map)->w && ((map)->name[0] != '/')) // || IS_MAP_SHMEM(map)))
#define IS_MAP_WRITEABLE_NON_ANON(map) ((map)->w && (map)->name[0] == '/')
#define IS_MAP_PRIV_NON_ANON(map)                                              \
  ((map)->p && ((map)->w || (map)->r) && (map)->name[0] == '/')
//#define IS_MAP_PRIV_NON_ANON(map) ( (map)->p && ((map)->w || (map)->r) &&
//(map)->inode!=0)
#define IS_MAP_ANON(map) ((map)->name[0] != '/')

#ifndef NT_X86_XSTATE
#define NT_X86_XSTATE 0x202 /* x86 extended state using xsave */
#endif

#define NT_PRSTATUS 1 /* x86 extended state using xsave */
#define NT_PRFPREG 2
#define NT_PRPSINFO 3
#define NT_TASKSTRUCT 4
#define NT_AUXV 6

#define INITIAL_NUMBER_OF_REGIONS 100
#define INITIAL_NUMBER_OF_PAGES 256

typedef struct map_t map_t;
typedef struct page_t page_t;

struct map_t {
  // CHECK: map_regions_cmp
  // DONOT ADD FIELDS BELOW
  uint64_t addr_start;
  uint64_t addr_end;

  bool r;
  bool w;
  bool x;
  bool p;

  uint32_t offset;

  uint16_t dev_major;
  uint16_t dev_minor;

  uint32_t inode;
  uint64_t total_num_pages;
  uint64_t soft_dirty_count;

  // DONOT ADD FIELDS ABOVE
  uint64_t freed_count;
  uint64_t uffd_dirty_count;
  uint64_t total_num_paged_pages;
  uint64_t soft_dirty_paged_count;
  uint64_t populated_num_pages;
  uint64_t first_wmem_index;
  uint64_t first_page_index;
  struct uffdio_register uffdio_register;
  char name[NAME_SIZE];
  bool dirty;
  bool has_freed;
};

struct page_t {
  // CHECK: page_entries_cmp
  // DONOT ADD FIELDS BELOW
  uint64_t addr_start;
  uint64_t page_bits;
  // DONOT ADD FIELDS ABOVE
  // Materializing the fields
  uint64_t pfn : 54;
  bool soft_dirty : 1;
  bool file_or_anon : 1;
  bool exclusive : 1;
  bool swapped : 1;
  bool present : 1;
  bool uffd_dirty : 1;
  bool is_freed : 1;
  uint64_t map_index;
};

typedef struct {
  pid_t pid;
  struct user_regs_struct regs;
  uint64_t maps_count;
  uint64_t maps_max_capacity;
  _Atomic uint64_t pages_count;
  _Atomic uint64_t writable_pages_count;
  _Atomic uint64_t paged_pages_count;
  _Atomic uint64_t pages_max_capacity;
  uint64_t max_num_pages_in_mem_region;
  map_t *maps;
  page_t *pages;
  char *wmem;
} process_mem_info_t;

typedef struct {
  process_mem_info_t added;
  process_mem_info_t removed;
  process_mem_info_t modified;
  process_mem_info_t modified_protection;
} process_mem_diff_t;

typedef struct {
  pid_t pid;
  pid_t tids[MAX_THREADS];
  siginfo_t si[MAX_THREADS];
  bool syscall_not_exited;
  struct rlimit rlim;
  struct user_regs_struct tregs[MAX_THREADS];
  struct xsave_struct txsave[MAX_THREADS];
  struct iovec tiov[MAX_THREADS];
  sigset_t tmask[MAX_THREADS];
  size_t tid_count;
  size_t tids_max;
  int keepalive;
  int revived_count;
  int stdin;
  int stdout;
  int stderr;
  int stdcmdin;
  int stdcmdout;
  int clear_fd;
  int pagemap_fd;
  int maps_fd;
  int mem_fd;
  int null_fd;

  process_mem_info_t *p_checkpointed_state;
  process_mem_info_t *p_curr_state;
  threadpool thpool;
  char dump_file_name[BUFSIZ];
  char stats_file_name[BUFSIZ / 4];
} trackedprocess_t;

struct read_range_pagemap_arguments {
  int fd;
  process_mem_info_t *ps;
  map_t *map;
  uint64_t map_index;
};
struct populate_memory_copy_info_arguments {
  int mem_fd;
  process_mem_info_t *ps;
  map_t *curr_map;
};

#endif
