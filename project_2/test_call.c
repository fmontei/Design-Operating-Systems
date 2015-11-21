#include <stdio.h>
#include <stdlib.h>

int nodeadlock(char *action, int thread_id, int index) {
  int retval = 0;
  printf("Calling nodeadlock with thread_id = %d\n", thread_id);
  if (action != NULL) {
    syscall(318, action, thread_id, index, &retval);
    if (retval == -1) {
      printf("CRITICAL ERROR, TERMINATING.\n");
      exit(1);
    }
  }
  return retval;
}

int main(void)
{
  int retval = 0;
  retval = nodeadlock("init", 12345, 0);
  printf("retval init = %d\n", retval);
  retval = nodeadlock("lock", 12345, -1);
  printf("retval lock = %d\n", retval);
  retval = nodeadlock("lock", 12345, -1);
  printf("retval lock = %d\n", retval);
  retval = nodeadlock("unlock", 12345, -1);
  printf("retval unlock = %d\n", retval);
  retval = nodeadlock("unlock", 12345, -1);
  printf("retval unlock = %d\n", retval);
  retval = nodeadlock("fail", 12345, 0);
  printf("retval fail = %d\n", retval);
  return 0;
}

