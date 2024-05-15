/**
 * This is Groundhog's entry file.
 * Here Groundhog starts, forks and execs the function's runtime, sets up the
 * communication pipes, and drops its privelages.
 *
 * This program runs the 2nd argument argv[1] in a chilld process,
 * all information about the child is stored in the trackedprocess_t struct.
 *
 *
 * Pipes logic:
 * https://jameshfisher.com/2017/02/17/how-do-i-call-a-program-in-c-with-pipes/
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "libs/thpool.h"
#include "libs/xstats.h"
#include "proc_control.h"
#include "proc_cr.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h> /* for ptrace() */
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/user.h> /* for struct user_regs_struct */
#include <sys/wait.h>
#include <unistd.h>

#include "libs/proc_privileges.h" /* for dropping privelages */
#include "communicator.h"
#include "stat_dump.h"

#define STRACE 0

#define INPUT_TIMEOUT 10
pthread_t controller_tid;
trackedprocess_t *tracked_proc;

void close_fd(int fd) {
  if (close(fd) == -1) {
    perror("Could not close_fd pipe end");
    exit(1);
  }
}

void mk_pipe(int fds[2]) {
  if (pipe(fds) == -1) {
    perror("Could not create pipe");
    exit(1);
  }
}

void mv_fd(int fd1, int fd2) {
  if (dup2(fd1, fd2) == -1) {
    perror("Could not duplicate pipe end");
    exit(1);
  }
  close_fd(fd1);
}

void prepare_thread_pool_if_needed() {
  XSTATS_INIT_TIMER(thread_pool);
  XSTATS_START_TIMER(thread_pool);
#if CONCURRENCY > 1
  tracked_proc->thpool = thpool_init(CONCURRENCY);
#else
  jprint(LVL_TEST, "NO THREADING");
  tracked_proc->thpool = NULL;
#endif
  XSTATS_END_TIMER(thread_pool);
}

void tear_down_thread_pool_if_needed() {
#if CONCURRENCY > 1
  thpool_destroy(tracked_proc->thpool);
#endif
}

void prepare_stdio_threads() {
  pthread_t handle_msg_tid;
  pthread_create(&handle_msg_tid, NULL, handle_stdout, (void *)tracked_proc);
  pthread_t handle_stderr_tid;
  pthread_create(&handle_stderr_tid, NULL, handle_stderr, (void *)tracked_proc);
  pthread_t handle_stdin_tid;
  pthread_create(&handle_stdin_tid, NULL, handle_stdin, (void *)tracked_proc);
}

void why_did_child_escape(int status) {
  if (WIFEXITED(status)) {
    printf("child exited, status=%d\n", WEXITSTATUS(status));
  } else if (WIFSIGNALED(status)) {
    printf("child killed by signal %d\n", WTERMSIG(status));
    fflush(stdout);
  } else if (WIFSTOPPED(status)) {
    printf("child stopped by signal %d\n", WSTOPSIG(status));
  } else if (WIFCONTINUED(status)) {
    printf("child continued\n");
  }
}

// Set up new stdin, stdout and stderr.
// Start program at argv[0] with arguments argv.
// Puts references to new process and pipes into `p`.
pid_t launch(char *argv[], trackedprocess_t *p) {
  // prepare the pipes
  int tracked_in[2];
  int tracked_out[2];
  int tracked_err[2];
  int tracked_cmd_in[2];
  int tracked_cmd_out[2];
  mk_pipe(tracked_in);
  mk_pipe(tracked_out);
  mk_pipe(tracked_err);
  mk_pipe(tracked_cmd_in);
  mk_pipe(tracked_cmd_out);

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  }

  if (pid == 0) {
    // close_fd parent pipes
    close_fd(0);
    close_fd(1);
    close_fd(2);
    // unused tracked pipe ends
    close_fd(tracked_in[1]);
    close_fd(tracked_out[0]);
    close_fd(tracked_err[0]);
    close_fd(tracked_cmd_in[1]);
    close_fd(tracked_cmd_out[0]);
    // make the new pipes appear as the standard
    mv_fd(tracked_in[0], 0);
    mv_fd(tracked_out[1], 1);
    mv_fd(tracked_err[1], 2);
    mv_fd(tracked_cmd_in[0], 4);
    mv_fd(tracked_cmd_out[1], 5);

    spc_drop_privileges(1);

    // child should expect being traced
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    // finally launching
    jprint(LVL_TEST, "Launching the process: %s\n", argv[1]);
#if RESTORE_TIME
    // prepare libfaketime envp
    char *envp[] = {"LD_PRELOAD=/bin/build/libfaketime.so.1", NULL};
    execvpe(argv[1], argv + 1, envp);
#else
    execvp(argv[1], argv + 1);
#endif
    // this never exeutes...
    exit(127);
  } else {
    // unused tracked pipe ends
    close_fd(tracked_in[0]);
    close_fd(tracked_out[1]);
    close_fd(tracked_err[1]);
    close_fd(tracked_cmd_in[0]);
    close_fd(tracked_cmd_out[1]);

    p->pid = pid;

    p->stdin = tracked_in[1]; // parent wants to write to subprocess tracked_in
    p->stdout =
        tracked_out[0]; // parent wants to read from subprocess tracked_out
    p->stderr =
        tracked_err[0]; // parent wants to read from subprocess tracked_err
    p->stdcmdin = tracked_cmd_in[1];
    p->stdcmdout = tracked_cmd_out[0];
  }
  return pid;
}

int controller(int argc, char *argv[]) {
  XSTATS_INIT_TIMER(groundhog_time);
  XSTATS_START_TIMER(groundhog_time);
  int status;

  tracked_proc = (trackedprocess_t *)calloc(1, sizeof(trackedprocess_t));
  int keepalive = strtol(argv[1], NULL, 10);
  tracked_proc->keepalive = keepalive;
  XSTATS_ADD_COUNT_POINT(num_keep_alives, keepalive);
  memcpy(&(tracked_proc->stats_file_name), argv[2], strlen(argv[2]));
  // Time to launch: fork + execvp
  pid_t pid = launch(argv + 2, tracked_proc);
  if (pid) {
    prepare_thread_pool_if_needed();
    // Wait for child's ptrace trap
    wait(&status);
    jprint(LVL_TEST, "We are tracking the process: %s, pid: %d\n", argv[3],
           pid);

    open_proc_fds(tracked_proc);
    prepare_stdio_threads();

    // Let our child continue
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    // ptrace(PTRACE_CONT, pid, NULL, NULL);
    // child trapped
    while ((status == 1407)) {
      handle_stdcmd(tracked_proc);
      wait(&status);
    }
    jprint(LVL_TEST, "Child escaped!");
    why_did_child_escape(status);
    close_proc_fds(tracked_proc);
    tear_down_thread_pool_if_needed();
  }
  assert(tracked_proc->revived_count >= keepalive - 1);
  free_tracked_proc(tracked_proc);
  XSTATS_END_TIMER(groundhog_time);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    jprint(LVL_EXP, "ERROR: You must provide a keepalive count, an output "
                    "directory, and a program");
    exit(-1);
  }

  setbuf(stdout, NULL);
  init_all_stats();
  controller(argc, argv);
  dump_json_stats_to_file(argc, argv);
  return 0;
}
