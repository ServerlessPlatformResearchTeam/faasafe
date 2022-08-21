#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/ptrace.h> /* for ptrace() */
#include <sys/types.h>
#include <sys/user.h> /* for struct user_regs_struct */
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

#include "libs/xdebug.h"
#include "proc_cr.h"

unsigned long do_clean_syscall(trackedprocess_t *tracked_proc,
                               unsigned long rax, unsigned long rdi,
                               unsigned long rsi, unsigned long rdx,
                               unsigned long r10, unsigned long r8,
                               unsigned long r9);

void do_brk(trackedprocess_t *tracked_proc, uint64_t end_addr);

void do_mprotect(trackedprocess_t *tracked_proc, uint64_t start_addr,
                 uint64_t size, int prot_flags);
void do_mmap(trackedprocess_t *tracked_proc, uint64_t start_addr, uint64_t size,
             int prot_flags, int map_flags, int fd);
void do_munmap(trackedprocess_t *tracked_proc, uint64_t start_addr,
               uint64_t size);
void do_mremap(trackedprocess_t *tracked_proc, uint64_t old_start_addr,
               uint64_t old_size, uint64_t start_addr, uint64_t size,
               int map_flags);
void do_madvise_dontneed(trackedprocess_t *tracked_proc, uint64_t start_addr,
                         uint64_t size);

void do_getrlimit_stack(trackedprocess_t *tracked_proc, struct rlimit *rlim);
void do_resetrlimit_stack(trackedprocess_t *tracked_proc);
