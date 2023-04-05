#include "multi-lookup.h"

// Global variables
pthread_mutex_t arrayMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t requesterFileMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t resolverFileMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t requesterQueueMutex = PTHREAD_MUTEX_INITIALIZER;
sem_t arraySem;
int sharedArrayHead = 0;
int sharedArrayTail = 0;
int activeRequesters = 0;
int activeResolvers = 0;
int arrayAmount = 0;
char* sharedArray[ARRAY_SIZE];
char* resOutputFileName;
char* reqOutputFileName;


void *requesterFunction (void *arg) {
    struct requesterArgs args = *(struct requesterArgs*) arg;
	activeRequesters++;
    int fileCount = 0;
    char fileName[MAX_NAME_LENGTH];
    while (1) {
        pthread_mutex_lock(&requesterQueueMutex);
        if (queue_isEmpty(args.inputFileQueue)) {
            pthread_mutex_unlock(&requesterQueueMutex);
            break;
        }
        strcpy(fileName, queue_pop(args.inputFileQueue));
		pthread_mutex_unlock(&requesterQueueMutex);
        FILE *inputFile = fopen(fileName, "r");
        if (inputFile == NULL) {
            fprintf(stderr, "invalid file %s\n", fileName);
        } else {
			fileCount++;
		    char *line = NULL;
		    size_t len = 0;
		    ssize_t read;
		    while ((read = getline(&line, &len, inputFile)) != -1) {
		        while (arrayAmount >= 10) {
		            // Wait until there is space in the array
		        }
                pthread_mutex_lock(&requesterFileMutex);
                fprintf(args.reqOutputFile, "%s", line);
                pthread_mutex_unlock(&requesterFileMutex);
                line[strcspn(line, "\r\n")] = '\0';
		        pthread_mutex_lock(&arrayMutex);
                memcpy(sharedArray[sharedArrayHead], line, MAX_NAME_LENGTH * sizeof(char));
                arrayAmount++;
                sharedArrayTail++;
				if (sharedArrayTail >= ARRAY_SIZE) {
					sharedArrayTail = 0;
				}		
		        pthread_mutex_unlock(&arrayMutex);
		    }
            fclose(inputFile);
		}
    }
	activeRequesters -= 1;
	fprintf(stdout, "Thread: %lu serviced %d files\n", pthread_self(), fileCount);
    return NULL;
}

void *resolverFunction (void *arg) {
    FILE *outputFile = (FILE*)arg;
	activeResolvers++;
	int servicedNames = 0;
    char line[MAX_NAME_LENGTH];
    char resolvedIP[MAX_IP_LENGTH];
    while (1) {
        if ((arrayAmount == 0) && (activeRequesters == 0)) {
            break;
        }
        pthread_mutex_lock(&arrayMutex);
        memcpy(line, sharedArray[sharedArrayHead], MAX_NAME_LENGTH * sizeof(char));
        arrayAmount--;
        sharedArrayHead++;
		if (sharedArrayHead >= ARRAY_SIZE) {
			sharedArrayHead = 0;
		}
        pthread_mutex_unlock(&arrayMutex);
        if (strcmp(line, "") == 0) {
            continue;
        }
        if (dnslookup(line, resolvedIP, MAX_IP_LENGTH) == UTIL_FAILURE) {
            strcpy(resolvedIP, "NOT_RESOLVED");
            servicedNames--;
        }
        pthread_mutex_lock(&resolverFileMutex);
        fprintf(outputFile, "%s, %s\n", line, resolvedIP);
        pthread_mutex_unlock(&resolverFileMutex);
        servicedNames++;
    }
	activeResolvers -= 1;
	fprintf(stdout, "Thread: %lu resolved %d hostnames\n", pthread_self(), servicedNames);
	return NULL;
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
    reqOutputFileName = argv[3];
    resOutputFileName = argv[4];
    for (int i = 0; i < ARRAY_SIZE; i++) {
        sharedArray[i] = malloc(sizeof(char*) * MAX_NAME_LENGTH);
    }
    queue_t *inputFileQueue = queue_create();
    for (int i = 5; i < argc; i++) {
        queue_push(inputFileQueue, argv[i]);
    }
    FILE *resOutputFile = fopen(resOutputFileName, "w");
    FILE *reqOutputFile = fopen(reqOutputFileName, "w");
    sem_init(&arraySem, 0, 0);
    pthread_t requesterThreads[numRequesters];
    pthread_t resolverThreads[numResolvers];
    struct requesterArgs *requesterArgs = (struct requesterArgs *)malloc(sizeof(struct requesterArgs));
    requesterArgs->inputFileQueue = inputFileQueue;
    requesterArgs->reqOutputFile = reqOutputFile;


    // Create the requester threads
    for (int i = 0; i < numRequesters; i++) {
        pthread_create(&requesterThreads[i], NULL, requesterFunction, requesterArgs);
    }

    // Create the resolver threads
    for (int i = 0; i < numResolvers; i++) {
        pthread_create(&resolverThreads[i], NULL, resolverFunction, resOutputFile);
    }
    for (int i = 0; i < numRequesters; i++) {
		pthread_join(requesterThreads[i], NULL);
	}
    // while (activeResolvers != 0) {
    //     // wait
    // }
    for (int i = 0; i < numResolvers; i++) {
		pthread_join(resolverThreads[i], NULL);
	}
    for (int i = 0; i < ARRAY_SIZE; i++) {
        free(sharedArray[i]);
    }
    free(requesterArgs);
    fclose(resOutputFile);
    fclose(reqOutputFile);
    pthread_mutex_destroy(&arrayMutex);
    pthread_mutex_destroy(&requesterFileMutex);
    pthread_mutex_destroy(&resolverFileMutex);
    pthread_mutex_destroy(&requesterQueueMutex);
    sem_destroy(&arraySem);
	queue_destroy(inputFileQueue);
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
