#include "multi-lookup.h"

// Global variables


// Creates a thread that reads from file in folder and writes to shared array and writes to file in folder
// Repeats until all files in folder have been read

void *requesterFunction (void *arg) {
    // Read from file in folder
    // Write to shared array
    // Write to file in folder
    // Repeat until all files in folder have been read
    struct requesterArgs *args = (struct requesterArgs*) arg;
	args->activeRequesters ++;
    int fileCount = 0;
    while (1) {
        pthread_mutex_lock(args->requesterQueueMutex);
        if (queue_isEmpty(args->inputFileQueue)) {
            pthread_mutex_unlock(args->requesterQueueMutex);
            break;
        }
		char* fileName = queue_pop(args->inputFileQueue);
		// fileName = queue_pop(args->inputFileQueue);
		//memcpy(&fileName, queue_pop(args->inputFileQueue), MAX_NAME_LENGTH);
		// strcpy(queue_pop(args->inputFileQueue), fileName);
		pthread_mutex_unlock(args->requesterQueueMutex);
        FILE *inputFile = fopen(fileName, "r");
        if (inputFile == NULL) {
            fprintf(stderr, "invalid file %s\n", fileName);
        } else {
			fileCount++;
		    char *line = NULL;
		    size_t len = 0;
		    ssize_t read;
		    while ((read = getline(&line, &len, inputFile)) != -1) {
		        // Remove newline character
		        line[strlen(line) - 1] = '\0';
		        // Write to shared array
		        while (args->sharedArrayTail - args->sharedArrayHead == ARRAY_SIZE - 1) {
		            // Wait until there is space in the array
		        }
				if ((sizeof(line) / sizeof(char)) > MAX_NAME_LENGTH) {
					fprintf(stderr, "Website Name too long");
					continue;
				}
		        pthread_mutex_lock(args->arrayMutex);
		        *args->sharedArrayTail++;
				if (*args->sharedArrayTail >= ARRAY_SIZE) {
					*args->sharedArrayTail = 0;
				}
				//strcpy(*args->sharedArray[*args->sharedArrayTail], line);
				*args->sharedArray[*args->sharedArrayTail] = line;
				
		        pthread_mutex_unlock(args->arrayMutex);
		        sem_post(args->sem);
		    }
		}
    }
	args->activeRequesters -= 1;
	fprintf(stdout, "Thread: %lu serviced %d files\n", pthread_self(), fileCount);
}

