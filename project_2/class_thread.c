#include "class_thread.h"

#define NUM_THREAD 2
#define TRUE 1
#define FALSE 0

static pthread_mutex_t count_mutex;
static pthread_cond_t count_cond;

typedef struct {
  int count;
  int thread_id;
} pthread_monitor;

static pthread_monitor thread_monitor[NUM_THREAD];

void nodeadlock_init(int thread_id, int index) {
  if (index == 0 || index == 1) {
    thread_monitor[index].count = 0;
    thread_monitor[index].thread_id = thread_id;
  }
}

pthread_monitor *get_thread_by_id(int thread_id) {
  int i;
  for (i = 0; i < NUM_THREAD; i++) {
    if (thread_monitor[i].thread_id == thread_id) {
      return &thread_monitor[i];
    }
  }
  return NULL;
}

pthread_monitor *get_other_thread_by_id(int thread_id) {
  int i;
  unsigned int found = FALSE;
  for (i = 0; i < NUM_THREAD; i++) {
    if (thread_monitor[i].thread_id == thread_id) {
      found = TRUE;
      break;
    }
  }
  
  if (found && i == 0) return &thread_monitor[1];
  else if (found && i == 1) return &thread_monitor[0];  
  else return NULL;
}

void nodeadlock_mutex_lock(int this_thread_id) {
  pthread_mutex_lock(&count_mutex); // Lock 
  
  pthread_monitor *this_monitor = get_thread_by_id(this_thread_id);
  pthread_monitor *other_monitor = get_other_thread_by_id(this_thread_id);
  
  if (this_monitor == NULL || other_monitor == NULL) {
    printf("ERROR: one of the monitors is NULL\n");
    exit(1);
  }   
  
  while (other_monitor->count > 0) {
    assert(this_monitor->count == 0);
    pthread_cond_wait(&count_cond, &count_mutex);
    other_monitor = get_other_thread_by_id(this_thread_id);
  
    if (this_monitor == NULL || other_monitor == NULL) {
      printf("ERROR: one of the monitors is NULL\n");
      exit(1);
    }  
  }
  
  this_monitor->count += 1;
    
  pthread_mutex_unlock(&count_mutex); // Unlock
}

void nodeadlock_mutex_unlock(int this_thread_id) {
  pthread_mutex_lock(&count_mutex); // Lock 
  
  pthread_monitor *this_monitor = get_thread_by_id(this_thread_id);
  pthread_monitor *other_monitor = get_other_thread_by_id(this_thread_id);
  
  if (this_monitor == NULL || other_monitor == NULL) {
    printf("ERROR: one of the monitors is NULL\n");
    exit(1);
  }
  
  assert(this_monitor->count == 1 || this_monitor->count == 2);
  assert(other_monitor->count == 0);
  
  this_monitor->count -= 1;
  
  if (this_monitor->count == 0) {
    assert(other_monitor->count == 0);
    pthread_cond_signal(&count_cond);
  }
    
  pthread_mutex_unlock(&count_mutex); // Unlock
}

void nodeadlock(char *action, int  *thread_id, int index) {
  if (strcmp(action, "init") == 0) {
    nodeadlock_init(*thread_id, index);
  } else if (strcmp(action, "lock") == 0) {
    nodeadlock_mutex_lock(*thread_id);
  } else if (strcmp(action, "unlock") == 0) {
    nodeadlock_mutex_unlock(*thread_id);
  } 
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

  nodeadlock("lock", (int *) &thread_id, -1 /* Unusued arg */);
  
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
  
  nodeadlock("unlock", (int *) &thread_id, -1 /* Unusued arg */);
  
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
  nodeadlock("init", (int *) &temp_pthread, index++);

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

