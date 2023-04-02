#include <queue.h>

queue_t* queue_create() {
    queue_t* queue = malloc(sizeof(queue_t));
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

void queue_push(queue_t* queue, void* data) {
    queue_node_t* node = malloc(sizeof(queue_node_t));
    node->data = data;
    node->next = NULL;
    queue->tail->next = node;
    queue->tail = node;
}

char* queue_pop(queue_t* queue) {
    queue_node_t* node = queue->head;
    queue->head = node->next;
    char* data = node->data;
    free(node);
    return data;
}

void queue_destroy(queue_t* queue) {
    queue_node_t* node = queue->head;
    while (node != NULL) {
        queue_node_t* next = node->next;
        free(node);
        node = next;
    }
    free(queue);
}

int queue_isEmpty(queue_t* queue) {
    return queue->head == NULL;
}