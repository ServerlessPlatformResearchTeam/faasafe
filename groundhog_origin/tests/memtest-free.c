#include "lib/memtest-helper.h"

int main(int argc, char *argv[]) {

  printf("Welcome to the memory diff test case\n");
  fflush(stdout);

  int map_size = NPAGES * PSIZE;
  char *buf_shared;
  char *buf_private_cow;

  printf("\n=========================================\n");
  printf("Will malloc 57 pages\n");
  char *test = (char *)calloc(500, PSIZE);
  char *mmap_test =
      mmap((void *)140609965461504, 500 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  char *mmap_test2 = mmap(NULL, 500 * PSIZE, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  char *mmap_test3 = mmap(NULL, 500 * PSIZE, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  char *mmap_test4 = mmap(NULL, 500 * PSIZE, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (mmap_test == MAP_FAILED)
    printf("ERROR in TESTCASE:  SYSCALL FAILED:  %s\n", strerror(errno));

  mmap_test[5] = 'a';
  mmap_test2[5] = 'b';
  mmap_test3[5] = 'n';
  mmap_test4[5] = 'm';
  test[2] = 'b';
  printf("malloc'ed 57 pages\n");
  printf("\n=========================================\n");

  checkpoint_me();
  printf("\n=========================================\n");

  printf("%c\n", mmap_test[5]);
  printf("%c\n", mmap_test2[5]);
  printf("%c\n", mmap_test3[5]);
  printf("%c\n", mmap_test4[5]);
  printf("%c\n", test[2]);
  int r = munmap(mmap_test, 500 * PSIZE);
  int r3 = munmap(mmap_test3, 500 * PSIZE);
  if (r == -1) {
    printf("munmap failed with error %d", r);
  }

  free(test);
  restore_me();
  printf("\n=========================================\n");
}
