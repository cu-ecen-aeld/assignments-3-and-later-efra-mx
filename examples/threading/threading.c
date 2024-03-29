#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter

    struct thread_data* tdata = (struct thread_data *) thread_param;
    int rc;

    DEBUG_LOG("[%d] waiting", tdata->id);
    usleep(tdata->wait_to_obtain_ms * 1000);
    rc = pthread_mutex_lock(tdata->lock);
    if (rc) {
        ERROR_LOG("mutex lock error: %d", rc);
        goto safe_exit;
    }
    DEBUG_LOG("[%d] locked", tdata->id);
    usleep(tdata->wait_to_release_ms * 1000);
    rc = pthread_mutex_unlock(tdata->lock);
    if (rc) {
        ERROR_LOG("mutex unlock error: %d", rc);
    }
    DEBUG_LOG("[%d] released", tdata->id);

safe_exit:
    if (rc == 0) {
        tdata->thread_complete_success = true;
    }

    DEBUG_LOG("[%d] completed", tdata->id);
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
    static int id = 0;
    int rc;
    struct thread_data *data = malloc(sizeof(struct thread_data));

    if (thread == NULL || mutex == NULL) {
        ERROR_LOG("invalid arguments");
        return false;
    }
    if (data == NULL) {
        ERROR_LOG("malloc failed");
        return false;
    }

    rc = pthread_create(thread, NULL, threadfunc, (void *)data);
    if (rc) {
        ERROR_LOG("[%d] failed to create thread", data->id);
        goto safe_exit;
    }

    data->id = id++;
    data->thread = thread;
    data->lock = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;
    DEBUG_LOG("[%d] created", data->id);
    return true;

safe_exit:
    free(data);

    return false;
}

