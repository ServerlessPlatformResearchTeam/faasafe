#include "lib/memtest-helper.h"

int main(int argc, char *argv[]) {
  printf("Welcome to the memory malloc test case\n");
  fflush(stdout);
  int c = 0;
  printf("\n=========%d================================\n", c++);

  checkpoint_me();

  printf("\n=========%d================================\n", c++);

  printf("\n=========%d================================\n", c++);

  printf("Will malloc 50 pages\n");
  char *test;
  for (int i = 0; i < 5; i++) {
    // never freed
    test = (char *)malloc(10 * PSIZE);
  }
  printf("malloc'ed 50 pages @ %p\n", test);

  restore_me();

  printf("\n=========%d================================\n", c++);
  fflush(stdout);
}
