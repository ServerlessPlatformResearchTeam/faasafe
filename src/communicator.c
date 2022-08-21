/*
 * Handling the communication between the function and the platform as well as
 * the function and Groundhog.
 *
 */

#include "communicator.h"

void *handle_exit(void *args) {
  trackedprocess_t *tracked_proc = (trackedprocess_t *)args;
  setbuf(stdout, NULL);
  while (1) {
    siginfo_t infop;
    waitid(P_ALL, 0, &infop, WEXITED);
    if (tracked_proc->keepalive != tracked_proc->revived_count) {
      if (infop.si_status == SIGSEGV) {
        psiginfo(&infop, "ERROR");
        jprint(LVL_EXP,
               "ERROR: CHILD %d EXITED, code %d: %s causing %d: %s, child was "
               "successfully revived %d times",
               infop.si_pid, infop.si_code, strsignal(infop.si_code),
               infop.si_status, strsignal(infop.si_status),
               tracked_proc->revived_count);
        exit(-1);
      } else {
        psiginfo(&infop, "");
        jprint(LVL_EXP,
               "CHILD %d EXITED, code %d: %s causing %d: %s, child was "
               "successfully revived %d times",
               infop.si_pid, infop.si_code, strsignal(infop.si_code),
               infop.si_status, strsignal(infop.si_status),
               tracked_proc->revived_count);
      }
    }
  }
}

void *handle_stderr(void *args) {
  trackedprocess_t *tracked_proc = (trackedprocess_t *)args;
  setbuf(stdout, NULL);
  char *child_err = (char *)malloc(BUFSIZ * sizeof(char));
  size_t buffersiz = BUFSIZ;
  FILE *tracked_err_fp = fdopen(tracked_proc->stderr, "r");
  while (getline(&child_err, &buffersiz, tracked_err_fp) > 0) {
    fprintf(stderr, "%s", child_err);
  }
  fclose(tracked_err_fp);
  free(child_err);
}

void *handle_stdout(void *args) {
  trackedprocess_t *tracked_proc = (trackedprocess_t *)args;
  setbuf(stdout, NULL);
  size_t buffersiz = 20 * BUFSIZ;
  char *child_out = (char *)malloc(20 * BUFSIZ * sizeof(char));
  FILE *tracked_out_fp = fdopen(tracked_proc->stdout, "r");

  int ret = fcntl(tracked_proc->stdout, F_SETPIPE_SZ, 1024 * 1024);
  if (ret < 0) {
    perror("set pipe size failed.");
  }

  while (getline(&child_out, &buffersiz, tracked_out_fp) > 0) {
    fprintf(stdout, "%s", child_out);
    fflush(stdout);
  }

  fclose(tracked_out_fp);
  free(child_out);
}

void *handle_stdin(void *args) {
  trackedprocess_t *tracked_proc = (trackedprocess_t *)args;
  char *parent_in = (char *)malloc(BUFSIZ * sizeof(char));
  size_t buffersiz = BUFSIZ;
  jprint(LVL_TEST, "All stdin will be forwarded to the child");
  while (getline(&parent_in, &buffersiz, stdin) > 0) {
#if XDEBUG_LVL <= LVL_TEST
    jprint(LVL_EXP, "STDin: %s %ld", parent_in, strlen(parent_in));
#endif
    write(tracked_proc->stdin, parent_in, strlen(parent_in));
  }
  free(parent_in);
}

void *handle_stdcmd(void *args) {
  // XSTATS_INIT_TIMER(groundhog_warm_time);
  trackedprocess_t *tracked_proc = (trackedprocess_t *)args;
  setbuf(stdout, NULL);
  char *child_command = (char *)malloc(BUFSIZ * sizeof(char));
  char *scratchmem = (char *)malloc(BUFSIZ * sizeof(char));
  size_t scratchbuffersiz = BUFSIZ;
  size_t buffersiz = BUFSIZ;
  char *cmd_checkpoint = "checkpoint.me";
  char *cmd_diff = "diff.me";
  char *cmd_restore = "restore.me";
  char *cmd_nop = "nop.me";
  char *cmd_dumpstats = "dump_stats.me";
  char *cmd_saveforme = "save_for.me";
  char *cmd_givebackme = "give_back.me";
  char *cmd_detach = "detach.it";
  char *cmd_cont = "cont.it";
  char *cmd_ACK = "OK!\n";
  int num_revived = 0;
  int status = 0;
  bool stop_tracking = 0;
  FILE *tracked_out_fp = fdopen(tracked_proc->stdcmdout, "r");

  while (getline(&child_command, &buffersiz, tracked_out_fp) > 0) {
    if (!strncmp(child_command, cmd_checkpoint, strlen(cmd_checkpoint))) {
      if (!stop_tracking) {
        checkpoint(tracked_proc);
      }
      // XSTATS_START_TIMER(groundhog_warm_time);
      write(tracked_proc->stdcmdin, cmd_ACK, strlen(cmd_ACK));
      continue;
    }
    if (!strncmp(child_command, cmd_diff, strlen(cmd_diff))) {
      if (!stop_tracking)
        diff(tracked_proc);
      write(tracked_proc->stdcmdin, cmd_ACK, strlen(cmd_ACK));
      continue;
    }

    if (!strncmp(child_command, cmd_restore, strlen(cmd_restore))) {

      if (num_revived == tracked_proc->keepalive - 1) {
        // XSTATS_END_TIMER(groundhog_warm_time);
        jprint(LVL_EXP, "Killing child, kept alive for too long");
        kill(tracked_proc->pid, SIGKILL);
        break;
      }

#if !GH_NOP
      if (!stop_tracking)
        restore(tracked_proc);
#endif
      num_revived += 1;
      tracked_proc->revived_count = num_revived;
      write(tracked_proc->stdcmdin, cmd_ACK, strlen(cmd_ACK));
      continue;
    }

    if (!strncmp(child_command, cmd_dumpstats, strlen(cmd_dumpstats))) {
      dump_stats();
      write(tracked_proc->stdcmdin, cmd_ACK, strlen(cmd_ACK));
      continue;
    }
    if (!strncmp(child_command, cmd_nop, strlen(cmd_nop))) {
      write(tracked_proc->stdcmdin, cmd_ACK, strlen(cmd_ACK));
      continue;
    }
    if (!strncmp(child_command, cmd_saveforme, strlen(cmd_saveforme))) {
      getline(&scratchmem, &scratchbuffersiz, tracked_out_fp);
      jprint(LVL_EXP, "got %lu bytes for %s\n", strlen(scratchmem), scratchmem);
      fflush(stdout);
      write(tracked_proc->stdcmdin, cmd_ACK, strlen(cmd_ACK));
      continue;
    }

    if (!strncmp(child_command, cmd_givebackme, strlen(cmd_givebackme))) {
      jprint(LVL_EXP, "giving back %lu bytes: %s\n", strlen(scratchmem),
             scratchmem);
      fflush(stdout);
      write(tracked_proc->stdcmdin, scratchmem, strlen(scratchmem));
      continue;
    }

    if (!strncmp(child_command, cmd_detach, strlen(cmd_detach))) {
      detach_or_cont_process(tracked_proc, 1);
      continue;
    }

    if (!strncmp(child_command, cmd_cont, strlen(cmd_cont))) {
      detach_or_cont_process(tracked_proc, 0);
      continue;
    }
  }
  fclose(tracked_out_fp);
  free(child_command);
  free(scratchmem);
}
