/*
 * tracing multiple threads:
        https://stackoverflow.com/questions/5477976/how-to-ptrace-a-multi-threaded-application
        https://stackoverflow.com/questions/18577956/how-to-use-ptrace-to-get-a-consistent-view-of-multiple-threads
        https://stackoverflow.com/questions/47658347/wait-for-signal-then-continue-execution
*/

#include "proc_control.h"

int wait_process(const pid_t pid, int *const statusptr) {
  int status;
  pid_t p;

  do {
    status = 0;
    p = waitpid(pid, &status, WUNTRACED | WCONTINUED);
  } while (p == (pid_t)-1 && errno == EINTR);
  if (p != pid)
    return errno = ESRCH;

  if (statusptr)
    *statusptr = status;

  return errno = 0;
}

int continue_process(const pid_t pid, int *const statusptr) {
  int status;
  pid_t p;

  do {

    if (kill(pid, SIGCONT) == -1)
      return errno = ESRCH;

    do {
      status = 0;
      p = waitpid(pid, &status, WUNTRACED | WCONTINUED);
    } while (p == (pid_t)-1 && errno == EINTR);

    if (p != pid)
      return errno = ESRCH;

  } while (WIFSTOPPED(status));

  if (statusptr)
    *statusptr = status;

  return errno = 0;
}

int detach_or_cont_process(trackedprocess_t *tracked_proc, int do_detach) {
  int tid;
  int status;
  XSTATS_INIT_TIMER(detaching);
  XSTATS_START_TIMER(detaching);
  // for (int i = tracked_proc->tid_count-1; i >= 0; i--){
  for (int i = 0; i < tracked_proc->tid_count; i++) {
    tid = tracked_proc->tids[i];
    if (tid < 0)
      continue;

#if XDEBUG_LVL <= LVL_TEST
    unsigned long peekdata =
        ptrace(PTRACE_PEEKTEXT, tid,
               tracked_proc->tregs[i].rip - SIZEOF_SYSCALL, NULL);
    jprint(LVL_TEST,
           "We will continue %d, rip:%llx, our next instruction is: %lx ", tid,
           tracked_proc->tregs[0].rip, SYSCALL_MASK & peekdata);
#endif

    if (do_detach) {
      if (ptrace(PTRACE_DETACH, tid, NULL, NULL) == -1) {
        iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno), tid);
      }
      jprint(LVL_TEST, "DETACHing thread %d", tid);
    } else {
#if STRACE
      if (ptrace(PTRACE_SYSCALL, tid, NULL, NULL) == -1) {
        iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno), tid);
      }
      jprint(LVL_TEST, "Waiting for a SYSCALL on thread %d", tid);
#else
      jprint(LVL_TEST, "We will continue %d", tid);

      if (ptrace(PTRACE_CONT, tid, NULL, NULL) == -1) {
        iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno), tid);
      }
#endif
    }
  }
  XSTATS_END_TIMER(detaching);
}

int seize_process(trackedprocess_t *tracked_proc, int do_seize) {
  pid_t pid = tracked_proc->pid;
  // https://lists.linuxfoundation.org/pipermail/containers/2011-July/027613.html
  char path[PATH_MAX];
  DIR *dir;
  struct dirent *ent;
  int nr_threads = 0;
  XSTATS_INIT_TIMER(seizing);
  XSTATS_START_TIMER(seizing);
  memset(tracked_proc->tids, 0, sizeof(tracked_proc->tids));

  snprintf(path, sizeof(path), "/proc/%d/task", pid);
  dir = opendir(path);
  if (!dir) {
    perror("opendir");
    return -errno;
  }
  while ((ent = readdir(dir))) {
    pid_t tid;
    char *eptr;
    int status;
    siginfo_t si;

    tid = strtoul(ent->d_name, &eptr, 0);
    if (*eptr != '\0')
      continue;

    if (nr_threads >= MAX_THREADS) {
      iprintl(LVL_EXP, "too many threads");
      return -EINVAL;
    }

    if (do_seize) {
      jprint(LVL_TEST, "Seizing %d\n", tid);
      if (ptrace(PTRACE_SEIZE, tid, NULL,
                 PTRACE_O_TRACESYSGOOD | PTRACE_O_EXITKILL)) {
        iprintl(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno), tid);
      }
    }
    tracked_proc->tids[nr_threads++] = tid;
  }
  closedir(dir);
  tracked_proc->tid_count = nr_threads;
  XSTATS_END_TIMER(seizing);
  XSTATS_ADD_COUNT_POINT(num_threads, nr_threads);
}
int interrupt_process(trackedprocess_t *tracked_proc) {
  int tid;
  int status;
  int r = -1;
  XSTATS_INIT_TIMER(interrupting);

  XSTATS_START_TIMER(interrupting);
  for (int i = 0; i < tracked_proc->tid_count; i++) {
    tid = tracked_proc->tids[i];
    if (tid < 0)
      continue;
    // jprint(LVL_EXP, "INTerrupting %d\n", tid);
    r = ptrace(PTRACE_INTERRUPT, tid, NULL, NULL);
    if (r) {
      jprint(LVL_EXP, "PTRACE ERROR: %s pid:%d", strerror(errno), tid);
    }
    do {
      r = wait4(tid, &status, __WALL, NULL);
      if (r != tid) {
        jprint(LVL_EXP, "ERROR wait4 failed: %d instead of %d", r, tid);
        // jprint(LVL_EXP, "%s\n",
        //		explain_wait4(tid, &status, __WALL, NULL));
        tracked_proc->tids[i] = -1;
        // exit(EXIT_FAILURE);
        goto SKIP_THREAD;
      }

    } while (!WIFSTOPPED(status));
    int event = status >> 16;
    int signal = WSTOPSIG(status);
    assert(event == PTRACE_EVENT_STOP);
    if (event == PTRACE_EVENT_STOP) {
      if (signal == SIGSTOP || signal == SIGTSTP || signal == SIGTTIN ||
          signal == SIGTTOU) {
        jprint(LVL_TEST,
               "Tracee entered group-stop PTRACE_EVENT_STOP (signal: %d)",
               signal);
      } else {
        jprint(LVL_TEST,
               "Tracee entered non-group-stop PTRACE_EVENT_STOP (signal: %d)",
               signal);
        if (signal == SIGTRAP) {
          jprint(LVL_TEST, "Tracee was in user space and got another signal or "
                           "ptrace_INTERRUPT signal");
        }
      }
    }

    if (signal == (SIGTRAP | 0x80)) {
      jprint(LVL_TEST, "Tracee entered/exited syscall --  syscall-exit-stop");
      // ptrace(PTRACE_SYSCALL, pid, NULL, 0);
      // assert(waitpid(tid, &status, 0) == tid);
    }

  SKIP_THREAD:
    continue;
  }
  XSTATS_END_TIMER(interrupting);
}

