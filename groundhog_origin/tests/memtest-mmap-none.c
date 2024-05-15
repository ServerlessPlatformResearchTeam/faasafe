#include "lib/memtest-helper.h"

int main(int argc, char *argv[]) {

  printf("Welcome to the memory mapping case\n");
  fflush(stdout);
  char *acks;
  size_t ackssize = 32;

  acks = (char *)malloc(ackssize * sizeof(char));

  int map_size = NPAGES * PSIZE;
  char *buf_shared;
  char *buf_private_cow;

  printf("\n=========================================\n");
  printf("Will malloc 57 pages\n");
  char *mmap_test = mmap((void *)140609965461504, 50 * PSIZE, PROT_NONE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (mmap_test == MAP_FAILED)
    printf("ERROR in TESTCASE:  SYSCALL FAILED:  %s\n", strerror(errno));

  printf("malloc'ed 57 pages\n");
  printf("\n=========================================\n");
  checkpoint_me();
  printf("\n=========================================\n");

  int r = munmap(mmap_test, 50 * PSIZE);
  if (r == -1) {
    printf("munmap failed with error %d", r);
  }

  restore_me();
  printf("\n=========================================\n");
}
