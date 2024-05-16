#ifndef _SHARED_LIBRARY_H_
#define _SHARED_LIBRARY_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int checkpoint_me();
int restore_me();
int dump_stats_me();

#endif
