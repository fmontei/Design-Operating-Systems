#ifndef CLASS_THREAD_H
#define CLASS_THREAD_H

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct _class_thread_condit
{
  pthread_cond_t * condition;
} * class_condit_ptr, class_cond_t;

typedef struct _class_thread_mutex
{
  pthread_mutex_t mutex;
} * class_mutex_ptr, class_mutex_t;

typedef pthread_t class_thread_t;

extern int allocate_mutex(class_mutex_ptr cmutex);
extern int allocate_cond(class_condit_ptr ccondit);


// Returns 0 on success, -1 on failure
extern int class_mutex_init(class_mutex_ptr cmutex);

extern int class_mutex_destroy(class_mutex_ptr cmutex);

extern int class_cond_init(class_condit_ptr ccondit);

extern int class_cond_destroy(class_condit_ptr ccondit);



extern int class_mutex_lock(class_mutex_ptr cmutex);

extern int class_mutex_unlock(class_mutex_ptr cmutex);

extern int class_thread_create(class_thread_t * cthread, void *(*start)(void *), void * arg, int * num_of_mutexes);

extern int class_thread_join(class_thread_t cthread, void ** retval);

extern int class_thread_cond_wait(class_condit_ptr ccondit, class_mutex_ptr cmutex);

extern int class_thread_cond_signal(class_condit_ptr cond);


#endif
