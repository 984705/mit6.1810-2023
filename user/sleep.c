#include "kernel/types.h"
#include "user/user.h"

int 
main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(2, "Usage: sleep [times]\n");
    exit(1);
  }
  int times = atoi(argv[1]);
  exit(sleep(times));
}