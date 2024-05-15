#include "memtest-helper.h"

int create_mmap_shared(char **buf_shared, int map_size) {
  char mmap_file[BUFSIZ];
  // Create a new shared memory region (file) and mmap it
  sprintf(mmap_file, "/tmpshmem");
  shm_unlink(mmap_file);
  int fd = shm_open(mmap_file, O_CREAT | O_RDWR, 0666);
  assert(fd > 0);
  int r = ftruncate(fd, map_size);
  assert(r == 0);

  *buf_shared =
      (char *)mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if ((*buf_shared) == MAP_FAILED) {
    printf("mmap failed: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  // Force demand paging NOW!
  for (int i = 0; i < map_size; i += PSIZE) {
    ((char *)(*buf_shared))[i] = i;
  }
  return fd;
}

int mmap_private_from_shared(int shm_fd, char **buf_cow, int map_size) {
  *buf_cow =
      mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, shm_fd, 0);
  if ((*buf_cow) == MAP_FAILED) {
    printf("mmap failed: %s\n", strerror(errno));
    return -1;
  }
  return shm_fd;
}

void clean_mmap(char *buf, int map_size) {
  int r = munmap(buf, map_size);
  if (r == -1) {
    printf("munmap failed with error %d", r);
  }
}

void clean_mmap_shared(int fd, char *buf, int map_size) {
  char mmap_file[BUFSIZ];
  sprintf(mmap_file, "/tmpshmem");
  int r = munmap(buf, map_size);
  if (r == -1) {
    printf("munmap failed with error %d", r);
  }
  shm_unlink(mmap_file);
  close(fd);
}

void write_random_pages_uniform(char *buf, int num_pages) {
  uint64_t buffer_index = -1;
  for (int i = 0; i < num_pages; i++) {
    buffer_index = (i * (uint64_t)NPAGES / num_pages) * PSIZE;
    buf[buffer_index] = i * 7;
  }
}

char *allocate_random_heap_buffer(size_t size) {
  time_t current_time = time(NULL);
  srandom((unsigned int)current_time);
  char *allocatedMemory = (char *)malloc(size);

  for (size_t bufferIndex = 0; bufferIndex < size; bufferIndex++) {
    uint8_t randomNumber = (uint8_t)random();
    allocatedMemory[bufferIndex] = randomNumber;
  }

  return allocatedMemory;
}
