#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "queue.h"
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>

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
    pthread_mutex_t *arrayMutex;
    pthread_mutex_t *requesterFileMutex;
    pthread_mutex_t *requesterQueueMutex;
    int* sharedArrayHead;
    int* sharedArrayTail;
	int* activeRequesters;
	char* *sharedArray[ARRAY_SIZE];
};

struct resolverArgs {
    char* outputFilename;
    sem_t *sem;
    pthread_mutex_t *arrayMutex;
    pthread_mutex_t *resolverFileMutex;
    int* sharedArrayHead;
    int* sharedArrayTail;
	int* activeRequesters;
	int* activeResolvers;
	char* *sharedArray[ARRAY_SIZE];
};
