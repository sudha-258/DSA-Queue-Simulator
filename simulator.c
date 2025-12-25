#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include <math.h>
#include "queue.h"
#include "priorityQueue.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 1000
#define ROAD_WIDTH 180
#define LANE_WIDTH 60
#define VEHICLE_WIDTH 25
#define VEHICLE_HEIGHT 35
#define VEHICLE_SPEED 3.0
#define NUM_ROADS 4
#define TIME_PER_VEHICLE 1000
#define PRIORITY_THRESHOLD_HIGH 10
#define PRIORITY_THRESHOLD_LOW 5
#define MIN_GREEN_DURATION 2000
#define SAFE_DISTANCE 50

// Road indices
#define ROAD_A 0
#define ROAD_B 1
#define ROAD_C 2
#define ROAD_D 3

// SDL objects
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_mutex* mutex = NULL;

// Traffic state
int currentGreen = -1;
Uint32 greenLightStartTime = 0;
Uint32 greenLightDuration = 0;
bool isPriorityMode = false;
int lastServedRoad = -1;

// Queues for lanes
Queue AL1, AL2, AL3;
Queue BL1, BL2, BL3;
Queue CL1, CL2, CL3;
Queue DL1, DL2, DL3;

Queue* lanes[12];
Queue* controlLanes[4];

// Statistics
int vehiclesPassed[4] = {0};
int totalVehiclesGenerated = 0;
int priorityActivations = 0;

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

bool isInJunction(Vehicle* v) {
    int centerX = WINDOW_WIDTH/2;
    int centerY = WINDOW_HEIGHT/2;
    int junctionSize = ROAD_WIDTH/2 + 10;
    
    return (v->x >= (centerX - junctionSize+23) && v->x <= (centerX + junctionSize -37) &&
            v->y >= (centerY - junctionSize+20) && v->y <= (centerY + junctionSize-45));
}

bool checkCollision(Queue* q, float x, float y) {
    if(isEmpty(q)) return false;
    
    int size = queueSize(q);
    for(int i=0; i<size; i++){
        Vehicle* existing = &q->data[(q->front + i) % MAX_QUEUE];
        if(!existing->active) continue;
        
        float dx = x - existing->x;
        float dy = y - existing->y;
        float distance = sqrtf(dx*dx + dy*dy);
        
        if(distance < SAFE_DISTANCE) {
            return true;
        }
    }
    return false;
}

Vehicle createVehicle(char road, int lane) {
    static int vehicleID = 1;
    Vehicle v;
    v.id = vehicleID++;
    v.road = road;
    v.lane = lane;
    v.active = true;
    v.arrivalTime = SDL_GetTicks();
    v.hasTurned = false;

    int baseX = WINDOW_WIDTH/2 - ROAD_WIDTH/2;
    int baseY = WINDOW_HEIGHT/2 - ROAD_WIDTH/2;
    int laneOffset = (lane-1) * LANE_WIDTH;
    int reversedLaneOffset = (3 - lane) * LANE_WIDTH;
    
    switch(road) {
        case 'A': // From North (top)
            v.x = baseX + laneOffset + (LANE_WIDTH - VEHICLE_WIDTH) / 2;
            v.y = -VEHICLE_HEIGHT - (rand() % 100);
            v.dir = 'D';  // Moving DOWN
            break;
            
        case 'B': // From South (bottom) 
            v.x = baseX + reversedLaneOffset + (LANE_WIDTH - VEHICLE_WIDTH) / 2;
            v.y = WINDOW_HEIGHT + (rand() % 100);
            v.dir = 'U';  // Moving UP
            break;
            
        case 'C': // From East (right)
            v.x = WINDOW_WIDTH + (rand() % 100);
            v.y = baseY + laneOffset + (LANE_WIDTH - VEHICLE_HEIGHT) / 2;
            v.dir = 'L';  // Moving LEFT (FIX: was 'R')
            break;
            
        case 'D': // From West (left)
            v.x = -VEHICLE_WIDTH - (rand() % 100);
            v.y = baseY + reversedLaneOffset + (LANE_WIDTH - VEHICLE_HEIGHT) / 2;
            v.dir = 'R';  // Moving RIGHT (FIX: was 'L')
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

    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_Rect junction = {
        WINDOW_WIDTH/2 - ROAD_WIDTH/2,
        WINDOW_HEIGHT/2 - ROAD_WIDTH/2,
        ROAD_WIDTH, ROAD_WIDTH
    };
    SDL_RenderFillRect(renderer, &junction);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int i=1; i<3; i++){
        for(int x=0; x<WINDOW_WIDTH; x+=30){
            int y = WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH;
            SDL_RenderDrawLine(renderer, x, y, x+15, y);
        }
        for(int y=0; y<WINDOW_HEIGHT; y+=30){
            int x = WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH;
            SDL_RenderDrawLine(renderer, x, y, x, y+15);
        }
    }
}

