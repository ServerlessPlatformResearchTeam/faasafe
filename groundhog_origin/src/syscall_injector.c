// reusing some logic from https://github.com/emptymonkey/ptrace_do
// http://man7.org/linux/man-pages/man2/syscall.2.html
// args:        x86-64        rdi   rsi   rdx   r10   r8    r9    -
// syscall rax, retval1 rax, retval2  rdx

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>

#include "syscall_injector.h"
#include <execinfo.h>

#define SYSCALL 0x050f
#define XSAVE 0xae0f
#define XRSTOR 0xae0f
#define SYSCALL_MASK 0x000000000000ffff
#define SIZEOF_SYSCALL 2

/* 1) Store registers
 * 2) Store the RIP instruction
 * 3) Inject the syscall Instruction
 * 4) Set the syscall registers
 * 5) Run the process till syscall happens, and wait for it
 * 6) Restore the RIP Instruction
 * 7) Restore the registers
 */

unsigned long do_clean_syscall(trackedprocess_t *tracked_proc,
                               unsigned long rax, unsigned long rdi,
                               unsigned long rsi, unsigned long rdx,
                               unsigned long r10, unsigned long r8,
                               unsigned long r9) {

  struct user_regs_struct original_regs;
  struct user_regs_struct syscall_regs;
  unsigned long peekdata;
  unsigned long syscall_address;
  long long syscall_ret;
  int status;

  // 1) store registers
  if (ptrace(PTRACE_GETREGS, tracked_proc->pid, NULL, &original_regs) < 0)
    iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);

  // 2) store the RIP instruction
  errno = 0;
  peekdata =
      ptrace(PTRACE_PEEKTEXT, tracked_proc->pid, original_regs.rip, NULL);
  if (errno) {
    jprint(LVL_EXP, "BUG");
    goto SYSCALL_FAILED;
  }
  // 3) Inject the syscall Instruction
  errno = 0;
  syscall_address = original_regs.rip;
  if (ptrace(PTRACE_POKETEXT, tracked_proc->pid, syscall_address, SYSCALL) <
      0) {
    iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);
  }

  // 4) set the syscall registers
  memcpy(&syscall_regs, &original_regs, sizeof(syscall_regs));

  syscall_regs.rax = rax;
  syscall_regs.orig_rax = rax;
  syscall_regs.rdi = rdi;
  syscall_regs.rsi = rsi;
  syscall_regs.rdx = rdx;
  syscall_regs.r10 = r10;
  syscall_regs.r8 = r8;
  syscall_regs.r9 = r9;
  syscall_regs.rip = syscall_address;

  if (ptrace(PTRACE_SETREGS, tracked_proc->pid, NULL, &syscall_regs) < 0) {
    iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);
    print_registers(syscall_regs);
  }

  int iter = 0;
  do {
    //  5) Run the process till syscall happens, and wait for it
  DO_SYSCALL:
    status = 0;
    if (ptrace(PTRACE_SINGLESTEP, tracked_proc->pid, NULL, NULL) < 0)
      iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno),
              tracked_proc->pid);

    if (waitpid(tracked_proc->pid, &status, 0) < 1) {
      iprintl(LVL_EXP, "PTRACE Wait Status ERROR: status %d, err: %s pid:%d",
              status, strerror(errno), tracked_proc->pid);
    };

    if (status) {
      if (WIFEXITED(status) || WIFSIGNALED(status) || WIFCONTINUED(status))
        goto SYSCALL_FAILED;
    }

    // 5+) check if there was an error with the syscall
    if (ptrace(PTRACE_GETREGS, tracked_proc->pid, NULL, &syscall_regs) < 0) {
      iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno),
              tracked_proc->pid);
      break;
    }

    if (syscall_regs.rax > -4096UL) {
    SYSCALL_FAILED:
      iprintl(LVL_EXP, "SYSCALL ERROR: ret = -1,  %llx,  %s , pid:%d",
              syscall_regs.orig_rax, strerror(-syscall_regs.rax),
              tracked_proc->pid);
      jprint(LVL_EXP, "Killing child, syscall failed");
      print_registers(syscall_regs);
      kill(tracked_proc->pid, SIGKILL);
      fflush(stdout);
      exit(-1);
    }
    iter += 1;
  } while (syscall_regs.rax == rax);
  jprint(LVL_TEST, "SYSCALL SUCCEDED from iteration number %d", iter);

  // 6) Restore the Instruction

  if (ptrace(PTRACE_POKETEXT, tracked_proc->pid, syscall_address, peekdata) <
      0) {
    iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);
  }

  // 7) Restore the registers
  if (ptrace(PTRACE_SETREGS, tracked_proc->pid, NULL, &original_regs) < 0) {
    iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);
  }

  return syscall_regs.rax;
}

void do_brk(trackedprocess_t *tracked_proc, uint64_t end_addr) {
  uint64_t r =
      do_clean_syscall(tracked_proc, __NR_brk, end_addr, 0, 0, 0, 0, 0);
  if (r != end_addr) {
    iprintl(LVL_EXP, "ERROR: expected brk %lx, but got %lx instead", end_addr,
            r);
    assert(r == end_addr);
  }
}

void do_mprotect(trackedprocess_t *tracked_proc, uint64_t start_addr,
                 uint64_t size, int prot_flags) {
  assert(do_clean_syscall(tracked_proc, __NR_mprotect, start_addr, size,
                          prot_flags, 0, 0, 0) == 0);
}

void do_mmap(trackedprocess_t *tracked_proc, uint64_t start_addr, uint64_t size,
             int prot_flags, int map_flags, int fd) {
  if (fd == -1)
    map_flags |= MAP_ANONYMOUS;
  assert(do_clean_syscall(tracked_proc, __NR_mmap, start_addr, size, prot_flags,
                          map_flags, (int)fd, 0) == start_addr);
}

void do_munmap(trackedprocess_t *tracked_proc, uint64_t start_addr,
               uint64_t size) {
  assert(do_clean_syscall(tracked_proc, __NR_munmap, start_addr, size, 0, 0, 0,
                          0) == 0);
}

void do_madvise_dontneed(trackedprocess_t *tracked_proc, uint64_t start_addr,
                         uint64_t size) {
  assert(do_clean_syscall(tracked_proc, __NR_madvise, start_addr, size,
                          MADV_DONTNEED, 0, 0, 0) == 0);
}

void do_mremap(trackedprocess_t *tracked_proc, uint64_t old_start_addr,
               uint64_t old_size, uint64_t start_addr, uint64_t size,
               int map_flags) {
  map_flags |= MREMAP_FIXED | MREMAP_MAYMOVE;
  assert(do_clean_syscall(tracked_proc, __NR_mremap, old_start_addr, old_size,
                          size, map_flags, start_addr, 0) == start_addr);
}

void do_getrlimit_stack(trackedprocess_t *tracked_proc, struct rlimit *rlim) {
  if (prlimit(tracked_proc->pid, RLIMIT_STACK, NULL, rlim) != 0)
    iprintl(LVL_EXP, "ERROR: RLIMIT_STACK %s", strerror(errno));
}

void do_resetrlimit_stack(trackedprocess_t *tracked_proc) {
  if (prlimit(tracked_proc->pid, RLIMIT_STACK, &(tracked_proc->rlim), NULL) !=
      0)
    iprintl(LVL_EXP, "ERROR: RLIMIT_STACK %s", strerror(errno));
}

uint64_t do_fork(trackedprocess_t *tracked_proc) {
  return do_clean_syscall(tracked_proc, __NR_fork, 0, 0, 0, 0, 0, 0);
}
