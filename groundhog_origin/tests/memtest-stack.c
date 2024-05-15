#include "lib/memtest-helper.h"

void recursive(int i) {
  if (i == 1000) {
    printf("Did %d iterations\n", i);
    restore_me();
  } else {
    assert(i < 1000);
    recursive(i + 1);
  }
}

int main(int argc, char *argv[]) {
  printf("Welcome to the memory stack test case\n");
  printf("\n=========================================\n");
  fflush(stdout);
  checkpoint_me();
  printf("\n=========================================\n");
  printf("Will do a lot of recursive calls\n");
  recursive(1);
  printf("\n=========================================\n");
}
