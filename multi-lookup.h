#include <util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <queue.h>
#include <sempahore.h>

// Macros
#define ARRAY_SIZE 10
#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

struct requesterArgs {
    queue_t* inputFileQueue;
    char* outputFilename;
    sem_t *sem;
    p_thread_mutex_t *mutex;
    p_thread_mutex_t *requesterFileMutex;
    p_thread_mutex_t *requesterThreadMutex;
    char* hostnamesArray[ARRAY_SIZE];
    int arrayHead;
    int arrayTail;
};

struct resolverArgs {
    char* outputFilename;
    sem_t *sem;
    p_thread_mutex_t *arrayMutex;
    p_thread_mutex_t *resolverFileMutex;
    char* hostnamesArray[ARRAY_SIZE];
    int arrayHead;
    int arrayTail;
};