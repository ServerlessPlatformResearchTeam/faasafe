#include "stat_dump.h"

/**
 * @brief Get the memory usage object
 *
 * @return int
 */
int get_memory_usage() {
  int pid = getpid();
  char memcmd[BUFSIZ];
  sprintf(memcmd, "echo $(cat /proc/%d/status | grep HWM | tr -dc '0-9')", pid);
  char buf[10];
  FILE *fp;
  int maxrss = 0;

  if ((fp = popen(memcmd, "r")) == NULL) {
    printf("Error opening pipe!\n");
    return -1;
  }
  while (fgets(buf, 10, fp) != NULL) {
    maxrss = atoi(buf);
  }
  if (pclose(fp)) {
    printf("Command not found or exited with error status\n");
    return -1;
  }
  return maxrss;
}

void fill_json_stats_buffers(void) {
#if PRINT_JSON_STATS
  XSTATS_ADD_COUNT_POINT(groundhog_memory_footprint, get_memory_usage());
  begin_json_stats();
  write_all_stats_as_json();
  end_json_stats();
#endif
}
/**
 * @brief Get current memory and print all stats in json
 *
 */
void dump_stats(void) {
#if PRINT_JSON_STATS
  fill_json_stats_buffers();
  print_json_stats();
#endif
}

void dump_json_stats_to_file(int argc, char *argv[]) {

  // Create a directory based on arg2
  FILE *fp;
  char of[512];
  char dir_make[512];
  sprintf(dir_make, "mkdir -p %s", argv[2]);
  mkdir(dir_make, 0777);

  // name the file according to the runtime used and script, else assume C
  // language
  if (argc >= 5)
    sprintf(of, "%s/%s_%s.json", argv[2], argv[3], basename(argv[4]));
  else
    sprintf(of, "%s/C_%s.json", argv[2], basename(argv[3]));
  fp = fopen(of, "w");
  if (fp == NULL) {
    jprint(LVL_EXP, "ERROR opening file! %s \n", of);
    exit(1);
  }
  jprint(LVL_EXP, "Writing JSON stats to %s", of);

  // end the json object and write it to file
  fill_json_stats_buffers();
  write_json_stats(fp);
  fclose(fp);
}