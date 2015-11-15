#include "class_thread.h"
#include <unistd.h>

typedef struct {
  int count;
  int thread_id;
} pthread_monitor;

static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t count_cond;
// static DECLARE_WAIT_QUEUE_HEAD(wq);
static pthread_monitor first_monitor;
static pthread_monitor second_monitor;

long nodeadlock_init(int thread_id, int index) {
  if (index == 0) {
    first_monitor.count = 0;
    first_monitor.thread_id = thread_id;

    second_monitor.count = 0;
    second_monitor.thread_id = -1;

    // Wait for other thread to initialize
    pthread_mutex_lock(&count_mutex);
    while (1)
      pthread_cond_wait(&count_cond, &count_mutex);
    pthread_mutex_unlock(&count_mutex);

    printf("nodeadlock_init called with thread_id = %d, index = %d\n",
      thread_id, index);
    return 0;
  } else if (index == 1) {
    second_monitor.count = 0;
    second_monitor.thread_id = thread_id;
    
    // Now that the second thread is initialized, unlock the first
    pthread_mutex_lock(&count_mutex);
    pthread_cond_signal(&count_cond);
    pthread_mutex_unlock(&count_mutex);
    
    printf("nodeadlock_init called with thread_id = %d, index = %d\n",
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

  pthread_mutex_lock(&count_mutex); // Lock MUST be used instead of trylock
  
  this_monitor = get_thread_by_id(this_thread_id);
  other_monitor = get_other_thread_by_id(this_thread_id);

  if (this_monitor == NULL || other_monitor == NULL) {
    pthread_mutex_unlock(&count_mutex); // Unlock
    return -1;
  } 
  
  while (other_monitor->count > 0) {
    pthread_cond_wait(&count_cond, &count_mutex);
    // wait_event_interruptible(wq, (other_monitor->count > 0));    
    other_monitor = get_other_thread_by_id(this_thread_id);
  
    if (other_monitor == NULL) {
      pthread_mutex_unlock(&count_mutex); // Unlock
      return -1;
    }  
  }
  
  this_monitor->count += 1;
    
  pthread_mutex_unlock(&count_mutex); // Unlock
  return 0;
}

long nodeadlock_mutex_unlock(int this_thread_id) {
  pthread_monitor *this_monitor;
  this_monitor = NULL;

  pthread_mutex_trylock(&count_mutex); // Lock 
  
  this_monitor = get_thread_by_id(this_thread_id);
  
  if (this_monitor == NULL) {
    pthread_mutex_unlock(&count_mutex); // Unlock
    return -1;
  } 
  
  this_monitor->count -= 1;
  
  if (this_monitor->count == 0) {
    pthread_cond_signal(&count_cond);
    // wake_up_interruptible(&wq);
  }
    
  pthread_mutex_unlock(&count_mutex); // Unlock
  return 0;
}

int nodeadlock(char *action, int thread_id, int index) {
  printf("Calling nodeadlock with action = %s\n", action);
  //if (action != NULL && thread_id != NULL) {
    //int retval = syscall(318, action, &thread_id, index);
  //}
  if (strcmp(action, "init") == 0) {
    return nodeadlock_init(thread_id, index);
  } else if (strcmp(action, "lock") == 0) {
    return nodeadlock_mutex_lock(thread_id);
  } else if (strcmp(action, "unlock") == 0) {
    return nodeadlock_mutex_unlock(thread_id);
  }
  return -1;
}

int allocate_mutex(class_mutex_t *cmutex)
{
  return 0;
}

int allocate_cond(class_condit_ptr ccondit)
{
  if(!ccondit->condition)
  {
    ccondit->condition = malloc(sizeof(pthread_cond_t));
    
    if(!ccondit->condition)
    {
      fprintf(stderr, "Error: malloc failed to allocate space for condition var!\n");
      return -1;
    }
  }
}


int class_mutex_init(class_mutex_ptr cmutex)
{
  if(pthread_mutex_init(&(cmutex->mutex), NULL))
  {
    fprintf(stderr, "Error: pthread mutex initialization failed!\n");
    return -1;
  }
  printf("mutex init ok\n");
  return 0;
}

int class_mutex_destroy(class_mutex_ptr cmutex)
{
  if(pthread_mutex_destroy(&cmutex->mutex))
  {
    fprintf(stderr, "Error: pthread mutex destruction failed!\n");
    return -1;
  }

  return 0;
}

int class_cond_init(class_condit_ptr ccondit)
{

  if(pthread_cond_init(ccondit->condition, NULL))
  {
    fprintf(stderr, "Error: pthread condition initialization failed!\n");
    return -1;
  }

  return 0;
}

int class_cond_destroy(class_condit_ptr ccondit)
{
  if(pthread_cond_destroy(ccondit->condition))
  {
    fprintf(stderr, "Error: pthread condition destruction failed!\n");
    return -1;
  }

  return 0;
}


int class_mutex_lock(class_mutex_ptr cmutex)
{

  pthread_t thread_id = pthread_self();
  printf("THREAD ID (LOCK) = %d\n", (int) thread_id);

  int retval = nodeadlock("lock", (int) thread_id, -1 /* Unusued arg */);
  if (retval == -1) {
    printf("Failed, terminating.\n");
    exit(1);
  }
  
  if(pthread_mutex_lock(&cmutex->mutex))
  {
    fprintf(stdout, "Error: pthread mutex lock failed!\n");
    return -1;
  }

  return 0;
}


int class_mutex_unlock(class_mutex_ptr cmutex)
{
  pthread_t thread_id = pthread_self();
  printf("THREAD ID (UNLOCK) = %d\n", (int) thread_id);
  
  int retval = nodeadlock("unlock", (int) thread_id, -1 /* Unusued arg */);
  if (retval == -1) {
    printf("Failed, terminating.\n");
    exit(1);
  }
  
  if(pthread_mutex_unlock(&cmutex->mutex))
  {
    fprintf(stdout, "Error: pthread mutex unlock failed!\n");
    return -1;
  }
  
  return 0;
}

/* Two threads and two mutexes are initially created */
int class_thread_create(class_thread_t * cthread, void *(*start)(void *), void * arg, int * num_of_mutexes)
{
  static int index = 0; 
  pthread_t temp_pthread;
  if(pthread_create(&temp_pthread, NULL, start, arg))
  {
    fprintf(stderr, "Error: Failed to create pthread!\n");
    return -1;
  }

  //Hacking a bit to get everything working correctly
  memcpy(cthread, &temp_pthread, sizeof(pthread_t));
  printf("THREAD ID CREATE = %d\n", (int) temp_pthread);
  int retval = nodeadlock("init", (int) temp_pthread, index++);
  if (retval == -1) {
    printf("Failed, terminating.\n");
    exit(1);
  }

  return 0;
}
  
int class_thread_join(class_thread_t cthread, void ** retval)
{ 
  pthread_t temp_pthread;
  memcpy(&temp_pthread, &cthread, sizeof(pthread_t));

  if(pthread_join(temp_pthread, retval))
  {
    fprintf(stderr, "Error: failed to join the pthread!\n");
    return -1;
  }

  return 0;
}

int class_thread_cond_wait(class_condit_ptr ccondit, class_mutex_ptr cmutex)
{
  if(pthread_cond_wait(ccondit->condition, &cmutex->mutex))
  {
    fprintf(stderr, "Error: pthread cond wait failed!\n");
    return -1;
  }

  return 0;
}

int class_thread_cond_signal(class_condit_ptr ccondit)
{
  if(pthread_cond_signal(ccondit->condition))
  {
    fprintf(stderr, "Error: pthread cond signal failed!\n");
    return -1;
  }

  return 0;
}