void drawLights() {
    int centerX = WINDOW_WIDTH/2;
    int centerY = WINDOW_HEIGHT/2;
    int lightSize = 25;
    int gap = 35;

    struct {int x, y;} lightPos[4] = {
        {centerX - lightSize/2, centerY - ROAD_WIDTH/2 - gap - lightSize},
        {centerX - lightSize/2, centerY + ROAD_WIDTH/2 + gap},
        {centerX + ROAD_WIDTH/2 + gap, centerY - lightSize/2},
        {centerX - ROAD_WIDTH/2 - gap - lightSize, centerY - lightSize/2}
    };

    for(int i=0; i<NUM_ROADS; i++){
        SDL_Rect light = {lightPos[i].x, lightPos[i].y, lightSize, lightSize};
        
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &light);
        
        if(currentGreen == i) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
        
        SDL_Rect innerLight = {
            lightPos[i].x + 3, 
            lightPos[i].y + 3, 
            lightSize - 6, 
            lightSize - 6
        };
        SDL_RenderFillRect(renderer, &innerLight);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &light);
    }
}

void drawQueue(Queue* q, int laneType) {
    if(isEmpty(q)) return;
    
    for(int i=0; i<queueSize(q); i++){
        Vehicle v = q->data[(q->front + i) % MAX_QUEUE];
        if(!v.active) continue;
        
        SDL_Rect body = {(int)v.x, (int)v.y, VEHICLE_WIDTH, VEHICLE_HEIGHT};
        
        if(laneType == 2) {
            SDL_SetRenderDrawColor(renderer, 30, 144, 255, 255);
        } else if(laneType == 3) {
            SDL_SetRenderDrawColor(renderer, 50, 205, 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
        }
        
        SDL_RenderFillRect(renderer, &body);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &body);
        
        if(v.road == 'A' && v.lane == 2) {
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
            SDL_Rect highlight = {(int)v.x-2, (int)v.y-2, VEHICLE_WIDTH+4, VEHICLE_HEIGHT+4};
            SDL_RenderDrawRect(renderer, &highlight);
        }
    }
}

bool shouldStop(Vehicle* v, int laneType) {
    if(laneType == 1 || laneType == 3) return false;
    
    int roadIdx = v->road - 'A';
    if(roadIdx == currentGreen) return false;
    
    int centerX = WINDOW_WIDTH/2;
    int centerY = WINDOW_HEIGHT/2;
    int stopDistance = ROAD_WIDTH/2 + 10;
    
    switch(v->road) {
        case 'A':
            return v->y >= (centerY - stopDistance) && v->y <= centerY;
        case 'B':
            return v->y <= (centerY + stopDistance) && v->y >= centerY;
        case 'C':
            return v->x <= (centerX + stopDistance) && v->x >= centerX;
        case 'D':
            return v->x >= (centerX - stopDistance) && v->x <= centerX;
    }
    return false;
}

