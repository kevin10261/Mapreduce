#include <pthread.h>
#include <stdbool.h>

typedef struct KeyValue {
    char *key;
    char *value;
    struct KeyValue *next_pair;
    struct KeyValue *next_head;
} KeyValue;

// Represents a partition containing key-value pairs with synchronization
typedef struct KeyValuePartition {
    KeyValue *key_values;      // Head of linked list for key-value pairs
    unsigned int size;         // Number of key-value pairs
    pthread_mutex_t lock;      // Mutex for synchronization
    unsigned int p_index;
} KeyValuePartition;



