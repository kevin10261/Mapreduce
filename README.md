
# MapReduce Implementation

## Overview
This project implements a basic MapReduce framework with multi-threading for parallel processing. It follows the standard MapReduce model with Mapper and Reducer functions and introduces thread pooling for efficient task execution. The program processes multiple input files, performs a mapping operation on them, partitions the data, and then reduces the data based on partitioned keys.

## Synchronization Primitives Used

### Mutex Locks (`pthread_mutex`)
In this implementation, mutex locks were used to synchronize access to shared resources, specifically during the insertion of key-value pairs into partitions. As multiple threads can concurrently access the same partition, mutexes prevent race conditions when accessing and modifying the `KeyValuePartition` linked lists. I also used them for the threadpool.c file to make sure that there were no race conditions or deadlocks.

#### How Mutexes Are Used:
- Mutexes are used to lock the partition or a job pool during the insertion of new key-value pairs into the partition’s list. This ensures that only one thread can modify a partition’s data at a time, maintaining thread safety.

### Thread Pool
The Thread Pool is used to manage the distribution of tasks to worker threads. The thread pool allows for the parallel execution of both the map and reduce phases. The thread pool ensures efficient use of the available threads by managing job queueing and executing jobs as threads become available. This was all implemented in threadpool.c file.

#### How Thread Pool Is Used:
- The thread pool is initialized at the start of the `MR_Run` function with a specified number of worker threads. Tasks for the Mapper and Reducer are added to the thread pool, and workers are dispatched to process the tasks concurrently in Shortest Job First (SJF).

## Data Structures Used

### KeyValue Structure
The `KeyValue` structure stores key-value pairs for the MapReduce operation. Each key-value pair is represented by the following fields:

- `key`: A string representing the key for the key-value pair.
- `value`: A string representing the value for the key-value pair.
- `next_head`: A pointer to the next `KeyValue` in the same partition.
- `next_pair`: A pointer to another `KeyValue` pair for the same key in case there are multiple values for the same key.

### KeyValuePartition Structure
The `KeyValuePartition` structure represents a partition in the MapReduce process, which holds a list of key-value pairs and tracks the partition's index. It contains the following fields:

- `size`: The size of the partition (number of key-value pairs).
- `lock`: A mutex lock to synchronize access to the partition.
- `key_values`: A pointer to the linked list of `KeyValue` pairs stored in the partition another word for the head.
- `p_index`: The index of the partition in the system.

### ThreadPool Structure
The `ThreadPool` structure is used to manage a pool of worker threads. It contains the following:

- A queue of jobs waiting to be processed.
- A pool of worker threads that process the jobs concurrently.
- Synchronization mechanisms like mutexes and condition variables to manage the worker threads and the job queue.

## Partitioning Implementation
To divide the data into partitions for the reduce phase, the `MR_Partitioner` function hashes the key and maps it to a partition. This allows the system to distribute key-value pairs among multiple partitions efficiently. This is then sent to MR_EMIT where the MR_EMIT inserts the Keyvalue pair to the coressponding partiition

## How the Implementation Was Tested

### Testing implementions
To firstly test my code I implemented my own test cases with smaller amount of job since having lower amount of job is a good way to test the base of the code. After completing this step I moved on to test multiple jobs and to see if my code can handle jobs when its greater then 2 jobs so > 5 jobs. To test that implementions, I created a makefile to Compile all the .c files and then when I do make and then make run it will be able to run all the sample test cases given to us. And following with the normal testing I also used the valgrind to test the memory leak in my code `valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./wordcount`  using this command and this will give us where the error happend and how many error our program can contain. To bugfix my code I also printed many statements during my program to ensure proper key values being in the correct partitions.
  

## Conclusion
This MapReduce implementation provides a basic yet functional framework for parallel data processing using a thread pool and partitioned key-value storage. By leveraging synchronization primitives like mutexes, the program ensures thread safety during concurrent access to shared resources. The system has been thoroughly tested with a variety of scenarios to ensure its correctness and performance.
