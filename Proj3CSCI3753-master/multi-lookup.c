#include "multi-lookup.h"

// Global variables
// static pthread_mutex_t arrayMutex = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t requesterFileMutex = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t resolverFileMutex = PTHREAD_MUTEX_INITIALIZER;
// static pthread_mutex_t requesterQueueMutex = PTHREAD_MUTEX_INITIALIZER;
// sem_t arraySem;
// queue_t *inputFileQueue;
// int sharedArrayHead = 0;
// int sharedArrayTail = 0;
// int activeRequesters = 0;
// int activeResolvers = 0;
// int arrayAmount = 0;
// char* sharedArray[ARRAY_SIZE];
// char* resOutputFileName;
// char* reqOutputFileName;
// FILE *resOutputFile;
// FILE *reqOutputFile;


void *requesterFunction (void *arg) {
    struct requesterArgs args = *(struct requesterArgs*) arg;
	args.activeRequesters++;
    int fileCount = 0;
    char fileName[MAX_NAME_LENGTH];
    while (1) {
        pthread_mutex_lock(args.requesterQueueMutex);
        if (queue_isEmpty(args.inputFileQueue)) {
            pthread_mutex_unlock(args.requesterQueueMutex);
            break;
        }
        strcpy(fileName, queue_pop(args.inputFileQueue));
		pthread_mutex_unlock(args.requesterQueueMutex);
        FILE *inputFile = fopen(fileName, "r");
        if (inputFile == NULL) {
            fprintf(stderr, "invalid file %s\n", fileName);
        } else {
			fileCount++;
		    char *line = NULL;
		    size_t len = 0;
		    ssize_t read;
		    while ((read = getline(&line, &len, inputFile)) != -1) {
		        while (*args.arrayAmount == 10) {
		            // Wait until there is space in the array
		        }
                pthread_mutex_lock(args.requesterFileMutex);
                fprintf(args.reqOutputFile, "%s", line);
                pthread_mutex_unlock(args.requesterFileMutex);
                line[strcspn(line, "\r\n")] = 0;
		        pthread_mutex_lock(args.arrayMutex);
				strncpy(*args.sharedArray[*args.sharedArrayTail], line, MAX_NAME_LENGTH * sizeof(char));
                *args.arrayAmount++;
                *args.sharedArrayTail++;
				if (*args.sharedArrayTail >= ARRAY_SIZE) {
					*args.sharedArrayTail = 0;
				}		
		        pthread_mutex_unlock(args.arrayMutex);
		        sem_post(args.sem);
		    }
            fclose(inputFile);
		}
    }
	*args.activeRequesters -= 1;
	fprintf(stdout, "Thread: %lu serviced %d files\n", pthread_self(), fileCount);
}

void *resolverFunction (void *arg) {
    // Read from shared array
    // Use utility function to resolve IP address
    // Write to file in folder
    // Repeat until all files in folder have been read
    struct resolverArgs args = *(struct resolverArgs*) arg;
	args.activeResolvers++;
	int servicedNames = 0;
    char line[MAX_NAME_LENGTH];
    char resolvedIP[MAX_IP_LENGTH];
    while (1) {
        pthread_mutex_lock(args.arrayMutex);
        if ((*args.arrayAmount == 0) && (args.activeRequesters == 0)) {
			// done
            pthread_mutex_unlock(args.arrayMutex);
            break;
        }
        pthread_mutex_unlock(args.arrayMutex);
        sem_wait(args.sem);
        pthread_mutex_lock(args.arrayMutex);
        strncpy(line, *args.sharedArray[*args.sharedArrayHead], MAX_NAME_LENGTH * sizeof(char));
        *args.arrayAmount--;
        *args.sharedArrayHead++;
		if (*args.sharedArrayHead >= ARRAY_SIZE) {
			*args.sharedArrayHead = 0;
		}
        pthread_mutex_unlock(args.arrayMutex);
        // Use utility function to resolve IP address
        if (dnslookup(line, resolvedIP, MAX_IP_LENGTH) == UTIL_FAILURE) {
            strcpy(resolvedIP, "NOT_RESOLVED");
            servicedNames--;
        }
        // Write to file in folder
        pthread_mutex_lock(args.resolverFileMutex);
        // print resolveip
        fprintf(args.resOutputFile, "%s, %s\n", line, resolvedIP);
        pthread_mutex_unlock(args.resolverFileMutex);
        servicedNames++;
    }
	*args.activeResolvers -= 1;
	fprintf(stdout, "Thread: %lu resolved %d hostnames\n", pthread_self(), servicedNames);
	
}

