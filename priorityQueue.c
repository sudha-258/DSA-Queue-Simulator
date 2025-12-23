#include "priorityQueue.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

// Initialize priority queue
void initPriorityQueue(PriorityQueue* pq) {
    pq->size = 0;
    memset(pq->data, 0, sizeof(pq->data));
}

// Check if priority queue is empty
bool isPQEmpty(PriorityQueue* pq) {
    return pq->size == 0;
}

// Check if priority queue is full
bool isPQFull(PriorityQueue* pq) {
    return pq->size >= MAX_PQ_SIZE;
}

// Swap two road data elements
void swap(RoadData* a, RoadData* b) {
    RoadData temp = *a;
    *a = *b;
    *b = temp;
}

// Heapify up (for insertion)
void heapifyUp(PriorityQueue* pq, int index) {
    if (index == 0) return;
    
    int parent = (index - 1) / 2;
    
    // Min heap: parent should have lower priority value
    if (pq->data[index].priority < pq->data[parent].priority) {
        swap(&pq->data[index], &pq->data[parent]);
        heapifyUp(pq, parent);
    }
}

// Heapify down (for extraction)
void heapifyDown(PriorityQueue* pq, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;
    
    if (left < pq->size && 
        pq->data[left].priority < pq->data[smallest].priority) {
        smallest = left;
    }
    
    if (right < pq->size && 
        pq->data[right].priority < pq->data[smallest].priority) {
        smallest = right;
    }
    
    if (smallest != index) {
        swap(&pq->data[index], &pq->data[smallest]);
        heapifyDown(pq, smallest);
    }
}

// Insert road data into priority queue
void pqInsert(PriorityQueue* pq, RoadData rd) {
    if (isPQFull(pq)) {
        printf("Priority Queue is full!\n");
        return;
    }
    
    pq->data[pq->size] = rd;
    heapifyUp(pq, pq->size);
    pq->size++;
}

// Extract minimum priority road (highest priority road)
RoadData pqExtractMin(PriorityQueue* pq) {
    RoadData empty = {-1, 0, INT_MAX, false};
    
    if (isPQEmpty(pq)) {
        printf("Priority Queue is empty!\n");
        return empty;
    }
    
    RoadData min = pq->data[0];
    pq->data[0] = pq->data[pq->size - 1];
    pq->size--;
    
    if (pq->size > 0) {
        heapifyDown(pq, 0);
    }
    
    return min;
}

// Peek at highest priority road without removing
RoadData pqPeek(PriorityQueue* pq) {
    RoadData empty = {-1, 0, INT_MAX, false};
    
    if (isPQEmpty(pq)) {
        return empty;
    }
    
    return pq->data[0];
}

// Update priority of a specific road
void pqUpdatePriority(PriorityQueue* pq, int roadIndex, int newPriority) {
    // Find the road in the heap
    int index = -1;
    for (int i = 0; i < pq->size; i++) {
        if (pq->data[i].roadIndex == roadIndex) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        printf("Road %d not found in priority queue\n", roadIndex);
        return;
    }
    
    int oldPriority = pq->data[index].priority;
    pq->data[index].priority = newPriority;
    
    // Restore heap property
    if (newPriority < oldPriority) {
        heapifyUp(pq, index);
    } else {
        heapifyDown(pq, index);
    }
}

// Display priority queue (for debugging)
void displayPriorityQueue(PriorityQueue* pq) {
    if (isPQEmpty(pq)) {
        printf("Priority Queue is empty\n");
        return;
    }
    
    printf("Priority Queue (size=%d):\n", pq->size);
    for (int i = 0; i < pq->size; i++) {
        RoadData rd = pq->data[i];
        char roadName = 'A' + rd.roadIndex;
        printf("  [Road %c: vehicles=%d, priority=%d%s]\n",
               roadName, rd.vehicleCount, rd.priority,
               rd.isPriorityLane ? " (PRIORITY LANE)" : "");
    }
}