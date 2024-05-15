#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <grp.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
void spc_drop_privileges(int permanent);
void spc_restore_privileges(void);
