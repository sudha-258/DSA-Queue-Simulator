#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include "queue.h"
#include "priorityQueue.h"
#include "junction.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define VEHICLE_WIDTH 20
#define VEHICLE_HEIGHT 30
#define LANES_PER_ROAD 3
#define VEHICLE_SPEED 2
#define NUM_ROADS 4
#define LIGHT_DURATION 5000
#define STOP_LINE_OFFSET 80  // Distance from center where vehicles stop

// SDL objects
SDL_Window* window;
SDL_Renderer* renderer;
SDL_mutex* mutex;

// Traffic lights
int currentGreen = 0;
Uint32 lastSwitchTime = 0;

// Queues for lanes
Queue AL1, AL2, AL3;
Queue BL1, BL2, BL3;
Queue CL1, CL2, CL3;
Queue DL1, DL2, DL3;

Queue* lanes[12];

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
    // Draw gray roads
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_Rect vert = {WINDOW_WIDTH/2 - ROAD_WIDTH/2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_Rect hori = {0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &vert);
    SDL_RenderFillRect(renderer, &hori);

    // Draw lane dividers (white dashed lines)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int i=1; i<3; i++){
        // Horizontal lane dividers
        for(int x=0; x<WINDOW_WIDTH; x+=20){
            SDL_RenderDrawLine(renderer, x, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH,
                             x+10, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH);
        }
        // Vertical lane dividers
        for(int y=0; y<WINDOW_HEIGHT; y+=20){
            SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, y,
                             WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, y+10);
        }
    }
}

void drawLights() {
    int centerX = WINDOW_WIDTH/2;
    int centerY = WINDOW_HEIGHT/2;
    int lightSize = 15;
    int gap = 25;

    // Light positions for each road
    struct {int x, y;} lightPos[4] = {
        {centerX - lightSize/2, centerY - ROAD_WIDTH/2 - gap - lightSize},  // Road A (top)
        {centerX - lightSize/2, centerY + ROAD_WIDTH/2 + gap},               // Road B (bottom)
        {centerX + ROAD_WIDTH/2 + gap, centerY - lightSize/2},               // Road C (right)
        {centerX - ROAD_WIDTH/2 - gap - lightSize, centerY - lightSize/2}   // Road D (left)
    };

    for(int i=0; i<NUM_ROADS; i++){
        SDL_Rect light = {lightPos[i].x, lightPos[i].y, lightSize, lightSize};
        
        // Draw light based on state
        if(currentGreen == i) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red
        }
        SDL_RenderFillRect(renderer, &light);
        
        // Draw border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &light);
    }
}

void drawQueue(Queue* q) {
    for(int i=0; i<queueSize(q); i++){
        Vehicle v = q->data[(q->front + i) % MAX_QUEUE];
        if(!v.active) continue;
        
        SDL_Rect body = {v.x, v.y, VEHICLE_WIDTH, VEHICLE_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255);
        SDL_RenderFillRect(renderer, &body);
        
        // Draw vehicle border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &body);
    }
}

bool shouldStop(Vehicle* v) {
    int roadIdx = v->road - 'A';
    if(roadIdx == currentGreen) return false;  // Green light, don't stop
    
    int centerX = WINDOW_WIDTH/2;
    int centerY = WINDOW_HEIGHT/2;
    
    switch(v->road) {
        case 'A': return v->y >= (centerY - STOP_LINE_OFFSET);
        case 'B': return v->y <= (centerY + STOP_LINE_OFFSET);
        case 'C': return v->x <= (centerX + STOP_LINE_OFFSET);
        case 'D': return v->x >= (centerX - STOP_LINE_OFFSET);
    }
    return false;
}

void moveVehicles() {
    SDL_LockMutex(mutex);
    
    for(int i=0; i<12; i++){
        Queue *q = lanes[i];
        int size = queueSize(q);
        
        for(int j=0; j<size; j++){
            Vehicle* v = &q->data[(q->front+j) % MAX_QUEUE];
            if(!v->active) continue;
            
            // Check if vehicle should stop
            if(shouldStop(v)) continue;
            
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
        
        // Remove inactive vehicles
        int count = queueSize(q);
        for(int k=0; k<count; k++){
            Vehicle frontV = dequeue(q);
            if(frontV.active) {
                enqueue(q, frontV);
            }
        }
    }
    
    SDL_UnlockMutex(mutex);
}

DWORD WINAPI vehicleGenerator(LPVOID arg){
    char roads[] = "ABCD";
    while(1){
        Sleep(2000);  // Generate vehicle every 2 seconds
        
        char road = roads[rand() % 4];
        int lane = (rand() % 3) + 1;
        
        Vehicle v = createVehicle(road, lane);
        
        SDL_LockMutex(mutex);
        switch(road){
            case 'A':
                switch(lane) {
                    case 1: enqueue(&AL1, v); break;
                    case 2: enqueue(&AL2, v); break;
                    case 3: enqueue(&AL3, v); break;
                }
                break;
            case 'B':
                switch(lane) {
                    case 1: enqueue(&BL1, v); break;
                    case 2: enqueue(&BL2, v); break;
                    case 3: enqueue(&BL3, v); break;
                }
                break;
            case 'C':
                switch(lane) {
                    case 1: enqueue(&CL1, v); break;
                    case 2: enqueue(&CL2, v); break;
                    case 3: enqueue(&CL3, v); break;
                }
                break;
            case 'D':
                switch(lane) {
                    case 1: enqueue(&DL1, v); break;
                    case 2: enqueue(&DL2, v); break;
                    case 3: enqueue(&DL3, v); break;
                }
                break;
        }
        SDL_UnlockMutex(mutex);
        
        printf("Vehicle %d created on Road %c, Lane %d\n", v.id, road, lane);
    }
    return 0;
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

    window = SDL_CreateWindow("Traffic Junction Simulator",
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

    CreateThread(NULL, 0, vehicleGenerator, NULL, 0, NULL);

    lastSwitchTime = SDL_GetTicks();
    bool running = true;

    printf("Traffic simulation started. Press window close to exit.\n");
    printf("Green light duration: %d ms\n", LIGHT_DURATION);

    while(running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
        }

        Uint32 now = SDL_GetTicks();
        if(now - lastSwitchTime > LIGHT_DURATION){
            currentGreen = (currentGreen + 1) % NUM_ROADS;
            lastSwitchTime = now;
            printf("Light switched to Road %c\n", 'A' + currentGreen);
        }
        
        moveVehicles();

        // Render
        drawBackground();// calls the background function
        drawRoads();
        drawLights();

        drawQueue(&AL1); drawQueue(&AL2); drawQueue(&AL3);
        drawQueue(&BL1); drawQueue(&BL2); drawQueue(&BL3);
        drawQueue(&CL1); drawQueue(&CL2); drawQueue(&CL3);
        drawQueue(&DL1); drawQueue(&DL2); drawQueue(&DL3);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyMutex(mutex);
    SDL_Quit();
    
    printf("Simulation ended.\n");
    return 0;
}