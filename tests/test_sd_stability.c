/**
 * File              : test_sd_stability.c
 * Author            : Mohamed Alzayat <alzayat@mpi-sws.org>
 * Date              : 19.11.2020
 * Last Modified Date: 30.11.2020
 * Last Modified By  : Mohamed Alzayat <alzayat@mpi-sws.org>
 * Adapted from:
 * https://github.com/dwks/pagemap/blob/8a25747bc79d6080c8b94eac80807a4dceeda57a/pagemap2.c
 * https://github.com/cirosantilli/linux-kernel-module-cheat/blob/25f9913e0c1c5b4a3d350ad14d1de9ac06bfd4be/kernel_module/user/pagemap_dump.c
 * https://src.openvz.org/projects/OVZ/repos/criu/browse/criu/mem.c
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  uint64_t pfn : 54;
  unsigned int soft_dirty : 1;
  unsigned int file_page : 1;
  unsigned int swapped : 1;
  unsigned int present : 1;
} PagemapEntry;

typedef struct {
  pid_t pid;
  int clear_fd;
  int pagemap_fd;
  int maps_fd;
  int mem_fd;
} trackedprocess_t;

void close_proc_fds(trackedprocess_t *tracked_proc) {
  close(tracked_proc->clear_fd);
  close(tracked_proc->maps_fd);
  close(tracked_proc->pagemap_fd);
  close(tracked_proc->mem_fd);
}

void open_proc_fds(trackedprocess_t *tracked_proc) {
  int pid = tracked_proc->pid;
  char soft_dirty_clear_file[BUFSIZ];
  char maps_file[BUFSIZ];
  char pagemap_file[BUFSIZ];
  char mem_file[BUFSIZ];

  // open fds
  snprintf(soft_dirty_clear_file, sizeof soft_dirty_clear_file,
           "/proc/%d/clear_refs", pid);
  tracked_proc->clear_fd = open(soft_dirty_clear_file, O_WRONLY);
  if (tracked_proc->clear_fd < 0) {
    printf("FD OPEN ERROR: %s pid:%d\n", strerror(errno), tracked_proc->pid);
    close(tracked_proc->clear_fd);
    exit(-1);
  }

  snprintf(maps_file, sizeof maps_file, "/proc/%d/maps", pid);
  tracked_proc->maps_fd = open(maps_file, O_RDONLY);
  if (tracked_proc->maps_fd < 0) {
    printf("FD OPEN ERROR: %s pid:%d\n", strerror(errno), tracked_proc->pid);
    close(tracked_proc->maps_fd);
    exit(-1);
  }

  snprintf(pagemap_file, sizeof pagemap_file, "/proc/%d/pagemap", pid);
  tracked_proc->pagemap_fd = open(pagemap_file, O_RDONLY);
  if (tracked_proc->pagemap_fd < 0) {
    printf("FD OPEN ERROR: %s pid:%d\n", strerror(errno), tracked_proc->pid);
    close(tracked_proc->pagemap_fd);
    exit(-1);
  }
  snprintf(mem_file, sizeof mem_file, "/proc/%d/mem", pid);
  tracked_proc->mem_fd = open(mem_file, O_RDWR);
  if (tracked_proc->mem_fd < 0) {
    printf("FD OPEN ERROR: %s pid:%d\n", strerror(errno), tracked_proc->pid);
    close(tracked_proc->mem_fd);
    exit(-1);
  }
}

/* Parse the pagemap entry for the given virtual address.
 *
 * @param[out] entry      the parsed entry
 * @param[in]  pagemap_fd file descriptor to an open /proc/pid/pagemap file
 * @param[in]  vaddr      virtual address to get entry for
 * @return 0 for success, 1 for failure
 */
int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr) {
  size_t nread;
  ssize_t ret;
  uint64_t data;

  nread = 0;
  while (nread < sizeof(data)) {
    ret = pread(pagemap_fd, &data, sizeof(data),
                (vaddr / sysconf(_SC_PAGE_SIZE)) * sizeof(data) + nread);
    nread += ret;
    if (ret <= 0) {
      return 1;
    }
  }
  entry->pfn = data & (((uint64_t)1 << 54) - 1);
  entry->soft_dirty = (data >> 55) & 1;
  entry->file_page = (data >> 61) & 1;
  entry->swapped = (data >> 62) & 1;
  entry->present = (data >> 63) & 1;
  return 0;
}

/* Convert the given virtual address to physical using /proc/PID/pagemap.
 *
 * @param[out] paddr physical address
 * @param[in]  pid   process to convert for
 * @param[in] vaddr virtual address to get entry for
 * @return 0 for success, 1 for failure
 */
