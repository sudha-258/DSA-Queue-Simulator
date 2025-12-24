#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#define MAX_QUEUE 100

// Vehicle structure
typedef struct {
    int id;
    char road;      // 'A', 'B', 'C', 'D'
    int lane;       // 1, 2, 3
    int x, y;       // Position
    bool active;    // Still in simulation
    Uint32 arrivalTime;  // When vehicle entered queue
    char dir;
    bool hasTurned;
} Vehicle;

// Queue structure
typedef struct {
    Vehicle data[MAX_QUEUE];
    int front;
    int rear;
    int size;
} Queue;

// Function prototypes
void initQueue(Queue* q);
bool isEmpty(Queue* q);
bool isFull(Queue* q);
void enqueue(Queue* q, Vehicle v);
Vehicle dequeue(Queue* q);
Vehicle peek(Queue* q);
int queueSize(Queue* q);
void displayQueue(Queue* q);
void clearQueue(Queue* q);

#endif // QUEUE_H