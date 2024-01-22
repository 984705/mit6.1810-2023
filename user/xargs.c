#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

char* 
readline() 
{
  char* buf = malloc(MAXARG);
  char* pos = buf;
  while(read(0, pos, sizeof(char)) != 0) {
    if (*pos == '\n' || *pos == '\0') {
      *pos = '\0';
      return buf;
    }
    pos++;
  }
  if (pos != buf) return buf;
  free(buf);
  return 0;
}

int
main(int argc, char* argv[]) 
{
  if (argc < 2) {
    fprintf(2, "Usage: xargs [commend]\n");
    exit(1);
  }

  char** real_argv;
  char* real_cmd;
  char* left_argv;

  real_cmd = argv[1];
  real_argv = malloc(argc * sizeof(char*));
  for (int i = 0; i < argc - 1; ++i)
    real_argv[i] = argv[i + 1];

  while((left_argv = readline()) != 0) {
    if (fork() == 0) {
      real_argv[argc - 1] = left_argv;
      exec(real_cmd, real_argv);
    }
    else
      wait(0);
  }
  exit(0);
}