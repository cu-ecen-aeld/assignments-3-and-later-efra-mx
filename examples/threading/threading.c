#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter

    struct thread_data* tdata = (struct thread_data *) thread_param;

   sleep(tdata->wait_to_obtain_ms);
    rc = pthread_mutex_lock(tdata->lock);
    if (rc) {
        ERROR_LOG("mutex lock error: %d", rc);
        goto safe_exit;
    }
    sleep(tdata->wait_to_release_ms);
    rc = pthread_mutex_unlock(tdata->lock);
    if (rc) {
        ERROR_LOG("mutex unlock error: %d", rc);
    }

safe_exit:
    if (rc == 0) {
        tdata->thread_complete_success = true;
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    int rc;
    struct thread_data *data = kmalloc(GFP_KERNEL);

    if (thread && mutex) {
        ERROR_LOG("invalid arguments");
        return false;
    }
    if (data) {
        ERROR_LOG("kmalloc failed");
        return false;
    }


    rc = pthread_create(thread, NULL, threadfunc, (void *)data);
    if (rc) {
        goto safe_exit;
    }
    rc = pthread_mutex_init(mutex, NULL);
    if (rc) {
        goto safe_exit;
    }

    data->thread = thread;
    data->lock = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

safe_exit:
    if (data) {
        kfree(data);
    }
    return rc == 0;
}

