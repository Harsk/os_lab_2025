#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
  pid_t pid = fork();
  if (pid == 0) {
    printf("Child %d exits\n", getpid());
    exit(0);
  }
  printf("Parent %d sleeps, child is %d\n", getpid(), pid);
  sleep(60);  // родитель не вызывает wait()
  return 0;
}
