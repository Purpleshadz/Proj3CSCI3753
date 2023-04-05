#include <multi-lookup.h>

// Global variables


// Creates a thread that reads from file in folder and writes to shared array and writes to file in folder
// Repeats until all files in folder have been read

void *requesterFunction (void *arg) {
    // Read from file in folder
    // Write to shared array
    // Write to file in folder
    // Repeat until all files in folder have been read
    struct requesterArgs *args = (struct requesterArgs*) arg;
    int fileCount = 0;
    while (1) {
        pthread_mutex_lock(&args->requesterQueueMutex);
        if (queue_isEmpty(args->inputFileQueue)) {
            pthread_mutex_unlock(&args->requesterQueueMutex);
            break;
        }
        FILE *inputFile = fopen(args->queue_pop(inputFileQueue), "r");
        pthread_mutex_unlock(&args->requesterQueueMutex);
        if (inputFile == NULL) {
            printf("Error opening file");
            exit(1);
        }
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
            pthread_mutex_lock(&args->arrayMutex);
            args->sharedArrayTail++;
            args->sharedArray[args->sharedArrayTail] = line;
            pthread_mutex_unlock(&args->arrayMutex);
            sem_post(&args->sem);
        }
    }
}

void *resolverFunction (void *arg) {
    // Read from shared array
    // Use utility function to resolve IP address
    // Write to file in folder
    // Repeat until all files in folder have been read
    struct requesterArgs *args = (struct requesterArgs*) arg;
    while (1) {
        sem_wait(&args->sem);
        pthread_mutex_lock(&args->arrayMutex);
        char *line = args->sharedArray[args->sharedArrayHead];
        args->sharedArrayHead++;
        pthread_mutex_unlock(&args->arrayMutex);
        if (line == NULL) {
            break;
        }
        // Use utility function to resolve IP address
        char *resolvedIP = malloc(100);
        if (dnslookup(line, resolvedIP, 100) == UTIL_FAILURE) {
            printf("Error resolving IP address");
            exit(1);
        }
        // Write to file in folder
        pthread_mutex_lock(&args->resolverFileMutex);
        FILE *outputFile = fopen(args->outputFilename, "a");
        if (outputFile == NULL) {
            printf("Error opening file");
            exit(1);
        }
        fprintf(outputFile, "%s,%s\n", line, resolvedIP);
        fclose(outputFile);
        pthread_mutex_unlock(&args->resolverFileMutex);
    }
}

int main () {
    if (argc != 5) {
        printf("Wrong number of input arguments");
        exit(1);
    }
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
    int sharedArrayTail = -1;
    pthread_mutex_t arrayMutex, requesterFileMutex, resolverFileMutex, requesterQueueMutex;
    // Initialize mutexes
    pthread_mutex_init(&arrayMutex, NULL);
    pthread_mutex_init(&requesterFileMutex, NULL);
    pthread_mutex_init(&resolverFileMutex, NULL);
    pthread_mutex_init(&requesterQueueMutex, NULL);
    sem_t arraySem
    // Initialize semaphores
    sem_init(&arraySem, 0, 0);
    pthread_t requesterThreads[numRequesters];
    pthread_t resolverThreads[numResolvers];

    // Create the input struct for the requester threads
    struct requesterArgs requesterArgs;
    requesterArgs.inputFilename = inputFilenames;
    requesterArgs.outputFilename = reqOutputFile;
    requesterArgs.sharedArray = sharedArray;
    requesterArgs.sharedArrayHead = sharedArrayHead;
    requesterArgs.sharedArrayTail = sharedArrayTail;
    requesterArgs.arrayMutex = arrayMutex;
    requesterArgs.requesterFileMutex = requesterFileMutex;
    requesterArgs.requesterQueueMutex = requesterQueueMutex;
    requesterArgs.sem = arraySem;

    // Create the input struct for the resolver threads
    struct resolverArgs resolverArgs;
    resolverArgs.outputFilename = resOutputFile;
    resolverArgs.sharedArray = sharedArray;
    resolverArgs.sharedArrayHead = sharedArrayHead;
    resolverArgs.sharedArrayTail = sharedArrayTail;
    resolverArgs.arrayMutex = arrayMutex;
    resolverArgs.resolverFileMutex = resolverFileMutex;
    resolverArgs.sem = arraySem;

    // Create the requester threads
    for (int i = 0; i < numRequesters; i++) {
        pthread_create(&requesterThreads[i], NULL, requesterFunction, (void*) &requesterArgs);
    }

    // Create the resolver threads
    for (int i = 0; i < numResolvers; i++) {
        pthread_create(&resolverThreads[i], NULL, resolverFunction, (void*) &resolverArgs);
    }

}