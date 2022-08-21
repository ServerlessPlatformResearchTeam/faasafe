#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "libs/xstats.h"
#include "structs.h"
#include "proc_control.h"
#include "proc_cr.h"
#include "stat_dump.h"
#include <sys/types.h>

void *handle_stderr(void *args);
void *handle_stdout(void *args);
void *handle_stdin(void *args);
void *handle_stdcmd(void *args);