void *resolverFunction (void *arg) {
    // Read from shared array
    // Use utility function to resolve IP address
    // Write to file in folder
    // Repeat until all files in folder have been read
    struct resolverArgs *args = (struct resolverArgs*) arg;
	args->activeResolvers ++;
	int servicedNames = 0;
    while (1) {
        sem_wait(args->sem);
        pthread_mutex_lock(args->arrayMutex);
        char *line = args->sharedArray[*args->sharedArrayHead];
        *args->sharedArrayHead++;
		if (*args->sharedArrayHead >= ARRAY_SIZE) {
			*args->sharedArrayHead = 0;
		}
        pthread_mutex_unlock(args->arrayMutex);
        if ((args->sharedArrayHead == args->sharedArrayTail) && args->activeRequesters == 0) {
			// done
            break;
        }
        // Use utility function to resolve IP address
        char *resolvedIP = malloc(100);
        if (dnslookup(line, resolvedIP, 100) == UTIL_FAILURE) {
            fprintf(stderr, "Error resolving IP address\n");
            exit(1);
        }
        // Write to file in folder
        pthread_mutex_lock(args->resolverFileMutex);
        FILE *outputFile = fopen(args->outputFilename, "a");
        if (outputFile == NULL) {
            fprintf(stderr, "Error opening file\n");
            exit(1);
        }
        fprintf(outputFile, "%s,%s\n", line, resolvedIP);
        fclose(outputFile);
		servicedNames++;
        pthread_mutex_unlock(args->resolverFileMutex);
    }
	args->activeResolvers -= 1;
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
    char *reqOutputFile = argv[3];
    char *resOutputFile = argv[4];
    queue_t *inputFilenames = queue_create();
    for (int i = 5; i < argc; i++) {
        queue_push(inputFilenames, argv[i]);
    }
    char* sharedArray[ARRAY_SIZE];
    int sharedArrayHead = 0;
    int sharedArrayTail = 1;
	int activeRequesters = 0;
	int activeResolvers = 0;
    pthread_mutex_t arrayMutex, requesterFileMutex, resolverFileMutex, requesterQueueMutex;
    // Initialize mutexes
    pthread_mutex_init(&arrayMutex, NULL);
    pthread_mutex_init(&requesterFileMutex, NULL);
    pthread_mutex_init(&resolverFileMutex, NULL);
    pthread_mutex_init(&requesterQueueMutex, NULL);
    sem_t arraySem;
    // Initialize semaphores
    sem_init(&arraySem, 0, 0);
    pthread_t requesterThreads[numRequesters];
    pthread_t resolverThreads[numResolvers];

    // Create the input struct for the requester threads
    struct requesterArgs *requesterArgs = (struct requesterArgs *)malloc(sizeof(struct requesterArgs));
    requesterArgs->inputFileQueue = inputFilenames;
    requesterArgs->outputFilename = reqOutputFile;
	memcpy(&requesterArgs->sharedArray, &sharedArray, sizeof(ARRAY_SIZE));
    requesterArgs->sharedArrayHead = &sharedArrayHead;
    requesterArgs->sharedArrayTail = &sharedArrayTail;
    requesterArgs->arrayMutex = &arrayMutex;
    requesterArgs->requesterFileMutex = &requesterFileMutex;
    requesterArgs->requesterQueueMutex = &requesterQueueMutex;
    requesterArgs->sem = &arraySem;
	requesterArgs->activeRequesters = &activeRequesters;

    // Create the input struct for the resolver threads
    struct resolverArgs *resolverArgs = (struct resolverArgs *)malloc(sizeof(struct resolverArgs));
    resolverArgs->outputFilename = resOutputFile;
	memcpy(&resolverArgs->sharedArray, &sharedArray, sizeof(ARRAY_SIZE));
    resolverArgs->sharedArrayHead = &sharedArrayHead;
    resolverArgs->sharedArrayTail = &sharedArrayTail;
    resolverArgs->arrayMutex = &arrayMutex;
    resolverArgs->resolverFileMutex = &resolverFileMutex;
    resolverArgs->sem = &arraySem;
	resolverArgs->activeRequesters = &activeRequesters;
	resolverArgs->activeResolvers = &activeResolvers;

    // Create the requester threads
    for (int i = 0; i < numRequesters; i++) {
        pthread_create(&requesterThreads[i], NULL, requesterFunction, (void*) &requesterArgs);
    }

    // Create the resolver threads
    for (int i = 0; i < numResolvers; i++) {
        pthread_create(&resolverThreads[i], NULL, resolverFunction, (void*) &resolverArgs);
    }
	while (activeRequesters != 0) {
		// wait
	}
	for (int i = 0; i < numRequesters; i++) {
		pthread_join(requesterThreads[i], NULL);
	}
	for (int i = 0; i < numResolvers; i++) {
		pthread_join(resolverThreads[i], NULL);
	}
	queue_destroy(inputFilenames);
	gettimeofday(&endTime, NULL);
	struct timeval exeTime;
	exeTime.tv_sec = endTime.tv_sec - startTime.tv_sec;
	exeTime.tv_usec = endTime.tv_usec - startTime.tv_usec;
	fprintf(stdout, "./multi-lookup: total time is %ld.%ld seconds\n", exeTime.tv_sec, exeTime.tv_usec);
}