int virt_to_phys_user(uintptr_t *paddr, pid_t pid, uintptr_t vaddr) {
  char pagemap_file[BUFSIZ];
  int pagemap_fd;

  snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap",
           (uintmax_t)pid);
  pagemap_fd = open(pagemap_file, O_RDONLY);
  if (pagemap_fd < 0) {
    return 1;
  }
  PagemapEntry entry;
  if (pagemap_get_entry(&entry, pagemap_fd, vaddr)) {
    return 1;
  }
  close(pagemap_fd);
  *paddr =
      (entry.pfn * sysconf(_SC_PAGE_SIZE)) + (vaddr % sysconf(_SC_PAGE_SIZE));
  return 0;
}

int do_task_reset_dirty_track(int fd) {
  int ret;
  char cmd[] = "4";

  if (fd < 0)
    return errno == EACCES ? 1 : -1;

  ret = write(fd, cmd, sizeof(cmd));
  if (ret < 0) {
    if (errno == EINVAL) /* No clear-soft-dirty in kernel */
      ret = 1;
    else {
      printf("Can't reset %d's dirty memory tracker (%d): %s\n", getpid(),
             errno, strerror(errno));
      ret = -1;
    }
  } else {
    ret = 0;
  }

  return ret;
}

int count_sd_bits(trackedprocess_t *tracked_proc) {

  char buffer[BUFSIZ];

  int maps_fd = tracked_proc->maps_fd;
  int pagemap_fd = tracked_proc->pagemap_fd;
  int count_sd = 0;
  int offset = 0;

  for (;;) {
    ssize_t length = read(maps_fd, buffer + offset, sizeof buffer - offset);
    if (length <= 0)
      break;
    length += offset;
    for (size_t i = offset; i < (size_t)length; i++) {
      uintptr_t low = 0, high = 0;
      if (buffer[i] == '\n' && i) {
        size_t y;
        /* Parse a line from maps. Each line contains a range that contains many
         * pages. */
        {
          size_t x = i - 1;
          while (x && buffer[x] != '\n')
            x--;
          if (buffer[x] == '\n')
            x++;
          while (buffer[x] != '-' && x < sizeof buffer) {
            char c = buffer[x++];
            low *= 16;
            if (c >= '0' && c <= '9') {
              low += c - '0';
            } else if (c >= 'a' && c <= 'f') {
              low += c - 'a' + 10;
            } else {
              break;
            }
          }
          while (buffer[x] != '-' && x < sizeof buffer)
            x++;
          if (buffer[x] == '-')
            x++;
          while (buffer[x] != ' ' && x < sizeof buffer) {
            char c = buffer[x++];
            high *= 16;
            if (c >= '0' && c <= '9') {
              high += c - '0';
            } else if (c >= 'a' && c <= 'f') {
              high += c - 'a' + 10;
            } else {
              break;
            }
          }
          for (int field = 0; field < 4; field++) {
            x++;
            while (buffer[x] != ' ' && x < sizeof buffer)
              x++;
          }
          while (buffer[x] == ' ' && x < sizeof buffer)
            x++;
          y = x;
          while (buffer[y] != '\n' && y < sizeof buffer)
            y++;
          buffer[y] = 0;
        }

        /* Get info about all pages in this page range with pagemap. */
        {
          PagemapEntry entry;
          for (uintptr_t addr = low; addr < high;
               addr += sysconf(_SC_PAGE_SIZE)) {
            /* TODO always fails for the last page (vsyscall), why? pread
             * returns 0. */
            if (!pagemap_get_entry(&entry, pagemap_fd, addr)) {
              if (entry.soft_dirty)
                count_sd += 1;
            }
          }
        }
        buffer[y] = '\n';
      }
    }
  }
  lseek(maps_fd, 0, SEEK_SET);
  return count_sd;
}

int test() {
  int x = 0;
  x = 5 * 16;
  x++;
  return x;
}
int main() {
  int pid = getpid();
  int x = 0;
  int total_sd = 0;
  trackedprocess_t *tracked_proc;
  tracked_proc = (trackedprocess_t *)calloc(1, sizeof(trackedprocess_t));
  tracked_proc->pid = pid;
  open_proc_fds(tracked_proc);
  do {
    total_sd = 0;
    assert(do_task_reset_dirty_track(tracked_proc->clear_fd) == 0);
    x++;
    test();
    total_sd = count_sd_bits(tracked_proc);
    if (total_sd == 0) {
      sleep(1);
      perror("Failure");
      goto exit;
    }
  } while (x < 1000);
  printf("Success\n");
exit:
  close_proc_fds(tracked_proc);
  free(tracked_proc);
  return 0;
}
