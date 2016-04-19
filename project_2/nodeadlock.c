#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/wait.h>

typedef struct {
  int count;
  int thread_id;
  int ready;
} pthread_monitor;

static pthread_monitor first_monitor;
static pthread_monitor second_monitor;

long nodeadlock_init(int thread_id, int index) {
  if (index == 0) {
    first_monitor.count = 0;
    first_monitor.thread_id = thread_id;
    first_monitor.ready = 1;

    second_monitor.count = 0;
    second_monitor.thread_id = -1;
    second_monitor.ready = 0;

    printk(KERN_INFO "nodeadlock_init called with thread_id = %d, index = %d\n",
      thread_id, index);
    return 0;
  } else if (index == 1) {
    second_monitor.count = 0;
    second_monitor.thread_id = thread_id;
    second_monitor.ready = 1;
    
    printk(KERN_INFO "nodeadlock_init called with thread_id = %d, index = %d\n",
      thread_id, index);
    return 0;
  } 
  return -1;
}

pthread_monitor *get_thread_by_id(int thread_id) {
  if (first_monitor.thread_id == thread_id) {
    return &first_monitor;
  } else if (second_monitor.thread_id == thread_id) {
    return &second_monitor;
  }
  return NULL;
}

pthread_monitor *get_other_thread_by_id(int thread_id) {
  if (first_monitor.thread_id == thread_id) {
    return &second_monitor;
  } else if (second_monitor.thread_id == thread_id) {
    return &first_monitor;
  } else {
    return NULL;
  }
}

long nodeadlock_mutex_lock(int this_thread_id) {
  pthread_monitor *this_monitor, *other_monitor;
  this_monitor = NULL;
  other_monitor = NULL;

  mutex_lock(&count_mutex); // Lock MUST be used instead of trylock
  
  this_monitor = get_thread_by_id(this_thread_id);
  other_monitor = get_other_thread_by_id(this_thread_id);

  if (this_monitor == NULL || other_monitor == NULL) {
    mutex_unlock(&count_mutex); // Unlock
    return -1;
  } 
  
  while (other_monitor->count > 0 || other_monitor->ready == 0) {
    // wait_event_interruptible(wq, (other_monitor->count > 0));    
    other_monitor = get_other_thread_by_id(this_thread_id);
  
    if (other_monitor == NULL) {
      mutex_unlock(&count_mutex); // Unlock
      return -1;
    }  
  }
  
  this_monitor->count += 1;
    
  mutex_unlock(&count_mutex); // Unlock
  return 0;
}

long nodeadlock_mutex_unlock(int this_thread_id) {
  pthread_monitor *this_monitor;
  this_monitor = NULL;

  mutex_lock(&count_mutex); // Lock 
  
  this_monitor = get_thread_by_id(this_thread_id);
  
  if (this_monitor == NULL) {
    mutex_unlock(&count_mutex); // Unlock
    return -1;
  } 
  
  this_monitor->count -= 1;
  
  if (this_monitor->count == 0) {
    // wake_up_interruptible(&wq);
  }
    
  mutex_unlock(&count_mutex); // Unlock
  return 0;
}

asmlinkage long sys_nodeadlock(const char *action, int thread_id, int index)
{
  if (strcmp(action, "init") == 0) {
    return nodeadlock_init(thread_id, index);
  } else if (strcmp(action, "lock") == 0) {
    return nodeadlock_mutex_lock(thread_id);
  } else if (strcmp(action, "unlock") == 0) {
    return nodeadlock_mutex_unlock(thread_id);
  }
  return -1;
}

