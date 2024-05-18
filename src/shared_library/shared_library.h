#ifndef _SHARED_LIBRARY_H_
#define _SHARED_LIBRARY_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int faasafe_checkpoint();
int faasafe_save_page(void * page);
int faasafe_rewind();
int faasafe_dump_stats();

#endif
