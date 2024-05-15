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

    // child should expect being traced
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);

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

  }
  return pid;
}

int controller(int argc, char *argv[]) {
  int status;
  // Time to launch: fork + execvp
  pid_t pid = launch(argv + 2);
  if (pid) {
    // Wait for child's ptrace trap
    wait(&status);

    // Let our child continue
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    // ptrace(PTRACE_CONT, pid, NULL, NULL);
    // child trapped
    while ((status == 1407)) {
      wait(&status);
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    exit(-1);
  }

  controller(argc, argv);
  return 0;
}
