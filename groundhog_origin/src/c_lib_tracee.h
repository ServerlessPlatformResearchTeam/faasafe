#ifndef _UFFD_TRACEE_H_
#define _UFFD_TRACEE_H_

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int checkpoint_me();
int restore_me();
int dump_stats_me();
#endif
