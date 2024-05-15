#include "proc_parsers.h"

/* parses a decimal from the buffer starting at buf_pointer and
 * keeps parsing until the hexadecimal ends or a delimeter is hit
 * modifies the buf_pointer to point to the very next element after the hex
 */
static inline uint64_t parse_dec_untill(char *buffer, char del,
                                        size_t *buf_pointer, size_t buf_size) {
  uint64_t val = 0;

  while (buffer[*buf_pointer] != del && (*buf_pointer) + 1 < buf_size) {
    char c = buffer[(*buf_pointer)++];
    // jprint(LVL_EXP, "%c", c);
    val *= 10;
    if (c >= '0' && c <= '9') {
      val += c - '0';
    } else
      break;
  }
  return val;
}

/* parses a hexadecimal from the buffer starting at buf_pointer and
 * keeps parsing until the hexadecimal ends or a delimeter is hit
 * modifies the buf_pointer to point to the very next element after the hex
 */
static inline uint64_t parse_hex2dec_untill(char *buffer, char del,
                                            size_t *buf_pointer,
                                            size_t buf_size) {
  uint64_t val = 0;

  while (buffer[*buf_pointer] != del && (*buf_pointer) + 1 < buf_size) {
    char c = buffer[(*buf_pointer)++];
    // jprint(LVL_EXP, "%c", c);
    val *= 16;
    if (c >= '0' && c <= '9') {
      val += c - '0';
    } else if (c >= 'a' && c <= 'f') {
      val += c - 'a' + 10;
    } else
      break;
  }
  return val;
}

// https://stackoverflow.com/questions/18577956/how-to-use-ptrace-to-get-a-consistent-view-of-multiple-threads
/* Similar to getline(), except gets process pid task IDs.
 * Returns positive (number of TIDs in list) if success,
 * otherwise 0 with errno set. */
size_t get_tids(pid_t **const listptr, size_t *const sizeptr, const pid_t pid) {
  char dirname[64];
  DIR *dir;
  pid_t *list;
  size_t size, used = 0;

  if (!listptr || !sizeptr || pid < (pid_t)1) {
    errno = EINVAL;
    return (size_t)0;
  }

  if (*sizeptr > 0) {
    list = *listptr;
    size = *sizeptr;
  } else {
    list = *listptr = NULL;
    size = *sizeptr = 0;
  }

  if (snprintf(dirname, sizeof dirname, "/proc/%d/task/", (int)pid) >=
      (int)sizeof dirname) {
    errno = ENOTSUP;
    return (size_t)0;
  }

  dir = opendir(dirname);
  if (!dir) {
    errno = ESRCH;
    return (size_t)0;
  }

  while (1) {
    struct dirent *ent;
    int value;
    char dummy;

    errno = 0;
    ent = readdir(dir);
    if (!ent)
      break;

    /* Parse TIDs. Ignore non-numeric entries. */
    if (sscanf(ent->d_name, "%d%c", &value, &dummy) != 1)
      continue;

    /* Ignore obviously invalid entries. */
    if (value < 1)
      continue;

    /* Make sure there is room for another TID. */
    if (used >= size) {
      size = (used | 127) + 128;
      list = realloc(list, size * sizeof list[0]);
      if (!list) {
        closedir(dir);
        errno = ENOMEM;
        return (size_t)0;
      }
      *listptr = list;
      *sizeptr = size;
    }

    /* Add to list. */
    list[used++] = (pid_t)value;
  }
  if (errno) {
    const int saved_errno = errno;
    closedir(dir);
    errno = saved_errno;
    return (size_t)0;
  }
  if (closedir(dir)) {
    errno = EIO;
    return (size_t)0;
  }

  /* None? */
  if (used < 1) {
    errno = ESRCH;
    return (size_t)0;
  }

  /* Make sure there is room for a terminating (pid_t)0. */
  if (used >= size) {
    size = used + 1;
    list = realloc(list, size * sizeof list[0]);
    if (!list) {
      errno = ENOMEM;
      return (size_t)0;
    }
    *listptr = list;
    *sizeptr = size;
  }

  /* Terminate list; done. */
  list[used] = (pid_t)0;
  errno = 0;
  return used;
}

/* returns the total number of pages
 *
 *
 *
 */
