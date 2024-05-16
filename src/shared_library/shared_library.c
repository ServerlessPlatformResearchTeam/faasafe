#include "shared_library.h"

static char *acks = NULL;
static size_t ackssize = 0;
static FILE *stdcmdin = NULL;
static FILE *stdcmdout = NULL;

static void open_pipes() {
  if (stdcmdin == NULL) {
    stdcmdin = fdopen(4, "r");
    stdcmdout = fdopen(5, "w");
  }
}
static void close_pipes() {
  if (stdcmdin != NULL) {
    fclose(stdcmdin);
    fclose(stdcmdout);
    free(acks);
  }
}

// shared library에서 제공하는 함수(명령어) 총 3개
// checkpoint : 현재상태를 저장
// rewind : 저장된 상태로 복원
// dump_stats : faasafe 상태 출력

int faasafe_checkpoint() {
  open_pipes();
  fprintf(stdcmdout, "faasafe_checkpoint\n");
  fflush(stdcmdout);
  getline((char **)&acks, &ackssize, stdcmdin);
  return 0;
}

int faasafe_rewind() {
  fprintf(stdcmdout, "faasafe_rewind\n");
  fflush(stdcmdout);
  getline((char **)&acks, &ackssize, stdcmdin);
  return 0;
}

int faasafe_dump_stats() {
  fprintf(stdcmdout, "faasafe_dump_stats\n");
  fflush(stdcmdout);
  getline((char **)&acks, &ackssize, stdcmdin);
  return 0;
}
