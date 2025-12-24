#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include "queue.h"
#include "priorityQueue.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 1000
#define ROAD_WIDTH 180
#define LANE_WIDTH 60
#define VEHICLE_WIDTH 25
#define VEHICLE_HEIGHT 35
#define VEHICLE_SPEED 2
#define NUM_ROADS 4
#define TIME_PER_VEHICLE 3000
#define PRIORITY_THRESHOLD_HIGH 10
#define PRIORITY_THRESHOLD_LOW 5

// Road indices
#define ROAD_A 0
#define ROAD_B 1
#define ROAD_C 2
#define ROAD_D 3

// SDL objects
SDL_Window* window;
SDL_Renderer* renderer;
SDL_mutex* mutex;

// Traffic state
int currentGreen = -1;
Uint32 greenLightStartTime = 0;
Uint32 greenLightDuration = 0;
bool isPriorityMode = false;

// Queues for lanes
Queue AL1, AL2, AL3;
Queue BL1, BL2, BL3;
Queue CL1, CL2, CL3;
Queue DL1, DL2, DL3;

Queue* lanes[12];
Queue* controlLanes[4];

// Priority queue for roads
PriorityQueue roadQueue;

// Statistics
int vehiclesPassed[4] = {0};

void initAllQueues() {
    initQueue(&AL1); initQueue(&AL2); initQueue(&AL3);
    initQueue(&BL1); initQueue(&BL2); initQueue(&BL3);
    initQueue(&CL1); initQueue(&CL2); initQueue(&CL3);
    initQueue(&DL1); initQueue(&DL2); initQueue(&DL3);
}

void initLanesArray() {
    lanes[0] = &AL1; lanes[1] = &AL2; lanes[2] = &AL3;
    lanes[3] = &BL1; lanes[4] = &BL2; lanes[5] = &BL3;
    lanes[6] = &CL1; lanes[7] = &CL2; lanes[8] = &CL3;
    lanes[9] = &DL1; lanes[10]= &DL2; lanes[11]= &DL3;
    
    controlLanes[0] = &AL2;
    controlLanes[1] = &BL2;
    controlLanes[2] = &CL2;
    controlLanes[3] = &DL2;
}

Vehicle createVehicle(char road, int lane) {
    static int vehicleID = 1;
    Vehicle v;
    v.id = vehicleID++;
    v.road = road;
    v.lane = lane;
    v.active = true;
    v.arrivalTime = SDL_GetTicks();

    switch(road) {
        case 'A':
            v.x = WINDOW_WIDTH/2 - ROAD_WIDTH/2 + (lane-1)*LANE_WIDTH + (LANE_WIDTH-VEHICLE_WIDTH)/2;
            v.y = 0;
            break;
        case 'B':
            v.x = WINDOW_WIDTH/2 - ROAD_WIDTH/2 + (lane-1)*LANE_WIDTH + (LANE_WIDTH-VEHICLE_WIDTH)/2;
            v.y = WINDOW_HEIGHT - VEHICLE_HEIGHT;
            break;
        case 'C':
            v.x = WINDOW_WIDTH - VEHICLE_WIDTH;
            v.y = WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + (lane-1)*LANE_WIDTH + (LANE_WIDTH-VEHICLE_HEIGHT)/2;
            break;
        case 'D':
            v.x = 0;
            v.y = WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + (lane-1)*LANE_WIDTH + (LANE_WIDTH-VEHICLE_HEIGHT)/2;
            break;
    }
    return v;
}

void drawBackground() {
    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
    SDL_RenderClear(renderer);
}

void drawRoads() {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect vert = {WINDOW_WIDTH/2 - ROAD_WIDTH/2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_Rect hori = {0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &vert);
    SDL_RenderFillRect(renderer, &hori);

    // Lane dividers
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int i=1; i<3; i++){
        for(int x=0; x<WINDOW_WIDTH; x+=20){
            SDL_RenderDrawLine(renderer, x, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH,
                             x+10, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH);
        }
        for(int y=0; y<WINDOW_HEIGHT; y+=20){
            SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, y,
                             WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, y+10);
        }
    }
}

void drawLights() {
    int centerX = WINDOW_WIDTH/2;
    int centerY = WINDOW_HEIGHT/2;
    int lightSize = 20;
    int gap = 30;

    struct {int x, y;} lightPos[4] = {
        {centerX - lightSize/2, centerY - ROAD_WIDTH/2 - gap - lightSize},
        {centerX - lightSize/2, centerY + ROAD_WIDTH/2 + gap},
        {centerX + ROAD_WIDTH/2 + gap, centerY - lightSize/2},
        {centerX - ROAD_WIDTH/2 - gap - lightSize, centerY - lightSize/2}
    };

    for(int i=0; i<NUM_ROADS; i++){
        SDL_Rect light = {lightPos[i].x, lightPos[i].y, lightSize, lightSize};
        
        if(currentGreen == i) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
        SDL_RenderFillRect(renderer, &light);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &light);
    }
}

