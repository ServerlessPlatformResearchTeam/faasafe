#include "lib/memtest-helper.h"

int main(int argc, char *argv[]) {

  printf("Welcome to the memory private mmap test case\n");
  fflush(stdout);
  int map_size = NPAGES * PSIZE;
  char *buf_shared;
  char *buf_private_cow;

  printf("\n=========================================\n");
  printf("Will mmap a new shared memory region\n");
  int shared_fd = create_mmap_shared(&buf_shared, map_size);
  printf("mmaped a new shared memory region\n");

  printf("\n=========================================\n");
  printf("Will mmap a private memory region on top of the shared memory one\n");
  mmap_private_from_shared(shared_fd, &buf_private_cow, map_size);
  printf("mmaped a private memory region on top of the shared memory one\n");
  printf("\n=========================================\n");

  checkpoint_me();

  printf("\n=========================================\n");

  printf("\n=========================================\n");
  printf("Will unmap the memory regions\n");
  clean_mmap(buf_private_cow, map_size);
  printf("unmapped the memory regions\n");

  restore_me();

  clean_mmap_shared(shared_fd, buf_shared, map_size);
  printf("\n=========================================\n");
}
