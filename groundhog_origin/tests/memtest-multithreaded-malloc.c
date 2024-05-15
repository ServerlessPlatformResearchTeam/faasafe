#include "../src/c_lib_tracee.h"
#include "../src/config.h"
#include "lib/memtest-helper.h"

#define NUM_THREADS 5
#define PSIZE 4096

void *PrintHello(void *threadid) {
  long tid;
  tid = (long)threadid;
  int count = 0;
  char *test = (char *)calloc(50, PSIZE);
  char *mmap_test = mmap(NULL, 50 * PSIZE, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  mmap_test[5] = 'a';
  test[2] = 'b';
  while (1) {
    printf("Hello World! Thread ID, %ld: %d\n", tid, count++);
    sleep(1);
    if (count <= 4) {
      assert(mmap_test[5] == 'a');
      assert(test[2] = 'b');
    }
    if (count == 4) {
      int r = munmap(mmap_test, 50 * PSIZE);
      if (r == -1) {
        printf("munmap failed with error %d", r);
      }
      free(test);
      test = (char *)calloc(50, PSIZE);
      mmap_test = mmap(NULL, 50 * PSIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
  }
}
int main() {
  pthread_t threads[NUM_THREADS];
  int rc;
  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    printf("main() : creating thread %d\n", i);
    rc = pthread_create(&threads[i], NULL, PrintHello, (void *)(intptr_t)i);
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
}
