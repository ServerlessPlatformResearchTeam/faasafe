#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/c_lib_tracee.h"
#include "../src/config.h"
#if USE_UFFD
#include "../src/uflib_tracee.h"
#endif

#define NUM_THREADS 5

void *PrintHello(void *threadid) {
  int tid;
  tid = *((int *)threadid);
  int count = 0;
  while (1) {
    printf("Hello World! Thread ID, %d: %d\n", tid, count++);
    fflush(stdout);
    sleep(0.5);
  }
}
int main() {
  pthread_t threads[NUM_THREADS];
  int rc;
  int i;
  int *tids = calloc(NUM_THREADS, sizeof(int));
  for (i = 0; i < NUM_THREADS; i++) {
    printf("main() : creating thread %d\n", i);
    tids[i] = i;
    rc = pthread_create(&threads[i], NULL, PrintHello, (void *)&tids[i]);
    if (rc) {
      printf("Error:unable to create thread, %d\n", rc);
      exit(-1);
    }
  }
  sleep(2);
  checkpoint_me();
  sleep(2);
  restore_me();
  pthread_exit(NULL);
  free(tids);
}