int read_maps(int maps_fd, process_mem_info_t *ps) {
#if XDEBUG_LVL <= LVL_DBG
  char dbg_buffer[BUFSIZ];
#endif
  char buffer[BUFSIZ];
  int total_num_pages = 0;
  int total_num_writable_pages = 0;
  int offset = 0;
  int region_num_pages = 0;
  int index = 0;
  int lib_name_size = 0;
  map_t map;
  for (;;) {
    ssize_t length = read(maps_fd, buffer + offset, sizeof buffer - offset);
    if (length <= 0) {
      break;
    }

    length += offset;

    for (size_t i = offset; i < (size_t)length; i++) {
      uint64_t low = 0, high = 0;
      if (buffer[i] == '\n' && i) {
        size_t x = i - 1;
        while (x && buffer[x] != '\n')
          x--;
        if (buffer[x] == '\n')
          x++;
        size_t beginning = x;

        low = parse_hex2dec_untill(buffer, '-', &x, sizeof buffer);

        while (buffer[x] != '-' && x + 1 < sizeof buffer)
          x++;

        if (buffer[x] == '-')
          x++;

        high = parse_hex2dec_untill(buffer, ' ', &x, sizeof buffer);

        memset(&map, 0, sizeof(map));
        map.addr_start = low;
        map.addr_end = high;

        region_num_pages = (high - low) / PSIZE;
        // assert( (high-low) % PSIZE == 0);
        map.total_num_pages = region_num_pages;

        // skip 1 space
        x++;
        map.r = buffer[x++] == 'r';
        map.w = buffer[x++] == 'w';
        map.x = buffer[x++] == 'x';
        map.p = buffer[x++] == 'p';

        x++; // skip space
        map.offset = parse_hex2dec_untill(buffer, ' ', &x, sizeof buffer);
        x++; // skip space
        map.dev_major = parse_hex2dec_untill(buffer, ':', &x, sizeof buffer);
        x++; // skip space

        map.dev_minor = parse_hex2dec_untill(buffer, ' ', &x, sizeof buffer);
        x++; // skip space

        map.inode = parse_dec_untill(buffer, ' ', &x, sizeof buffer);
        // skip spaces
        while (buffer[x] == ' ' && x + 1 < sizeof buffer)
          x++;

        const char *lib_name = 0;
        size_t y = x;
        while (buffer[y] != '\n' && y + 1 < sizeof buffer)
          y++;
        buffer[y] = 0;

        lib_name = buffer + x;
        lib_name_size = MIN(y - x, NAME_SIZE - 1);

        memcpy(map.name, lib_name, lib_name_size);
        map.name[lib_name_size] = 0;
        if (!memcmp(map.name, "[vvar]", strlen("[vvar]")) ||
            !memcmp(map.name, "[vdso]", strlen("[vdso]")) ||
            !memcmp(map.name, "[vsyscall]", strlen("[vsyscall]")))
          continue;

        // initialize map indexes early on
        map.first_page_index = total_num_pages;
        if (map.w)
          map.first_wmem_index = total_num_writable_pages;
        else
          map.first_wmem_index = -1;

        add_to_maps(ps, map);

        buffer[y] = '\n';

#if XDEBUG_LVL <= LVL_DBG
        // memset(dbg_buffer, 0, BUFSIZ);
        // memcpy(dbg_buffer, buffer+beginning, y-beginning);
        // jprint(LVL_EXP, "%s", dbg_buffer);
        print_maps_index(ps, index);
#endif
        total_num_pages += map.total_num_pages;
        if (map.w)
          total_num_writable_pages += map.total_num_pages;

        index++;
      }
    }
  }
  lseek(maps_fd, 0, SEEK_SET);
  return total_num_pages;
}

