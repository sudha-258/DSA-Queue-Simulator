#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stdbool.h>

#define MAX_PQ_SIZE 10

// Road information for priority queue
typedef struct {
    int roadIndex;     // 0=A, 1=B, 2=C, 3=D
    int vehicleCount;  // Number of vehicles waiting
    int priority;      // Lower value = higher priority
    bool isPriorityLane;  // Is this the priority lane (AL2)?
} RoadData;

// Priority Queue structure (Min Heap)
typedef struct {
    RoadData data[MAX_PQ_SIZE];
    int size;
} PriorityQueue;

// Function prototypes
void initPriorityQueue(PriorityQueue* pq);
bool isPQEmpty(PriorityQueue* pq);
bool isPQFull(PriorityQueue* pq);
void pqInsert(PriorityQueue* pq, RoadData rd);
RoadData pqExtractMin(PriorityQueue* pq);
RoadData pqPeek(PriorityQueue* pq);
void pqUpdatePriority(PriorityQueue* pq, int roadIndex, int newPriority);
void displayPriorityQueue(PriorityQueue* pq);

// Helper functions
void heapifyUp(PriorityQueue* pq, int index);
void heapifyDown(PriorityQueue* pq, int index);
void swap(RoadData* a, RoadData* b);

#endif // PRIORITY_QUEUE_H