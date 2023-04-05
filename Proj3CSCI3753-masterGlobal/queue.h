#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>

// Queue node
typedef struct node {
    char* data;
    struct node* next;
} node_t;

// Queue
typedef struct queue {
    node_t* head;
    node_t* tail;
    int size;
} queue_t;

// Create a new queue
queue_t* queue_create();

// Enqueue a string onto the queue
void queue_push(queue_t* queue, char* data);

// Dequeue a string from the queue
char* queue_pop(queue_t* queue);

// Free the queue
void queue_destroy(queue_t* queue);

// Check if the queue is empty
int queue_isEmpty(queue_t* queue);

// Get the size of the queue
int queue_size(queue_t* queue);

// Print the queue
void queue_print(queue_t* queue);

#endif