void drawQueue(Queue* q, int laneType) {
    int size = queueSize(q);
    for(int i=0; i<size; i++){
        Vehicle v = q->data[(q->front + i) % MAX_QUEUE];
        if(!v.active) continue;
        
        // Skip if outside screen bounds (optimization)
        if(v.x < -VEHICLE_WIDTH || v.x > WINDOW_WIDTH ||
           v.y < -VEHICLE_HEIGHT || v.y > WINDOW_HEIGHT) {
            continue;
        }
        
        SDL_Rect body = {v.x, v.y, VEHICLE_WIDTH, VEHICLE_HEIGHT};
        
        // Color based on lane type
        if(laneType == 2) {
            SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);
        } else if(laneType == 3) {
            SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
        }
        
        SDL_RenderFillRect(renderer, &body);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &body);
    }
}

bool shouldStop(Vehicle* v, int laneType) {
    if(laneType == 3) return false;  // Free lane
    if(laneType == 1) return false;  // Incoming lane
    
    // For control lanes (L2)
    int roadIdx = v->road - 'A';
    if(roadIdx == currentGreen) return false;
    
    int centerX = WINDOW_WIDTH/2;
    int centerY = WINDOW_HEIGHT/2;
    int stopDistance = ROAD_WIDTH/2 + 20;
    
    switch(v->road) {
        case 'A': return v->y >= (centerY - stopDistance);
        case 'B': return v->y <= (centerY + stopDistance - VEHICLE_HEIGHT);
        case 'C': return v->x <= (centerX + stopDistance - VEHICLE_WIDTH);
        case 'D': return v->x >= (centerX - stopDistance);
    }
    return false;
}

void moveVehicles() {
    SDL_LockMutex(mutex);
    
    int laneTypes[] = {1,2,3, 1,2,3, 1,2,3, 1,2,3};
    
    for(int i=0; i<12; i++){
        Queue *q = lanes[i];
        int size = queueSize(q);
        
        // Process vehicles in-place without dequeuing
        for(int j=0; j<size; j++){
            Vehicle* v = &q->data[(q->front+j) % MAX_QUEUE];
            if(!v->active) continue;
            
            // Check spacing with vehicle ahead
            bool blocked = false;
            if(j > 0) {
                Vehicle* ahead = &q->data[(q->front+j-1) % MAX_QUEUE];
                if(ahead->active) {
                    int distance = 0;
                    switch(v->road) {
                        case 'A': distance = ahead->y - v->y; break;
                        case 'B': distance = v->y - ahead->y; break;
                        case 'C': distance = v->x - ahead->x; break;
                        case 'D': distance = ahead->x - v->x; break;
                    }
                    if(distance < VEHICLE_HEIGHT + 10) blocked = true;
                }
            }
            
            if(blocked || shouldStop(v, laneTypes[i])) continue;
            
            // Move vehicle
            switch(v->road){
                case 'A': 
                    v->y += VEHICLE_SPEED; 
                    if(v->y > WINDOW_HEIGHT) v->active = false;
                    break;
                case 'B': 
                    v->y -= VEHICLE_SPEED; 
                    if(v->y < -VEHICLE_HEIGHT) v->active = false;
                    break;
                case 'C': 
                    v->x -= VEHICLE_SPEED; 
                    if(v->x < -VEHICLE_WIDTH) v->active = false;
                    break;
                case 'D': 
                    v->x += VEHICLE_SPEED; 
                    if(v->x > WINDOW_WIDTH) v->active = false;
                    break;
            }
        }
        
        // Clean up inactive vehicles only when they're at the front
        while(queueSize(q) > 0) {
            Vehicle* front = &q->data[q->front];
            if(!front->active) {
                if(laneTypes[i] == 2) {
                    vehiclesPassed[i/3]++;
                }
                dequeue(q);
            } else {
                break;
            }
        }
    }
    
    SDL_UnlockMutex(mutex);
}

int calculateVehiclesToServe() {
    int totalVehicles = 0;
    int normalLanes = 0;
    
    for(int i=0; i<4; i++){
        if(i == ROAD_A && isPriorityMode) continue;
        int count = queueSize(controlLanes[i]);
        if(count > 0) {
            totalVehicles += count;
            normalLanes++;
        }
    }
    
    if(normalLanes == 0) return 1;
    return (totalVehicles + normalLanes - 1) / normalLanes;
}

