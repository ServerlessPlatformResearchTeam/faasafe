#include "../src/c_lib_tracee.h"

int main(int argc, char *argv[]) {
  printf("Welcome to the simplist test case\n");
  fflush(stdout);

  int seq = 0;
  checkpoint_me();
  assert(seq == 0);
  printf("%d: Hello Wordl!\n", seq++);
  printf("New sequence number: %d\n", seq);
  fflush(stdout);
  restore_me();
}
