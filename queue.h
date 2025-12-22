#ifndef QUEUE_H
#define QUEUE_H
#include<stdio.h>
#include<stdbool.h>

#define MAX_QUEUE 50

typedef struct {
    int id;
    char road;   // A/B/C/D
    int arrivalTime;
    int lane;          // vehicle id
} Vehicle;

typedef struct {
    Vehicle v;      // logical vehicle (queue)
    int x, y;
    bool active;
    bool waiting;
} VisualVehicle;


typedef struct {
    Vehicle data[MAX_QUEUE];
    int front;
    int rear;
    int count;
} Queue;

/* Queue operations */
void initQueue(Queue *q);
int isEmpty(Queue *q);
int isFull(Queue *q);
int enqueue(Queue *q, Vehicle v);
Vehicle dequeue(Queue *q);
int queueSize(Queue *q);

#endif
