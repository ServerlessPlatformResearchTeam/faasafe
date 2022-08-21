#define _GNU_SOURCE
#include "lib/memtest-helper.h"

int main(int argc, char *argv[]) {
  printf("Welcome to the memory diff test case\n");
  fflush(stdout);
  int c = 0;
  int map_size = NPAGES * PSIZE;

  printf("\n=========%d================================\n", c++);

  // char randomblock [100*PSIZE];
  char *randomblock = allocate_random_heap_buffer(100 * PSIZE);
  // memset (randomblock, 0, 100*PSIZE);

  char *map_to_stay =
      mmap((void *)0x7f3632400000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  char *map_to_grow =
      mmap((void *)0x7f3633400000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  char *map_to_shrink =
      mmap((void *)0x7f3634400000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  char *map_to_stay2 =
      mmap((void *)0x7f3635400000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  char *map_to_merge_p1 =
      mmap((void *)0x7f3636400000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  char *map_to_merge_p2 =
      mmap((void *)0x7f3636421000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  char *map_to_split =
      mmap((void *)0x7f3637400000, 65 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  char *map2_to_merge_p1 =
      mmap((void *)0x7f3638400000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  char *map2_to_merge_p2 =
      mmap((void *)0x7f3638421000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

  assert(!madvise(map2_to_merge_p2, 32 * PSIZE, MADV_DONTNEED));

  char *map2_to_merge_p3 =
      mmap((void *)0x7f3638442000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

  char *map_to_not_page =
      mmap((void *)0x7f3638542000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

  char *map_to_fully_mprotect =
      mmap((void *)0x7f3638642000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

  char *map_to_partially_mprotect =
      mmap((void *)0x7f3638742000, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

  memcpy(map_to_stay, randomblock, 32 * PSIZE);
  memcpy(map_to_grow, randomblock, 32 * PSIZE);
  memcpy(map_to_shrink, randomblock, 32 * PSIZE);
  memcpy(map_to_stay2, randomblock, 32 * PSIZE);
  memcpy(map_to_merge_p1, randomblock, 32 * PSIZE);
  memcpy(map_to_merge_p2, randomblock, 32 * PSIZE);
  memcpy(map_to_split, randomblock, 65 * PSIZE);
  memcpy(map2_to_merge_p1, randomblock, 32 * PSIZE);
  memcpy(map2_to_merge_p2, randomblock + 32 * PSIZE, 32 * PSIZE);
  memcpy(map2_to_merge_p3, randomblock + 64 * PSIZE, 32 * PSIZE);
  memcpy(map_to_not_page + PSIZE, randomblock, 2 * PSIZE);
  memcpy(map_to_fully_mprotect, randomblock, 32 * PSIZE);
  memcpy(map_to_partially_mprotect + PSIZE, randomblock, 2 * PSIZE);
  checkpoint_me();

  errno = 0;
  printf("Checkpointed, with all randomness\n");
  printf("Asserting map_to_stay is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_stay, 32 * PSIZE));
  printf("Asserting map_to_grow is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_grow, 32 * PSIZE));
  printf("Asserting map_to_shrink is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_shrink, 32 * PSIZE));
  printf("Asserting map_to_stay2 is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_stay2, 32 * PSIZE));
  printf("Asserting map_to_merge_p1 is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_merge_p1, 32 * PSIZE));
  printf("Asserting map_to_merge_p2 is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_merge_p2, 32 * PSIZE));
  printf("Asserting map_to_split is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_split, 65 * PSIZE));
  printf("All initially allocated maps are indeed zeroed\n");
  fflush(stdout);
  printf("Asserting map2_to_merge_p1 is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map2_to_merge_p1, 32 * PSIZE));
  printf("Asserting map2_to_merge_p2 is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock + 32 * PSIZE, map2_to_merge_p2, 32 * PSIZE));
  printf("Asserting map3_to_merge_p2 is zeroed\n");
  fflush(stdout);
  assert(!memcmp(randomblock + 64 * PSIZE, map2_to_merge_p3, 32 * PSIZE));
  printf("Asserting map_to_not_page is restored\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_not_page + PSIZE, 2 * PSIZE));
  printf("Asserting map_to_fully_mprotect is restored\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_fully_mprotect, 32 * PSIZE));
  printf("Asserting map_to_partially_mprotect is restored\n");
  fflush(stdout);
  assert(!memcmp(randomblock, map_to_partially_mprotect + PSIZE, 2 * PSIZE));

  printf("Will set all initially allocated maps to 1\n");
  fflush(stdout);
  memset(map_to_stay + 10 * PSIZE, 1, 22 * PSIZE);
  memset(map_to_grow + 5 * PSIZE, 2, 20 * PSIZE);
  memset(map_to_shrink + 2 * PSIZE, 3, 28 * PSIZE);
  memset(map_to_stay2 + 10 * PSIZE, 4, 14 * PSIZE);
  memset(map_to_merge_p1, 5, 3 * PSIZE);
  memset(map_to_merge_p2, 6, 30 * PSIZE);
  memset(map_to_split + 32 * PSIZE, 7, 1 * PSIZE);
  memset(map2_to_merge_p1 + PSIZE, 8, 20 * PSIZE);
  memset(map2_to_merge_p2, 9, 30 * PSIZE);
  memset(map2_to_merge_p3, 10, 2 * PSIZE);
  memset(map_to_not_page + 2 * PSIZE, 2, 2 * PSIZE);
  memset(map_to_fully_mprotect + 2 * PSIZE, 2, 2 * PSIZE);
  mprotect(map_to_fully_mprotect, 32 * PSIZE, PROT_NONE);
  memset(map_to_partially_mprotect + 2 * PSIZE, 2, 2 * PSIZE);
  mprotect(map_to_partially_mprotect + PSIZE, 5 * PSIZE, PROT_NONE);
  printf("Successfully set to 1\n");
  fflush(stdout);

  printf("will grow map_to_grow %p \n", map_to_grow);
  map_to_grow = (char *)mremap((void *)map_to_grow, 32 * PSIZE, 64 * PSIZE, 0);
  assert(map_to_grow != NULL);
  printf("map_to_grow growed @ %p %s\n", map_to_grow, strerror(errno));
  fflush(stdout);

  map_to_shrink =
      (char *)mremap((void *)map_to_shrink, 32 * PSIZE, 16 * PSIZE, 0);
  assert(map_to_shrink != NULL);
  printf("map_to_shrink shrinked @ %p\n", map_to_shrink);
  fflush(stdout);
  assert(!munmap(map_to_merge_p2, 32 * PSIZE));
  printf("map_to_merge_p2 unmapped\n");
  fflush(stdout);

  map_to_merge_p1 =
      (char *)mremap((void *)map_to_merge_p1, 32 * PSIZE, 65 * PSIZE, 0);
  assert(map_to_merge_p1 != NULL);
  printf("map_to_merge_p1 merged\n");
  fflush(stdout);

  map_to_split =
      (char *)mremap((void *)map_to_split, 65 * PSIZE, 32 * PSIZE, 0);
  assert(map_to_split != NULL);
  printf("map_to_split shrinked\n");
  fflush(stdout);

  char *map_to_split2 =
      mmap((char *)map_to_split + PSIZE, 32 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  assert(map_to_split2 != NULL);
  printf("second part of map_to_split mapped\n");
  fflush(stdout);

  assert(!munmap(map2_to_merge_p2, 32 * PSIZE));
  assert(!munmap(map2_to_merge_p3, 32 * PSIZE));
  printf("map2_to_merge p1 and p2 unmapped\n");
  fflush(stdout);

  map2_to_merge_p1 =
      (char *)mremap((void *)map_to_merge_p1, 32 * PSIZE, 98 * PSIZE, 0);
  assert(map2_to_merge_p1 != NULL);
  printf("map2_to_merge_p1 merged with p2 and p3\n");
  fflush(stdout);

  memset(map_to_grow, 1, 64 * PSIZE);
  printf("memsetting map_to_grow \n");
  fflush(stdout);
  memset(map_to_split2, 1, 32 * PSIZE);
  printf("memsetting map_to_split2\n");
  fflush(stdout);

  char *new_map =
      mmap((void *)0x7f3637450000, 65 * PSIZE, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  memcpy(new_map, randomblock + 32 * PSIZE, 32 * PSIZE);

  dump_stats_me();
  restore_me();

  printf("\n=========%d================================\n", c++);
  fflush(stdout);
}