void updateTrafficLights() {
    Uint32 now = SDL_GetTicks();
    
    if(currentGreen >= 0 && (now - greenLightStartTime) >= greenLightDuration) {
        currentGreen = -1;
        printf("Light cycle ended\n");
    }
    
    if(currentGreen < 0) {
        SDL_LockMutex(mutex);
        
        int al2Count = queueSize(&AL2);
        
        if(al2Count >= PRIORITY_THRESHOLD_HIGH) {
            isPriorityMode = true;
            currentGreen = ROAD_A;
            int vehiclesToServe = al2Count;
            greenLightDuration = vehiclesToServe * TIME_PER_VEHICLE;
            printf("PRIORITY MODE: AL2 has %d vehicles, serving for %d ms\n", 
                   al2Count, greenLightDuration);
        } else if(isPriorityMode && al2Count < PRIORITY_THRESHOLD_LOW) {
            isPriorityMode = false;
            printf("Priority mode ended, AL2 count: %d\n", al2Count);
        }
        
        if(!isPriorityMode && currentGreen < 0) {
            static int lastServed = -1;
            int attempts = 0;
            
            do {
                lastServed = (lastServed + 1) % NUM_ROADS;
                attempts++;
                if(attempts > NUM_ROADS) {
                    lastServed = 0;
                    break;
                }
            } while(queueSize(controlLanes[lastServed]) == 0);
            
            if(queueSize(controlLanes[lastServed]) > 0) {
                currentGreen = lastServed;
                int vehiclesToServe = calculateVehiclesToServe();
                greenLightDuration = vehiclesToServe * TIME_PER_VEHICLE;
                printf("Normal mode: Serving Road %c, vehicles=%d, duration=%d ms\n",
                       'A'+currentGreen, vehiclesToServe, greenLightDuration);
            }
        }
        
        greenLightStartTime = now;
        SDL_UnlockMutex(mutex);
    }
}

void drawStats() {
    char stats[256];
    SDL_Color white = {255, 255, 255, 255};
    
    sprintf(stats, "AL2:%d BL2:%d CL2:%d DL2:%d | Mode:%s | Green:%c",
            queueSize(&AL2), queueSize(&BL2), queueSize(&CL2), queueSize(&DL2),
            isPriorityMode ? "PRIORITY" : "NORMAL",
            currentGreen >= 0 ? 'A'+currentGreen : '-');
    
    printf("\r%s", stats);
    fflush(stdout);
}

void readVehiclesFromFiles() {
    static int currentFileIndex = 0;
    char* files[] = {"lanea.txt", "laneb.txt", "lanec.txt", "laned.txt"};
    char roads[] = "ABCD";
    
    int i = currentFileIndex;
    currentFileIndex = (currentFileIndex + 1) % 4;
    
    FILE* f = fopen(files[i], "r");
    if(!f) return;
    
    char line[128];
    int vehiclesAdded = 0;
    while(fgets(line, sizeof(line), f) && vehiclesAdded < 5){
        int lane;
        if(sscanf(line, "%d", &lane) == 1 && lane >= 1 && lane <= 3){
            Vehicle v = createVehicle(roads[i], lane);
            
            SDL_LockMutex(mutex);
            Queue* targetQueue = lanes[i*3 + (lane-1)];
            enqueue(targetQueue, v);
            SDL_UnlockMutex(mutex);
            vehiclesAdded++;
        }
    }
    fclose(f);
    
    // Clear file after reading
    f = fopen(files[i], "w");
    if(f) fclose(f);
}

int main(int argc, char *argv[])
{
    initAllQueues();
    initLanesArray();
    srand((unsigned int)time(NULL));
    mutex = SDL_CreateMutex();

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Traffic Junction Simulator - DSA Assignment",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if(!window) {
        printf("Window Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) {
        printf("Renderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    Uint32 lastFileRead = SDL_GetTicks();

    printf("Traffic Simulator Started\n");
    printf("=========================\n");
    printf("AL2 is PRIORITY LANE\n");
    printf("Priority activates at %d vehicles\n", PRIORITY_THRESHOLD_HIGH);
    printf("Priority ends at %d vehicles\n\n", PRIORITY_THRESHOLD_LOW);

    while(running) {
        Uint32 frameStart = SDL_GetTicks();
        
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
        }

        // Read vehicle data from files less frequently
        Uint32 now = SDL_GetTicks();
        if(now - lastFileRead > 1000) {
            readVehiclesFromFiles();
            lastFileRead = now;
        }

        updateTrafficLights();
        moveVehicles();

        // Render
        drawBackground();
        drawRoads();
        drawLights();

        int laneTypes[] = {1,2,3, 1,2,3, 1,2,3, 1,2,3};
        for(int i=0; i<12; i++){
            drawQueue(lanes[i], laneTypes[i]);
        }
        
        drawStats();

        SDL_RenderPresent(renderer);
        
        // Frame rate control
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if(frameTime < 16) {
            SDL_Delay(16 - frameTime);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyMutex(mutex);
    SDL_Quit();
    
    printf("\n\nSimulation Statistics:\n");
    printf("Road A passed: %d\n", vehiclesPassed[0]);
    printf("Road B passed: %d\n", vehiclesPassed[1]);
    printf("Road C passed: %d\n", vehiclesPassed[2]);
    printf("Road D passed: %d\n", vehiclesPassed[3]);
    
    return 0;
}