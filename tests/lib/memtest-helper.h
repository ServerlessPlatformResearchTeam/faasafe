#include <assert.h>
#include <err.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../../src/c_lib_tracee.h"
#include "../../src/config.h"

#define PSIZE 4096
#define NPAGES 512

int create_mmap_shared(char **buf_shared, int map_size);
int mmap_private_from_shared(int shm_fd, char **buf_cow, int map_size);
void clean_mmap(char *buf, int map_size);
void clean_mmap_shared(int fd, char *buf, int map_size);
void write_random_pages_uniform(char *buf, int num_pages);
char *allocate_random_heap_buffer(size_t size);