void moveVehicles() {
    if(!mutex) return;
    SDL_LockMutex(mutex);
    
    int laneTypes[] = {1,2,3, 1,2,3, 1,2,3, 1,2,3};
    
    for(int i=0; i<12; i++){
        Queue *q = lanes[i];
        if(isEmpty(q)) continue;
        
        int size = queueSize(q);
        int laneType = laneTypes[i];
        
        for(int j=0; j<size; j++){
            int idx = (q->front + j) % MAX_QUEUE;
            Vehicle* v = &q->data[idx];
            
            if(!v->active) continue;
            if(shouldStop(v, laneType)) continue;
            
            bool inJunction = isInJunction(v);
            
           
            if(v->road == 'A') {
                if (!inJunction && !v->hasTurned) {
                    v->y += VEHICLE_SPEED;
                }
                else if (inJunction && !v->hasTurned) {
                    if (laneType == 1) {
                        v->dir = 'L';  
                    }
                    else if (laneType == 2) {
                        v->dir = 'D'; 
                    }
                    else if (laneType == 3) {
                        v->dir = 'R'; 
                    }
                    v->hasTurned = true;
                }
                else {
                    switch (v->dir) {
                        case 'D': v->y += VEHICLE_SPEED; break;  // Down
                        case 'U': v->y -= VEHICLE_SPEED; break;  // Up
                        case 'L': v->x -= VEHICLE_SPEED; break;  // Left (West)
                        case 'R': v->x += VEHICLE_SPEED; break;  // Right (East)
                    }
                }
                if(v->y > WINDOW_HEIGHT + VEHICLE_HEIGHT || 
                   v->x < -VEHICLE_WIDTH*2 || v->x > WINDOW_WIDTH + VEHICLE_WIDTH*2) {
                    v->active = false;
                }
            }
            
            // ═══════════════════════════════════════════════════════════
            // Road B: Coming from SOUTH (moving UP ↑)
            // ═══════════════════════════════════════════════════════════
            else if(v->road == 'B') {
                if (!inJunction && !v->hasTurned) {
                    v->y -= VEHICLE_SPEED;
                }
                else if (inJunction && !v->hasTurned) {
                    if (laneType == 1) {
                        v->dir = 'R';  // Right turn → EAST
                    }
                    else if (laneType == 2) {
                        v->dir = 'U';  // Straight → UP
                    }
                    else if (laneType == 3) {
                        v->dir = 'L';  // Left turn → WEST
                    }
                    v->hasTurned = true;
                }
                else {
                    switch (v->dir) {
                        case 'D': v->y += VEHICLE_SPEED; break;  // Down
                        case 'U': v->y -= VEHICLE_SPEED; break;  // Up
                        case 'L': v->x -= VEHICLE_SPEED; break;  // Left (West)
                        case 'R': v->x += VEHICLE_SPEED; break;  // Right (East)
                    }
                }
                if(v->y < -VEHICLE_HEIGHT*2 || 
                   v->x < -VEHICLE_WIDTH*2 || v->x > WINDOW_WIDTH + VEHICLE_WIDTH*2) {
                    v->active = false;
                }
            }
            
            // ═══════════════════════════════════════════════════════════
            // Road C: Coming from EAST (moving LEFT ←)
            // ═══════════════════════════════════════════════════════════
            else if(v->road == 'C') {
                if (!inJunction && !v->hasTurned) {
                    v->x -= VEHICLE_SPEED;
                }
                else if (inJunction && !v->hasTurned) {
                    if (laneType == 1) {
                        v->dir = 'U';  // Right turn → NORTH
                    }
                    else if (laneType == 2) {
                        v->dir = 'L';  // Straight → LEFT
                    }
                    else if (laneType == 3) {
                        v->dir = 'D';  // Left turn → SOUTH
                    }
                    v->hasTurned = true;
                }
                else {
                    switch (v->dir) {
                        case 'D': v->y += VEHICLE_SPEED; break;  // Down
                        case 'U': v->y -= VEHICLE_SPEED; break;  // Up
                        case 'L': v->x -= VEHICLE_SPEED; break;  // Left (West)
                        case 'R': v->x += VEHICLE_SPEED; break;  // Right (East)
                    }
                }
                if(v->x < -VEHICLE_WIDTH*2 || 
                   v->y < -VEHICLE_HEIGHT*2 || v->y > WINDOW_HEIGHT + VEHICLE_HEIGHT*2) {
                    v->active = false;
                }
            }
            
            // ═══════════════════════════════════════════════════════════
            // Road D: Coming from WEST (moving RIGHT →)
            // ═══════════════════════════════════════════════════════════
            else if(v->road == 'D') {
                if (!inJunction && !v->hasTurned) {
                    v->x += VEHICLE_SPEED;
                }
                else if (inJunction && !v->hasTurned) {
                    if (laneType == 1) {
                        v->dir = 'D';  // Right turn → SOUTH
                    }
                    else if (laneType == 2) {
                        v->dir = 'R';  // Straight → RIGHT
                    }
                    else if (laneType == 3) {
                        v->dir = 'U';  // Left turn → NORTH
                    }
                    v->hasTurned = true;
                }
                else {
                    switch (v->dir) {
                        case 'D': v->y += VEHICLE_SPEED; break;  // Down
                        case 'U': v->y -= VEHICLE_SPEED; break;  // Up
                        case 'L': v->x -= VEHICLE_SPEED; break;  // Left (West)
                        case 'R': v->x += VEHICLE_SPEED; break;  // Right (East)
                    }
                }
                if(v->x > WINDOW_WIDTH + VEHICLE_WIDTH*2 || 
                   v->y < -VEHICLE_HEIGHT*2 || v->y > WINDOW_HEIGHT + VEHICLE_HEIGHT*2) {
                    v->active = false;
                }
            }
        }
        
        int count = queueSize(q);
        for(int k=0; k<count; k++){
            Vehicle frontV = dequeue(q);
            if(frontV.active) {
                enqueue(q, frontV);
            } else if(laneType == 2) {
                vehiclesPassed[i/3]++;
            }
        }
    }
    
    SDL_UnlockMutex(mutex);
}

