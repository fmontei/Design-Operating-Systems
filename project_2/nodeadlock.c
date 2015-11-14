#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/wait.h>

static struct mutex count_mutex;
static DECLARE_WAIT_QUEUE_HEAD(wq);

typedef struct {
  int count;
  int thread_id;
  int valid;
} pthread_monitor;

static pthread_monitor thread_monitor[2];

long nodeadlock_init(int thread_id, int index) {
  if (index == 0) {
    mutex_init(&count_mutex);
  }
  if (index == 0 || index == 1) {
    thread_monitor[index].count = 0;
    thread_monitor[index].thread_id = thread_id;
    thread_monitor[index].valid = 1;
    printk(KERN_INFO "nodeadlock_init called with thread_id = %d, index = %d\n",
      thread_id, index);
    return 0;
  }
  return -1;
}

pthread_monitor get_thread_by_id(int thread_id) {
  pthread_monitor invalid;
  int i;
  for (i = 0; i < 2; i++) {
    if (thread_monitor[i].thread_id == thread_id) {
      return thread_monitor[i];
    }
  }
  invalid.valid = 0;
  return invalid;
}

pthread_monitor get_other_thread_by_id(int thread_id) {
  pthread_monitor invalid;
  int i;
  unsigned int found = 0;
  for (i = 0; i < 2; i++) {
    if (thread_monitor[i].thread_id == thread_id) {
      found = 1;
      break;
    }
  }
  
  if (found == 1 && i == 0) {
    return thread_monitor[1];
  } else if (found == 1 && i == 1) {
    return thread_monitor[0];
  } else {
    invalid.valid = 0;
    return invalid;
  }
}

long nodeadlock_mutex_lock(int this_thread_id) {
  pthread_monitor this_monitor, other_monitor;

  mutex_trylock(&count_mutex); // Lock 
  
  this_monitor = get_thread_by_id(this_thread_id);
  other_monitor = get_other_thread_by_id(this_thread_id);
  
  if (this_monitor.valid == 0 || other_monitor.valid == 0) {
    // printk(KERN_INFO "ERROR: one of the monitors is NULL\n");
    mutex_unlock(&count_mutex); // Unlock
    return -1;
  }   
  
  while (other_monitor.count > 0) {
    // pthread_cond_wait(&count_cond, &count_mutex);
    wait_event_interruptible(wq, (other_monitor.count > 0));    
    other_monitor = get_other_thread_by_id(this_thread_id);
  
    if (other_monitor.valid == 0) {
      // printk(KERN_INFO "ERROR: one of the monitors is NULL\n");
      mutex_unlock(&count_mutex); // Unlock
      return -1;
    }  
  }
  
  this_monitor.count += 1;
    
  mutex_unlock(&count_mutex); // Unlock
  return 0;
}

long nodeadlock_mutex_unlock(int this_thread_id) {
  pthread_monitor this_monitor, other_monitor;  

  mutex_trylock(&count_mutex); // Lock 
  
  this_monitor = get_thread_by_id(this_thread_id);
  other_monitor = get_other_thread_by_id(this_thread_id);
  
  if (this_monitor.valid == 0 || other_monitor.valid == 0) {
    // printk(KERN_INFO "ERROR: one of the monitors is NULL\n");
    mutex_unlock(&count_mutex); // Unlock
    return -1;
  }
  
  this_monitor.count -= 1;
  
  if (this_monitor.count == 0) {
    // pthread_cond_signal(&count_cond);
    wake_up_interruptible(&wq);
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