int read_range_pagemap(struct read_range_pagemap_arguments *threadFuncArgs) {
  int fd = threadFuncArgs->fd;
  process_mem_info_t *ps = threadFuncArgs->ps;
  map_t *map = threadFuncArgs->map;
  uint64_t map_index = threadFuncArgs->map_index;

  jprint(LVL_DBG, "fd %d, map_index %ld", fd, map_index);
  // assert(!memcmp(map, &ps->maps[map_index], sizeof(map_t)));
  uint64_t addr_start = map->addr_start;
  uint64_t addr_end = map->addr_end;

  uint64_t index_start = (addr_start / PSIZE) * sizeof(uint64_t);
  uint64_t index_end = (addr_end / PSIZE) * sizeof(uint64_t);
  uint64_t num_entries = (addr_end - addr_start) / PSIZE;
  uint64_t num_pages = map->total_num_pages;
  uint64_t *pagemap_bits = malloc(num_pages * sizeof(uint64_t));
  if (num_entries != num_pages) {
    jprint(LVL_EXP, "BUG: num_entries is %lu while we have %lu pages",
           num_entries, num_pages);
  }
  int r = pread(fd, pagemap_bits, num_entries * sizeof(uint64_t), index_start);
  if (r != num_entries * sizeof(uint64_t)) {
    // we cannot read the vsyscall data
    // need to be atomic in case of multithreading
    // ps->pages_max_capacity -= map->total_num_pages;
    //__atomic_fetch_sub(&ps->pages_max_capacity, map->total_num_pages,
    //__ATOMIC_SEQ_CST);
    return -1;
  }

  uint64_t curr_data;
  for (uint64_t i = 0; i < num_pages; i++) {
    curr_data = pagemap_bits[i];
    page_t curr_page;
    memset(&curr_page, 0, sizeof(curr_page));
    curr_page.addr_start = addr_start + i * PSIZE;
    curr_page.page_bits = curr_data;
    curr_page.pfn = GET_PAGE_PFN(curr_data);
    curr_page.soft_dirty =
        IS_PAGE_SOFT_DIRTY(curr_data); //|| IS_PAGE_FREED(curr_data);
    curr_page.file_or_anon = IS_PAGE_FILE_OR_ANON(curr_data);
    curr_page.exclusive = IS_PAGE_EXCLUSIVE(curr_data);
    curr_page.swapped = IS_PAGE_IN_SWAP(curr_data);
    curr_page.present = IS_PAGE_PRESENT(curr_data);
    curr_page.map_index = map_index;
    curr_page.is_freed = IS_PAGE_FREED(curr_data);

    add_page_to_map(ps, curr_page);
#if XDEBUG_LVL <= LVL_DBG
    print_map(map);
    print_page(&curr_page, map->name);
#endif
  }
  __atomic_fetch_add(&ps->pages_count, num_pages, __ATOMIC_SEQ_CST);
  __atomic_fetch_add(&ps->paged_pages_count, map->total_num_paged_pages,
                     __ATOMIC_SEQ_CST);
  free(pagemap_bits);
  return num_pages;
}

int populate_memory_copy_info_map(
    struct populate_memory_copy_info_arguments *threadFuncArgs) {
  int mem_fd = threadFuncArgs->mem_fd;
  process_mem_info_t *ps = threadFuncArgs->ps;
  map_t *curr_map = threadFuncArgs->curr_map;

#if XDEBUG_LVL <= LVL_DBG
  iprintl(LVL_INFO,
          "Storing WMEM: pid:%d, map_start %lu, map_end %lu, total_num_pages "
          "%ld,  wmem_addr %lu, wmem_size %lu",
          ps->pid, curr_map->addr_start, curr_map->addr_end,
          curr_map->total_num_pages, curr_map->first_wmem_index * PSIZE,
          PSIZE * ps->writable_pages_count);
#endif
  jprint(LVL_DBG, "bytes %ld == numpages*PSIZE %ld ",
         curr_map->addr_end - curr_map->addr_start,
         curr_map->total_num_pages * PSIZE);
  assert(curr_map->addr_end - curr_map->addr_start ==
         curr_map->total_num_pages * PSIZE);
  int r = 0;
  int new_r = 0;
  jprint(LVL_DBG, "COMP: map_start %p #PagedPages %ld",
         (void *)curr_map->addr_start, curr_map->total_num_paged_pages);

  if (curr_map->total_num_paged_pages == curr_map->total_num_pages) {
    jprint(LVL_DBG, "COMP: storing memory of whole map %p at wmem %p ",
           (void *)curr_map->addr_start,
           (void *)ps->wmem + curr_map->first_wmem_index * PSIZE);
    do {
      new_r = pread(mem_fd, ps->wmem + curr_map->first_wmem_index * PSIZE + r,
                    curr_map->total_num_pages * PSIZE - r,
                    curr_map->addr_start + r);
      r += new_r;
    } while (r != curr_map->total_num_pages * PSIZE && new_r > 0);
  } else {
    int paged_index = 0;
    for (int i = 0; i < curr_map->total_num_pages; i++) {
      page_t p = ps->pages[curr_map->first_page_index + i];
      if (p.pfn > 0) {
        jprint(LVL_DBG, "COMP: storing memory of page %p map %p at wmem %p ",
               (void *)p.addr_start, (void *)curr_map->addr_start,
               (void *)ps->wmem + curr_map->first_wmem_index * PSIZE +
                   (paged_index * PSIZE));
        pread(mem_fd,
              ps->wmem + curr_map->first_wmem_index * PSIZE +
                  (paged_index * PSIZE),
              PSIZE, curr_map->addr_start + PSIZE * i);
        paged_index += 1;
      } else {
        jprint(LVL_DBG, "COMP: not storing memory of page %p map %p",
               (void *)p.addr_start, (void *)curr_map->addr_start);
      }
    }
  }
  // if (new_r <= 0)
  //	assert(new_r > 0);
}
