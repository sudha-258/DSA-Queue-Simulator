#include "queue.h"
#include <stdio.h>
#include <string.h>

// Initialize queue
void initQueue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
    memset(q->data, 0, sizeof(q->data));
}

// Check if queue is empty
bool isEmpty(Queue* q) {
    return q->size == 0;
}

// Check if queue is full
bool isFull(Queue* q) {
    return q->size >= MAX_QUEUE;
}

// Add vehicle to queue
void enqueue(Queue* q, Vehicle v) {
    if (isFull(q)) {
        printf("Queue is full! Cannot enqueue vehicle %d\n", v.id);
        return;
    }
    
    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->data[q->rear] = v;
    q->size++;
}

// Remove vehicle from queue
Vehicle dequeue(Queue* q) {
    Vehicle empty = {0};
    
    if (isEmpty(q)) {
        printf("Queue is empty! Cannot dequeue\n");
        return empty;
    }
    
    Vehicle v = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->size--;
    
    return v;
}

// Peek at front vehicle without removing
Vehicle peek(Queue* q) {
    Vehicle empty = {0};
    
    if (isEmpty(q)) {
        return empty;
    }
    
    return q->data[q->front];
}

// Get current queue size
int queueSize(Queue* q) {
    return q->size;
}

// Display queue contents (for debugging)
void displayQueue(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return;
    }
    
    printf("Queue size: %d\n", q->size);
    printf("Vehicles: ");
    
    for (int i = 0; i < q->size; i++) {
        int index = (q->front + i) % MAX_QUEUE;
        Vehicle v = q->data[index];
        printf("[ID:%d Road:%c Lane:%d] ", v.id, v.road, v.lane);
    }
    printf("\n");
}

// Clear all vehicles from queue
void clearQueue(Queue* q) {
    initQueue(q);
}