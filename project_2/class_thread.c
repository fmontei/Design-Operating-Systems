#include "class_thread.h"

#define MAX_COUNT 2
#define NUM_THREAD 2

static pthread_mutex_t count_mutex;
static pthread_cond_t count_cond;

typedef struct {
  int count;
  pthread_t thread_id;
  bool initialized;
} pthread_monitor;

static pthread_monitor thread_monitor[NUM_THREAD] = 
 {{.count = 0, .initialized = false}, 
  {.count = 0, .initialized = false}};

void init_thread_monitor(pthread_t thread_id) {
  if (!thread_monitor[0].initialized) {
    thread_monitor[0].count = 0;
    thread_monitor[0].thread_id = thread_id;
    thread_monitor[0].initialized = true;
  } else if (!thread_monitor[1].initialized) {
    thread_monitor[1].count = 0;
    thread_monitor[1].thread_id = thread_id;
    thread_monitor[1].initialized = true;
  }
}

pthread_monitor *get_thread_by_id(pthread_t thread_id) {
  int i;
  for (i = 0; i < NUM_THREAD; i++) {
    if (thread_monitor[i].thread_id == thread_id) {
      return &thread_monitor[i];
    }
  }
  return NULL;
}

pthread_monitor *get_other_thread_by_id(pthread_t thread_id) {
  int i;
  bool found = false;
  for (i = 0; i < NUM_THREAD; i++) {
    if (thread_monitor[i].thread_id == thread_id) {
      found = true;
      break;
    }
  }
  
  if (found && i == 0) return &thread_monitor[1];
  else if (found && i == 1) return &thread_monitor[0];  
  else return NULL;
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

  int thread_id = pthread_self();
  printf("THREAD ID (LOCK) = %d\n", thread_id);

  pthread_mutex_lock(&count_mutex); // Lock 
  
  pthread_t this_thread_id = pthread_self();
  pthread_monitor *this_monitor = get_thread_by_id(this_thread_id);
  pthread_monitor *other_monitor = get_other_thread_by_id(this_thread_id);
  
  if (other_monitor == NULL) {
    printf("ERROR: IS NULL\n");
    exit(1);
  } 
  
  printf("OTHER COUNT = %d\n", other_monitor->count);
  
  while (other_monitor->count > 0) {
    printf("WAITING...\n");
    //exit(1);
    pthread_cond_wait(&count_cond, &count_mutex);
    other_monitor = get_other_thread_by_id(this_thread_id);
  
    if (other_monitor == NULL) {
      printf("ERROR: IS NULL\n");
      exit(1);
    }  
  }
  
  this_monitor->count += 1;
  
  // printf("THIS COUNT = %d\n", this_monitor->count);
    
  pthread_mutex_unlock(&count_mutex); // Unlock
  
  if(pthread_mutex_lock(&cmutex->mutex))
  {
    fprintf(stdout, "Error: pthread mutex lock failed!\n");
    return -1;
  }

  return 0;
}


int class_mutex_unlock(class_mutex_ptr cmutex)
{
  int thread_id = pthread_self();
  printf("THREAD ID (UNLOCK) = %d\n", thread_id);
  
  pthread_mutex_lock(&count_mutex); // Lock 
  
  pthread_t this_thread_id = pthread_self();
  pthread_monitor *this_monitor = get_thread_by_id(this_thread_id);
  pthread_monitor *other_monitor = get_other_thread_by_id(this_thread_id);
  
  if (this_monitor == NULL || other_monitor == NULL) {
    printf("ERROR: IS NULL\n");
    exit(1);
  } 
  
  assert(this_monitor->count == 1 || this_monitor->count == 2);
  assert(other_monitor->count == 0);
  
  this_monitor->count -= 1;
  
  printf("HI\n");
  
  if (this_monitor->count == 0) {
    printf("SIGNALLING...\n");
    //exit(1);
    pthread_cond_signal(&count_cond);
  }
  
  // printf("THIS COUNT = %d\n", this_monitor->count);
    
  pthread_mutex_unlock(&count_mutex); // Unlock
  
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
  pthread_t temp_pthread;
  if(pthread_create(&temp_pthread, NULL, start, arg))
  {
    fprintf(stderr, "Error: Failed to create pthread!\n");
    return -1;
  }

  //Hacking a bit to get everything working correctly
  memcpy(cthread, &temp_pthread, sizeof(pthread_t));
  printf("THREAD ID CREATE = %d\n", (int) temp_pthread);
  init_thread_monitor(temp_pthread);

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