void ptrace_attach(trackedprocess_t *tracked_proc) {
  jprint(LVL_TEST, "Going to attach");
  XSTATS_INIT_TIMER(attaching);
  XSTATS_START_TIMER(attaching);
  int status;
  long r;
  seize_process(tracked_proc, 1);
  interrupt_process(tracked_proc);
  XSTATS_END_TIMER(attaching);
}

void ptrace_detach(trackedprocess_t *tracked_proc) {
  jprint(LVL_TEST,
         "Will Detach and let the tracee %d and its %ld threads continue",
         tracked_proc->pid, tracked_proc->tid_count);
  detach_or_cont_process(tracked_proc, 1);
}

void open_proc_fds(trackedprocess_t *tracked_proc) {
  int pid = tracked_proc->pid;
  char soft_dirty_clear_file[BUFSIZ];
  char maps_file[BUFSIZ];
  char pagemap_file[BUFSIZ];
  char mem_file[BUFSIZ];

  snprintf(tracked_proc->dump_file_name, sizeof(tracked_proc->dump_file_name),
           "/local/workspace/groundhog-launcher/dumps/%d-dump", pid);

  // open fds
  snprintf(soft_dirty_clear_file, sizeof soft_dirty_clear_file,
           "/proc/%d/clear_refs", pid);
  tracked_proc->clear_fd = open(soft_dirty_clear_file, O_WRONLY);
  if (tracked_proc->clear_fd < 0) {
    iprintl(LVL_EXP, "FD OPEN ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);
    close(tracked_proc->clear_fd);
    exit(-1);
  }

  snprintf(maps_file, sizeof maps_file, "/proc/%d/maps", pid);
  tracked_proc->maps_fd = open(maps_file, O_RDONLY);
  if (tracked_proc->maps_fd < 0) {
    iprintl(LVL_EXP, "FD OPEN ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);
    close(tracked_proc->maps_fd);
    exit(-1);
  }

  snprintf(pagemap_file, sizeof pagemap_file, "/proc/%d/pagemap", pid);
  tracked_proc->pagemap_fd = open(pagemap_file, O_RDONLY);
  if (tracked_proc->pagemap_fd < 0) {
    iprintl(LVL_EXP, "FD OPEN ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);
    close(tracked_proc->pagemap_fd);
    exit(-1);
  }

  snprintf(mem_file, sizeof mem_file, "/proc/%d/mem", pid);
  tracked_proc->mem_fd = open(mem_file, O_RDWR);
  if (tracked_proc->mem_fd < 0) {
    iprintl(LVL_EXP, "FD OPEN ERROR: %s pid:%d", strerror(errno),
            tracked_proc->pid);
    close(tracked_proc->mem_fd);
    exit(-1);
  }

  tracked_proc->null_fd = open("/dev/null", O_RDONLY);

  jprint(LVL_DBG, "All file descriptors opened successfully");
}

void close_proc_fds(trackedprocess_t *tracked_proc) {
  close(tracked_proc->clear_fd);
  close(tracked_proc->maps_fd);
  close(tracked_proc->pagemap_fd);
  close(tracked_proc->mem_fd);
  close(tracked_proc->null_fd);
  jprint(LVL_DBG, "All file descriptors closed successfully");
}