int calculateVehiclesToServe() {
    int totalVehicles = 0;
    int activeLanes = 0;
    
    for(int i=0; i<4; i++){
        if(i == ROAD_A && isPriorityMode) continue;
        
        int count = queueSize(controlLanes[i]);
        if(count > 0) {
            totalVehicles += count;
            activeLanes++;
        }
    }
    
    if(activeLanes == 0) return 1;
    
    int avgVehicles = (totalVehicles + activeLanes - 1) / activeLanes;
    return (avgVehicles > 0) ? avgVehicles : 1;
}

void updateTrafficLights() {
    Uint32 now = SDL_GetTicks();
    
    if(currentGreen >= 0) {
        if((now - greenLightStartTime) >= greenLightDuration) {
            printf("Light cycle ended for Road %c\n", 'A' + currentGreen);
            currentGreen = -1;
        } else {
            return;
        }
    }
    
    if(!mutex) return;
    SDL_LockMutex(mutex);
    
    int al2Count = queueSize(&AL2);
    
    if(al2Count >= PRIORITY_THRESHOLD_HIGH && !isPriorityMode) {
        isPriorityMode = true;
        priorityActivations++;
        currentGreen = ROAD_A;
        greenLightDuration = al2Count * TIME_PER_VEHICLE;
        if(greenLightDuration < MIN_GREEN_DURATION) {
            greenLightDuration = MIN_GREEN_DURATION;
        }
        greenLightStartTime = now;
        printf(">>> PRIORITY MODE ACTIVATED: AL2=%d vehicles, duration=%u ms\n", 
               al2Count, greenLightDuration);
        SDL_UnlockMutex(mutex);
        return;
    }
    
    if(isPriorityMode && al2Count < PRIORITY_THRESHOLD_LOW) {
        isPriorityMode = false;
        printf(">>> Priority mode ENDED: AL2=%d vehicles\n", al2Count);
    }
    
    if(!isPriorityMode) {
        int attempts = 0;
        int nextRoad = lastServedRoad;
        
        do {
            nextRoad = (nextRoad + 1) % NUM_ROADS;
            attempts++;
            if(attempts > NUM_ROADS) {
                nextRoad = 0;
                break;
            }
        } while(queueSize(controlLanes[nextRoad]) == 0);
        
        if(queueSize(controlLanes[nextRoad]) > 0) {
            currentGreen = nextRoad;
            lastServedRoad = nextRoad;
            
            int vehiclesToServe = calculateVehiclesToServe();
            greenLightDuration = vehiclesToServe * TIME_PER_VEHICLE;
            
            if(greenLightDuration < MIN_GREEN_DURATION) {
                greenLightDuration = MIN_GREEN_DURATION;
            }
            
            greenLightStartTime = now;
            
            printf("Normal mode: Road %c | Vehicles=%d | Duration=%u ms | ",
                   'A'+currentGreen, vehiclesToServe, greenLightDuration);
            printf("Queue sizes: AL2=%d BL2=%d CL2=%d DL2=%d\n",
                   queueSize(&AL2), queueSize(&BL2), 
                   queueSize(&CL2), queueSize(&DL2));
        }
    }
    
    SDL_UnlockMutex(mutex);
}

