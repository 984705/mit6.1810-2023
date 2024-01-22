#include "kernel/types.h"
#include "user/user.h"

__attribute__((noreturn))
void run_child(int* parent_pipe) {
  close(parent_pipe[1]);
  int n;
  if(read(parent_pipe[0], &n, sizeof(int)) == 0) 
  {
    close(parent_pipe[0]);
    exit(0);
  }
  int child_pipe[2];
  pipe(child_pipe);
  if (fork() == 0) {
    close(parent_pipe[0]);
    run_child(child_pipe);
  }
  else {
    close(child_pipe[0]);
    printf("prime %d \n", n);
    int temp = n;
    while (read(parent_pipe[0], &n, sizeof(int)) != 0) 
    {
      if (n % temp != 0) {
        write(child_pipe[1], &n, sizeof(int));
      }
    }
    close(child_pipe[1]);
    wait(0);
  }
  exit(0);
}

int main(int argc, char* argv[]) {
  int parent_pipe[2]; 
  pipe(parent_pipe);

  if (fork() == 0) 
  {
    run_child(parent_pipe);
  }
  else 
  {
    close(parent_pipe[0]);
    for (int i = 2; i <= 35; ++i) 
    {
      write(parent_pipe[1], &i, sizeof(int));
    }
    close(parent_pipe[1]);
    wait(0);
  }
  exit(0);
}