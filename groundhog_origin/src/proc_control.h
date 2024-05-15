#include "proc_cr.h"
#include <sys/ptrace.h> /* for ptrace() */
#include <sys/user.h>   /* for struct user_regs_struct */

int wait_process(const pid_t pid, int *const statusptr);
int continue_process(const pid_t pid, int *const statusptr);
int detach_or_cont_process(trackedprocess_t *tracked_proc, int do_detach);
int seize_process(trackedprocess_t *tracked_proc, int do_seize);
int interrupt_process(trackedprocess_t *tracked_proc);
void ptrace_attach(trackedprocess_t *tracked_proc);
void ptrace_detach(trackedprocess_t *tracked_proc);

void open_proc_fds(trackedprocess_t *tracked_proc);
void close_proc_fds(trackedprocess_t *tracked_proc);
