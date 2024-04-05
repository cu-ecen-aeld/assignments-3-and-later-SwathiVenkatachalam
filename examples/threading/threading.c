#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

#define SUCCESS (1)
#define FAILURE (0)
#define MILLISEC_IN_SEC (1000)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    //1. Obtain thread args from parameter like above
    struct thread_data* thread_args = (struct thread_data*) thread_param;
    
    //2. Wait
    // Ref: https://www.man7.org/linux/man-pages/man3/sleep.3.html
    int wait_to_obtain_s = thread_args->wait_to_obtain_ms/MILLISEC_IN_SEC;
    int ret_val = sleep(wait_to_obtain_s);
    if(ret_val != 0)
    {
        ERROR_LOG("Error: Failure of sleep before obtaining mutex\n");
        thread_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Success: Waited for %d seconds before obtaining mutex!\n", wait_to_obtain_s);
    
    //3. Obtain lock
    ret_val = pthread_mutex_lock(thread_args->mutex);
    if(ret_val != 0)
    {
        ERROR_LOG("Error: Failed to obtain lock using pthread_mutex_lock\n");
        thread_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Success: Obtained lock!\n");
    
    //3. Hold
    int wait_to_release_s = thread_args->wait_to_release_ms/MILLISEC_IN_SEC;
    ret_val = sleep(wait_to_release_s);
    if(ret_val != 0)
    {
        ERROR_LOG("Error: Failure of sleep while holding mutex\n");
        thread_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Success: Held for %d seconds before releasing mutex!\n", wait_to_release_s);
    
    //4. Release lock
    ret_val = pthread_mutex_unlock(thread_args->mutex);
    if(ret_val != 0)
    {
        ERROR_LOG("Error: Failed to release lock using pthread_mutex_unlock\n");
        thread_args->thread_complete_success = false;
        return thread_param;
    }
    DEBUG_LOG("Success: Released lock!\n");    
    
    thread_args->thread_complete_success = true;
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
     
    // 1. Dynamic allocation of thread_data struct to be passed into thread
    struct thread_data* thread_args = (struct thread_data*) malloc(sizeof(struct thread_data));
    if(thread_args == NULL)
    {
        ERROR_LOG("Error: Failure of thread_args dynamic memory allocation using malloc\n");
        return FAILURE;
    }
    
    // 2. Get struct member values
    thread_args->mutex = mutex;
    thread_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_args->wait_to_release_ms = wait_to_release_ms;
    thread_args->thread_complete_success = false;
    
    // 3. Create thread using threadfunc and thread_args parameters
    int rc = pthread_create(thread,
                            NULL, // Use default attributes
                            threadfunc,
                            thread_args);
    if (rc != 0) //returns 0 on success
    {
        ERROR_LOG("Error: Failure of thread creation using pthread_create\n");
        free(thread_args); //free memory allocated
        return FAILURE;       
    }                   
     
    DEBUG_LOG("Success: Thread created!\n");
    return SUCCESS;
}