void drawStats() {
    char stats[512];
    snprintf(stats, sizeof(stats), 
            "AL2:%d BL2:%d CL2:%d DL2:%d | Mode:%s | Green:%c | Total:%d | Passed:%d | Priority:%d",
            queueSize(&AL2), queueSize(&BL2), queueSize(&CL2), queueSize(&DL2),
            isPriorityMode ? "PRIORITY" : "NORMAL ",
            currentGreen >= 0 ? 'A'+currentGreen : '-',
            totalVehiclesGenerated,
            vehiclesPassed[0]+vehiclesPassed[1]+vehiclesPassed[2]+vehiclesPassed[3],
            priorityActivations);
    
    printf("\r%s", stats);
    fflush(stdout);
}

void readVehiclesFromFiles() {
    const char* files[] = {"lanea.txt", "laneb.txt", "lanec.txt", "laned.txt"};
    const char roads[] = "ABCD";
    
    for(int i=0; i<4; i++){
        FILE* f = fopen(files[i], "r");
        if(!f) continue;
        
        char line[128];
        
        while(fgets(line, sizeof(line), f)){
            int lane;
            if(sscanf(line, "%d", &lane) == 1 && lane >= 1 && lane <= 3){
                Vehicle v = createVehicle(roads[i], lane);
                
                if(!mutex) continue;
                SDL_LockMutex(mutex);
                Queue* targetQueue = lanes[i*3 + (lane-1)];
                
                if(!isFull(targetQueue) && !checkCollision(targetQueue, v.x, v.y)) {
                    enqueue(targetQueue, v);
                    totalVehiclesGenerated++;
                }
                
                SDL_UnlockMutex(mutex);
            }
        }
        fclose(f);
        
        f = fopen(files[i], "w");
        if(f) fclose(f);
    }
}

void cleanup() {
    if(renderer) SDL_DestroyRenderer(renderer);
    if(window) SDL_DestroyWindow(window);
    if(mutex) SDL_DestroyMutex(mutex);
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    initAllQueues();
    initLanesArray();
    srand((unsigned int)time(NULL));
    
    mutex = SDL_CreateMutex();
    if(!mutex) {
        printf("Failed to create mutex: %s\n", SDL_GetError());
        return 1;
    }

    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Traffic Junction Simulator - DSA Queue Assignment",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if(!window) {
        printf("Window Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!renderer) {
        printf("Renderer Error: %s\n", SDL_GetError());
        cleanup();
        return 1;
    }

    bool running = true;
    Uint32 lastFileRead = SDL_GetTicks();

    printf("\n========================================\n");
    printf("   Traffic Junction Simulator Started\n");
    printf("========================================\n");
    printf("Lane Behavior:\n");
    printf("  L1 (Incoming): Turn RIGHT - Always move\n");
    printf("  L2 (Control):  Go STRAIGHT - Obey lights\n");
    printf("  L3 (Free):     Turn LEFT - Always move\n");
    printf("\n");
    printf("AL2 is PRIORITY LANE\n");
    printf("Priority activates: >=%d vehicles\n", PRIORITY_THRESHOLD_HIGH);
    printf("Priority ends: <%d vehicles\n", PRIORITY_THRESHOLD_LOW);
    printf("========================================\n\n");

    while(running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
            if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        Uint32 now = SDL_GetTicks();
        if(now - lastFileRead >= 500) {
            readVehiclesFromFiles();
            lastFileRead = now;
        }

        updateTrafficLights();
        moveVehicles();

        drawBackground();
        drawRoads();
        drawLights();

        int laneTypes[] = {1,2,3, 1,2,3, 1,2,3, 1,2,3};
        for(int i=0; i<12; i++){
            drawQueue(lanes[i], laneTypes[i]);
        }
        
        drawStats();

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    printf("\n\n========================================\n");
    printf("      Simulation Statistics\n");
    printf("========================================\n");
    printf("Total vehicles generated: %d\n", totalVehiclesGenerated);
    printf("Vehicles passed:\n");
    printf("  Road A (AL2): %d\n", vehiclesPassed[0]);
    printf("  Road B (BL2): %d\n", vehiclesPassed[1]);
    printf("  Road C (CL2): %d\n", vehiclesPassed[2]);
    printf("  Road D (DL2): %d\n", vehiclesPassed[3]);
    printf("  Total: %d\n", 
           vehiclesPassed[0]+vehiclesPassed[1]+vehiclesPassed[2]+vehiclesPassed[3]);
    printf("Priority activations: %d\n", priorityActivations);
    printf("========================================\n");

    cleanup();
    return 0;
}