#include "../src/c_lib_tracee.h"
#include "../src/config.h"

#define PSIZE 4096
#define NPAGES 512

int main(int argc, char *argv[]) {

  printf("Welcome to the memory diff test case\n");
  fflush(stdout);
  int operations_count = 0;

  printf("\n=========================================\n");
  printf("Will do an operation, checkpoint and then do 2 operations after the "
         "checkpoint\n");
  printf("Operation %d: 1 + 2 = %d\n", operations_count++, 1 + 2);

  checkpoint_me();
  assert(operations_count == 1);
  int x = 0;
  printf("Operation %d: 2 + 3 = %d\n", operations_count++, 2 + 3);
  printf("Operation %d: 3 + 4 = %d\n", operations_count++, 3 + 4);
  x = 17;
  printf("\n=========================================\n");

  restore_me();
  printf("Operation %d: 4 + 5 = %d\n", operations_count++, 4 + 5);
  printf("Operation %d: 5 + 6 = %d\n", operations_count++, 5 + 6);

  printf("\n=========================================\n");
}