int main (int argc, char* argv[]) {
    if (argc <= 5) {
		fprintf(stderr, "Not enough arguments. Expected: ./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]\n");
		return 1;
	} else if (argc > 5 + MAX_INPUT_FILES) {
		fprintf(stderr, "Too many input files\n");
		return 1;
	}
	
	if (atoi(argv[1]) < 1) {
		fprintf(stderr, "Must have at least 1 requester thread\n");
		return 2;
	} else if (atoi(argv[1]) > MAX_REQUESTER_THREADS) {
		fprintf(stderr, "Too many requester threads. Max is %d\n", MAX_REQUESTER_THREADS);
		return 2;
	}

	if (atoi(argv[2]) < 1) {
		fprintf(stderr, "Must have at least 1 resolver thread\n");
		return 2;
	} else if (atoi(argv[2]) > MAX_RESOLVER_THREADS) {
		fprintf(stderr, "Too many resolver threads. Max is %d\n", MAX_RESOLVER_THREADS);
		return 2;
	}
	

	struct timeval startTime;
	struct timeval endTime;
	gettimeofday(&startTime, NULL);
	
    int numRequesters = atoi(argv[1]);
    int numResolvers = atoi(argv[2]);
    char* reqOutputFileName = argv[3];
    char* resOutputFileName = argv[4];
    queue_t *inputFilenames = queue_create();
    //inputFileQueue = queue_create();
    for (int i = 5; i < argc; i++) {
        queue_push(inputFilenames, argv[i]);
    }
    char** sharedArray = malloc(sizeof(char*) * ARRAY_SIZE);
    FILE *resOutputFile = fopen(resOutputFileName, "w");
    FILE *reqOutputFile = fopen(reqOutputFileName, "w");
    int *sharedArrayHead = malloc(sizeof(int));
    int *sharedArrayTail = malloc(sizeof(int));
	int *activeRequesters = malloc(sizeof(int));
	int *activeResolvers = malloc(sizeof(int));
    int *arrayAmount = malloc(sizeof(int));
    *sharedArrayHead = 0;
    *sharedArrayTail = 0;
    *arrayAmount = 0;
    *activeRequesters = 0;
    *activeResolvers = 0;
    // // Initialize mutexes
    pthread_mutex_t arrayMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t requesterFileMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t resolverFileMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t requesterQueueMutex = PTHREAD_MUTEX_INITIALIZER;
    // pthread_mutex_t *arrayMutex;
    // pthread_mutex_t *requesterFileMutex;
    // pthread_mutex_t *resolverFileMutex;
    // pthread_mutex_t *requesterQueueMutex;
    // pthread_mutex_init(arrayMutex, NULL);
    // pthread_mutex_init(requesterFileMutex, NULL);
    // pthread_mutex_init(resolverFileMutex, NULL);
    // pthread_mutex_init(requesterQueueMutex, NULL);
    sem_t arraySem;
    sem_init(&arraySem, 0, 0);
    pthread_t requesterThreads[numRequesters];
    pthread_t resolverThreads[numResolvers];

    // // Create the input struct for the requester threads
    struct requesterArgs *requesterArgs = (struct requesterArgs *)malloc(sizeof(struct requesterArgs));
	memcpy(&requesterArgs->sharedArray, &sharedArray, sizeof(ARRAY_SIZE));
    requesterArgs->inputFileQueue = inputFilenames;
    requesterArgs->sharedArrayHead = sharedArrayHead;
    requesterArgs->sharedArrayTail = sharedArrayTail;
    requesterArgs->arrayMutex = &arrayMutex;
    requesterArgs->requesterFileMutex = &requesterFileMutex;
    requesterArgs->requesterQueueMutex = &requesterQueueMutex;
    requesterArgs->sem = &arraySem;
	requesterArgs->activeRequesters = activeRequesters;
    requesterArgs->reqOutputFile = reqOutputFile;
    requesterArgs->arrayAmount = arrayAmount;

    // // Create the input struct for the resolver threads
    struct resolverArgs *resolverArgs = (struct resolverArgs *)malloc(sizeof(struct resolverArgs));
	memcpy(&resolverArgs->sharedArray, &sharedArray, sizeof(ARRAY_SIZE));
    resolverArgs->sharedArrayHead = sharedArrayHead;
    resolverArgs->sharedArrayTail = sharedArrayTail;
    resolverArgs->arrayMutex = &arrayMutex;
    resolverArgs->resolverFileMutex = &resolverFileMutex;
    resolverArgs->sem = &arraySem;
	resolverArgs->activeRequesters = activeRequesters;
	resolverArgs->activeResolvers = activeResolvers;
    resolverArgs->resOutputFile = resOutputFile;
    resolverArgs->arrayAmount = arrayAmount;

    // Create the requester threads
    for (int i = 0; i < numRequesters; i++) {
        pthread_create(&requesterThreads[i], NULL, requesterFunction, (void*) requesterArgs);
    }
    while (arrayAmount == 0) {
        // wait
    }
    // Create the resolver threads
    for (int i = 0; i < numResolvers; i++) {
        pthread_create(&resolverThreads[i], NULL, resolverFunction, (void*) resolverArgs);
    }
    fclose(resOutputFile);
    fclose(reqOutputFile);
    free(requesterArgs);
    free(resolverArgs);
    free(sharedArrayHead);
    free(sharedArrayTail);
    free(arrayAmount);
    free(activeRequesters);
    free(activeResolvers);
    pthread_mutex_destroy(&arrayMutex);
    pthread_mutex_destroy(&requesterFileMutex);
    pthread_mutex_destroy(&resolverFileMutex);
    pthread_mutex_destroy(&requesterQueueMutex);
    sem_destroy(&arraySem);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        free(sharedArray[i]);
    }
    free(sharedArray);
	queue_destroy(inputFilenames);
    for (int i = 0; i < numRequesters; i++) {
		pthread_join(requesterThreads[i], NULL);
	}
	for (int i = 0; i < numResolvers; i++) {
		pthread_join(resolverThreads[i], NULL);
	}
	gettimeofday(&endTime, NULL);
	struct timeval exeTime;
	exeTime.tv_sec = endTime.tv_sec - startTime.tv_sec;
	exeTime.tv_usec = endTime.tv_usec - startTime.tv_usec;
    // If the microseconds is negative, subtract a second and add 1000000 to the microseconds
    if (exeTime.tv_usec < 0) {
        exeTime.tv_sec--;
        exeTime.tv_usec += 1000000;
    }
	fprintf(stdout, "./multi-lookup: total time is %ld.%ld seconds\n", exeTime.tv_sec, exeTime.tv_usec);
}
