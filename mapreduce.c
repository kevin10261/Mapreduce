#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mapreduce.h"
#include "threadpool.h"
#include "partition.h"

void Reduce(const char *key, int partition_index);


KeyValuePartition **partitions;
unsigned int num_partitions;

static ThreadPool_t *thread_pool;

int first_run = 1;


void MR_Run(unsigned int file_count, char *file_names[],
            Mapper mapper, Reducer reducer, 
            unsigned int num_workers, unsigned int num_parts) {

    num_partitions = num_parts;

    partitions = (KeyValuePartition **)malloc(num_parts * sizeof(KeyValuePartition *));

    for (unsigned int i = 0; i < num_parts; i++) {
        partitions[i] = (KeyValuePartition *)malloc(sizeof(KeyValuePartition));
        partitions[i]->size = 0;
        pthread_mutex_init(&(partitions[i]->lock), NULL);
        partitions[i]->key_values = NULL;
        partitions[i]->p_index = i;
    }

    // Create thread pool
    thread_pool = ThreadPool_create(num_workers);

    // Map Phase: Submit each file as a job to the thread pool
    for (unsigned int i = 0; i < file_count; i++) {
        ThreadPool_add_job(thread_pool, (thread_func_t)mapper, file_names[i]);
    }

    // Wait for all map tasks to complete
    ThreadPool_check(thread_pool);


    // Reduce Phase: Submit each partition as a reduce job
    for (int i = 0; i < num_parts; i++) {
        ThreadPool_add_job(thread_pool, (thread_func_t)MR_Reduce , (void *)partitions[i]);
    }

    // Wait for all reduce tasks to complete
    ThreadPool_check(thread_pool);

    // Destroy the thread pool and cleanup
    ThreadPool_destroy(thread_pool);

}


void MR_Emit(char *key, char *value) {

    unsigned int partition_num = MR_Partitioner(key, num_partitions);
    pthread_mutex_lock(&(partitions[partition_num]->lock));

    // Create a new KeyValue pair
    KeyValue *kv_pair = (KeyValue *)malloc(sizeof(KeyValue));
    kv_pair->key = strdup(key);
    kv_pair->value = strdup(value);
    kv_pair->next_head = NULL;
    kv_pair->next_pair = NULL;

    // Insert the pair into the partition in alphabetical order
    KeyValue *current = partitions[partition_num]->key_values;
    KeyValue *prev = NULL;

    if (current == NULL) {
        // No items in partition; add as the first item
        partitions[partition_num]->key_values = kv_pair;
    } else {
        // Traverse the list to find the correct position
        while (current) {
            if (strcmp(current->key, key) == 0) {
                // If the key matches, add to the next_pair chain
                kv_pair->next_pair = current->next_pair;
                current->next_pair = kv_pair;
                pthread_mutex_unlock(&(partitions[partition_num]->lock));
                return;
            } else if (strcmp(current->key, key) > 0) {
                // Insert kv_pair in correct order
                if (current == partitions[partition_num]->key_values) {
                    // Insert at the head if current is the first element
                    kv_pair->next_head = partitions[partition_num]->key_values;
                    partitions[partition_num]->key_values = kv_pair;
                } else {
                    // Insert between previous and current
                    kv_pair->next_head = current;
                    prev->next_head = kv_pair;
                }
                pthread_mutex_unlock(&(partitions[partition_num]->lock));
                return;
            }
            prev = current;
            current = current->next_head;
        }

        // If we reached the end of the list, insert at the end
        prev->next_head = kv_pair;
    }

    partitions[partition_num]->size++;
    pthread_mutex_unlock(&(partitions[partition_num]->lock));
}




unsigned int MR_Partitioner(char *key, unsigned int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
    hash = hash * 33 + c;
    return hash % num_partitions;
}


void MR_Reduce(void *threadarg) {
    KeyValue *current = ((KeyValuePartition *)threadarg)->key_values;
    if (current == NULL) return;
    
    unsigned int partition_index = ((KeyValuePartition*)threadarg)->p_index;
    char *key = NULL;

    while (current) {
        if (key == NULL) {
            key = (char *)malloc(strlen(current->key) + 1);
        } else if (strlen(current->key) + 1 > strlen(key)) {
            key = (char *)realloc(key, strlen(current->key) + 1);
        }

        if (key == NULL) {
            perror("Memory allocation failed");
            return;
        }

        strcpy(key, current->key);
        Reduce(key, partition_index);
        current = current->next_head;
    }
    
    free(key);

    // Free all key-value pairs after reduction is complete
    current = ((KeyValuePartition *)threadarg)->key_values;
    while (current) {
        KeyValue *temp = current;
        current = current->next_head;
        free(temp->key);
        free(temp->value);
        free(temp);
    }
}




char* MR_GetNext(char *key, unsigned int partition_idx) {
    pthread_mutex_lock(&(partitions[partition_idx]->lock));
    KeyValue *current = partitions[partition_idx]->key_values;

    // If there’s a `next_pair`, advance there for the next value
    KeyValue* ret = NULL;
    KeyValue* prev = NULL;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (current->next_pair == NULL) { // pointing to nothing, delete from partition
                // special case handling
                if (current == partitions[partition_idx]->key_values) // if current is head
                    partitions[partition_idx]->key_values = current->next_head; // make next one the new head
                else
                    prev->next_head = current->next_head; // tell the previous one to go to the next one over

                ret = current;
            } else { // normal case handling (no deletions)
                ret = current->next_pair;
                current->next_pair = ret->next_pair;
            }

            char* value = strdup(ret->value);
            // free(ret->key);
            // free(ret->value);
            // free(ret);
            pthread_mutex_unlock(&(partitions[partition_idx]->lock));
            return value;
        }
        prev = current;
        current = current->next_head;
    }

    pthread_mutex_unlock(&(partitions[partition_idx]->lock));
    return NULL;
}

