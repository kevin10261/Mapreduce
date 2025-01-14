#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "threadpool.h"
#include "partition.h"


ThreadPool_t *ThreadPool_create(unsigned int num){
    ThreadPool_t *tp = (ThreadPool_t *)malloc(sizeof(ThreadPool_t));
    if (!tp) return NULL; // Error Check if allocation fails it returns NULL

    // Initialize thread tp data
    tp->threads = (pthread_t *)malloc(sizeof(pthread_t) * num);
    if (!tp->threads) {
        free(tp);
        return NULL;
    }

    tp->num_threads = num; //Setting number of threads to num_threads for access across other functions.
    tp->jobs.head = NULL; //Setting the first job queue to NULL
    tp->jobs.size = 0; //Amount of jobs currently
    tp->idle_threads = 0;
    tp->stop = false;



    pthread_mutex_init(&(tp->jobs.lock), NULL); // Mutex Lock Creation
    // pthread_mutex_init(&(tp->t_lock), NULL);
    pthread_cond_init(&(tp->jobs.cond), NULL); // Cond variable Creation Allow Waiting for jobs

    // Create worker threads
    for (unsigned int i = 0; i < num; i++) {
        int result = pthread_create(&(tp->threads[i]), NULL, (void*)Thread_run, tp);
        if (result != 0) {
            fprintf(stderr, "Error creating thread %u\n", i);
            tp->num_threads = i; // Limit number of threads to successfully created ones
            break;
        }
    }

    return tp;
}

void ThreadPool_destroy(ThreadPool_t *tp){
    pthread_mutex_lock(&(tp->jobs.lock));
    tp->stop = true;  // Set stop flag to true to stop the threads
    pthread_cond_broadcast(&(tp->jobs.cond));  // Wake up all waiting threads
    pthread_mutex_unlock(&(tp->jobs.lock));

    // Wait for all threads to complete their jobs and exit
    for (unsigned int i = 0; i < tp->num_threads; i++) {
        pthread_join(tp->threads[i], NULL);
    }

    // Free resources
    pthread_mutex_destroy(&(tp->jobs.lock));
    pthread_cond_destroy(&(tp->jobs.cond));
    free(tp->threads);
    free(tp);
}

static int get_job_length(void *arg) {
    struct stat st;
    if (stat((char *)arg, &st) == 0) {
        return (int)st.st_size;  // File size as job length
    }
    return 1;  // Default to 1 if unable to retrieve size
}


bool ThreadPool_add_job(ThreadPool_t *tp, thread_func_t func, void *arg){
    ThreadPool_job_t *job =(ThreadPool_job_t *)malloc(sizeof(ThreadPool_job_t));
    if(!job) return false;

    job->func = func;
    job->arg = arg;
    job->length = 0;
    job->next = NULL;
    int check = get_job_length(arg);



    pthread_mutex_lock(&(tp->jobs.lock));
    tp->jobs.size++;
    if(check != 1){
        job->length = get_job_length(arg);
    // Insert job in sorted order (SJF)
        if (tp->jobs.head == NULL || tp->jobs.head->length > job->length) {
            job->next = tp->jobs.head;
            tp->jobs.head = job;  // First job in the queue
        } else {
            ThreadPool_job_t *current = tp->jobs.head;
            while (current->next != NULL && current->next->length <= job->length) {
                current = current->next;  // Traverse to the end of the queue
            }
            job->next = current->next;
            current->next = job;  // Append the new job at the end
        }
    }else if(check == 1){
        job->length = ((KeyValuePartition*)job->arg)->size;

        if(tp->jobs.head == NULL || tp->jobs.head->length > job->length){
            job->next = tp->jobs.head;
            tp->jobs.head = job;
        }else {
            ThreadPool_job_t *current = tp->jobs.head;
            while (current->next != NULL && current->next->length <= job->length) {
                current = current->next;  // Traverse to the end of the queue
            }
            job->next = current->next;
            current->next = job;  // Append the new job at the end
        }


        
    }
    

    // Signal an idle thread that a new job is available
    pthread_mutex_unlock(&(tp->jobs.lock));
    pthread_cond_signal(&(tp->jobs.cond));


    return true;
}


ThreadPool_job_t *ThreadPool_get_job(ThreadPool_t *tp){
  

    // Get the next job from the queue (head of the queue)
    ThreadPool_job_t *job;

    while(tp->jobs.size == 0 && !tp->stop){
        pthread_cond_signal(&(tp->jobs.cond));
        pthread_cond_wait(&(tp->jobs.cond), &(tp->jobs.lock));
    }
    if(tp->stop)return NULL;

    if(tp->jobs.size > 0){
        job = tp->jobs.head;
        tp->jobs.head = job->next;  // Update the head to the next job
        tp->jobs.size--;
    }

    return job;
}



void *Thread_run(ThreadPool_t *tp){
    while (1) {
        pthread_mutex_lock(&(tp->jobs.lock)); //Give lock
        if (tp->stop) {
            pthread_mutex_unlock(&(tp->jobs.lock));
            pthread_exit(NULL);
        }

        // Wait for a job or signal to stop
        ThreadPool_job_t *job = ThreadPool_get_job(tp);
        tp->idle_threads++;
        pthread_mutex_unlock(&(tp->jobs.lock));

        // Run the job if it exists
        if (job) {
            job->func(job->arg);
            pthread_mutex_lock(&(tp->jobs.lock));
            tp->idle_threads--;
            pthread_mutex_unlock(&(tp->jobs.lock));
            free(job);
        }

    }
    return NULL;

}


void ThreadPool_check(ThreadPool_t *tp){
    pthread_mutex_lock(&(tp->jobs.lock));

    // Wait until all jobs are done
    while (tp->jobs.size != 0 || tp->idle_threads != 0) {
        // Only wait on jobs.lock and ensure other mutexes are free
        pthread_cond_wait(&(tp->jobs.cond), &(tp->jobs.lock));
    }

    pthread_mutex_unlock(&(tp->jobs.lock));
}
