#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char* argv[]) 
{
  int p2c[2], c2p[2];
  char buf[1] = {'#'};
  pipe(p2c);
  pipe(c2p);
  if (fork() == 0) {
    close(p2c[1]);
    close(c2p[0]);
    write(c2p[1], buf, sizeof(buf));
    if (read(p2c[0], buf, sizeof(buf)) != sizeof(buf)){
      fprintf(2, "fail to read from buffer-p2c");
      exit(1);
    }
    fprintf(1, "%d: received ping\n", getpid());
  }
  else {
    close(p2c[0]);
    close(c2p[1]);
    if (read(c2p[0], buf, sizeof(buf)) != sizeof(buf)) {
      fprintf(2, "fail to read from buffer-c2p");
      exit(1);
    }
    write(p2c[1], buf, sizeof(buf));
    wait(0);
    fprintf(1, "%d: received pong\n", getpid());
  }
  exit(0);
}