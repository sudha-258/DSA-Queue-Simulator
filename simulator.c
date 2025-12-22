#include <SDL2/SDL.h>

#include <stdio.h>

#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define VEHICLE_WIDTH 40
#define VEHICLE_HEIGHT 30
#define NUM_VEHICLES 8
#define LIGHT_DURATION 3000  // 3 seconds per light

typedef struct {
    int x, y;
    char road; // 'A'=top, 'B'=bottom, 'C'=right, 'D'=left
    bool active;
} Vehicle;

// Global SDL objects
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// Traffic light state (0=A, 1=B, 2=C, 3=D)
int currentGreen = 0;
Uint32 lastSwitchTime = 0;

// Vehicles
Vehicle vehicles[NUM_VEHICLES];

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Init failed: %s\n", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("4-Way Traffic Junction", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    return true;
}

// Initialize vehicles at road entries
void initVehicles() {
    for (int i = 0; i < NUM_VEHICLES; i++) {
        vehicles[i].active = true;
        switch (i % 4) {
            case 0: vehicles[i].road = 'A'; vehicles[i].x = WINDOW_WIDTH/2 - LANE_WIDTH + (i%2)*LANE_WIDTH; vehicles[i].y = 0; break;
            case 1: vehicles[i].road = 'B'; vehicles[i].x = WINDOW_WIDTH/2 - LANE_WIDTH + (i%2)*LANE_WIDTH; vehicles[i].y = WINDOW_HEIGHT; break;
            case 2: vehicles[i].road = 'C'; vehicles[i].x = WINDOW_WIDTH; vehicles[i].y = WINDOW_HEIGHT/2 - LANE_WIDTH + (i%2)*LANE_WIDTH; break;
            case 3: vehicles[i].road = 'D'; vehicles[i].x = 0; vehicles[i].y = WINDOW_HEIGHT/2 - LANE_WIDTH + (i%2)*LANE_WIDTH; break;
        }
    }
}

// Draw roads
void drawRoads() {
    SDL_SetRenderDrawColor(renderer, 200,200,200,255);

    SDL_Rect vertical = {WINDOW_WIDTH/2 - ROAD_WIDTH/2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_Rect horizontal = {0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &vertical);
    SDL_RenderFillRect(renderer, &horizontal);

    // Lane lines
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    for(int i=1;i<4;i++){
        // Horizontal
        SDL_RenderDrawLine(renderer, 0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i,
                                      WINDOW_WIDTH, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i);
        // Vertical
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, 0,
                                      WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, WINDOW_HEIGHT);
    }
}

// Draw traffic lights
void drawLights() {
    SDL_Rect rect = {0,0,20,20};
    for(int i=0;i<4;i++){
        if(currentGreen==i) SDL_SetRenderDrawColor(renderer,0,255,0,255);
        else SDL_SetRenderDrawColor(renderer,255,0,0,255);
        switch(i){
            case 0: rect.x = WINDOW_WIDTH/2 - 10; rect.y = 10; break; // A top
            case 1: rect.x = WINDOW_WIDTH/2 - 10; rect.y = WINDOW_HEIGHT - 30; break; // B bottom
            case 2: rect.x = WINDOW_WIDTH - 30; rect.y = WINDOW_HEIGHT/2 - 10; break; // C right
            case 3: rect.x = 10; rect.y = WINDOW_HEIGHT/2 - 10; break; // D left
        }
        SDL_RenderFillRect(renderer,&rect);
    }
}

// Move vehicles based on traffic light
void moveVehicles() {
    for (int i=0;i<NUM_VEHICLES;i++){
        if(!vehicles[i].active) continue;
        int roadIdx = vehicles[i].road-'A';
        if(roadIdx != currentGreen) continue;

        switch(vehicles[i].road){
            case 'A': vehicles[i].y += 2; if(vehicles[i].y>WINDOW_HEIGHT) vehicles[i].active=false; break;
            case 'B': vehicles[i].y -= 2; if(vehicles[i].y<0) vehicles[i].active=false; break;
            case 'C': vehicles[i].x -= 2; if(vehicles[i].x<0) vehicles[i].active=false; break;
            case 'D': vehicles[i].x += 2; if(vehicles[i].x>WINDOW_WIDTH) vehicles[i].active=false; break;
        }
    }
}

// Draw vehicles
void drawVehicles() {
    SDL_SetRenderDrawColor(renderer, 0,0,255,255);
    for(int i=0;i<NUM_VEHICLES;i++){
        if(!vehicles[i].active) continue;
        SDL_Rect rect = {vehicles[i].x, vehicles[i].y, VEHICLE_WIDTH, VEHICLE_HEIGHT};
        SDL_RenderFillRect(renderer,&rect);
    }
}

int main(int argc, char* argv[]) {
    if(!initSDL()) return -1;
    initVehicles();

    bool running = true;
    SDL_Event event;

    lastSwitchTime = SDL_GetTicks();

    while(running){
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT) running=false;
        }

        Uint32 now = SDL_GetTicks();
        if(now - lastSwitchTime > LIGHT_DURATION){
            currentGreen = (currentGreen+1)%4; // rotate green light
            lastSwitchTime = now;
        }

        moveVehicles();

        SDL_SetRenderDrawColor(renderer, 0,150,0,255);
        SDL_RenderClear(renderer);

        drawRoads();
        drawLights();
        drawVehicles();

        SDL_RenderPresent(renderer);

        SDL_Delay(20); // ~50 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
