#include <linux/kernel.h>
#include <linux/syscalls.h>

asmlinkage long sys_nodeadlock(const char *action, int thread_id, int index, int *retval)
{
  return 0;
}

