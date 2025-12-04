#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800 
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define MAX_VEHICLES 100
#define MAIN_FONT "C:\\Windows\\Fonts\\Arial.ttf"
#define VEHICLE_FILE "vehicles.data"

// Vehicle structure
typedef struct {
    char id[10];
    char road;   // A/B/C/D
    int lane;    // 1/2/3
} Vehicle;

// Shared data between threads
typedef struct {
    int currentGreen;    // 0=A,1=B,2=C,3=D
    int counts[4];       // number of vehicles waiting in priority lane (lane 2)
    SDL_mutex* mutex;
} SharedData;

SharedData sharedData;

// Vehicle queue
Vehicle vehicleQueue[MAX_VEHICLES];
int vehicleCount = 0;

// SDL objects
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

// Function declarations
bool initSDL();
void drawRoads();
void drawLights();
void drawText(const char* text, int x, int y);
DWORD WINAPI readVehicles(LPVOID arg);
DWORD WINAPI manageLights(LPVOID arg);
void refreshScreen();
int getPriorityRoad();

int main(int argc, char* argv[]) {
    if (!initSDL()) return -1;

    SDL_Event event;
    bool running = true;

    // Initialize shared data
    sharedData.currentGreen = 0;
    for(int i=0;i<4;i++) sharedData.counts[i]=0;
    sharedData.mutex = SDL_CreateMutex();

    // Start threads
    HANDLE hReadThread = CreateThread(NULL,0,readVehicles,NULL,0,NULL);
    HANDLE hLightThread = CreateThread(NULL,0,manageLights,NULL,0,NULL);

    while(running) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) running = false;
        }
        refreshScreen();
        SDL_Delay(50); // 20 FPS
    }

    WaitForSingleObject(hReadThread, INFINITE);
    WaitForSingleObject(hLightThread, INFINITE);

    SDL_DestroyMutex(sharedData.mutex);
    if(font) TTF_CloseFont(font);
    if(renderer) SDL_DestroyRenderer(renderer);
    if(window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

bool initSDL() {
    if(SDL_Init(SDL_INIT_VIDEO) <0) { SDL_Log("SDL Init failed: %s",SDL_GetError()); return false; }
    if(TTF_Init() <0) { SDL_Log("TTF Init failed: %s",TTF_GetError()); return false; }

    window = SDL_CreateWindow("Traffic Junction", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if(!window){ SDL_Log("Window failed: %s", SDL_GetError()); return false; }

    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    if(!renderer){ SDL_Log("Renderer failed: %s",SDL_GetError()); return false; }

    font = TTF_OpenFont(MAIN_FONT,24);
    if(!font){ SDL_Log("Font failed: %s",TTF_GetError()); return false; }

    return true;
}

void drawRoads() {
    SDL_SetRenderDrawColor(renderer, 200,200,200,255);
    SDL_Rect vertical = {WINDOW_WIDTH/2 - ROAD_WIDTH/2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_Rect horizontal = {0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &vertical);
    SDL_RenderFillRect(renderer, &horizontal);

    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    for(int i=0;i<=3;i++){
        // horizontal lanes
        SDL_RenderDrawLine(renderer,0,WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, WINDOW_WIDTH/2 - ROAD_WIDTH/2, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i);
        SDL_RenderDrawLine(renderer,WINDOW_WIDTH,WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, WINDOW_WIDTH/2 + ROAD_WIDTH/2, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + LANE_WIDTH*i);
        // vertical lanes
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i,0, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, WINDOW_HEIGHT/2 - ROAD_WIDTH/2);
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i,WINDOW_HEIGHT, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + LANE_WIDTH*i, WINDOW_HEIGHT/2 + ROAD_WIDTH/2);
    }

    drawText("A", WINDOW_WIDTH/2, 10);
    drawText("B", WINDOW_WIDTH/2, WINDOW_HEIGHT - 40);
    drawText("C", WINDOW_WIDTH - 40, WINDOW_HEIGHT/2);
    drawText("D", 10, WINDOW_HEIGHT/2);
}

void drawLights() {
    SDL_Rect rect = {WINDOW_WIDTH/2 - 25, WINDOW_HEIGHT/2 - 25, 50, 50};
    for(int i=0;i<4;i++){
        if(sharedData.currentGreen==i) SDL_SetRenderDrawColor(renderer,0,255,0,255);
        else SDL_SetRenderDrawColor(renderer,255,0,0,255);
        switch(i){
            case 0: rect.x = WINDOW_WIDTH/2 -25; rect.y=10; break;   // A top
            case 1: rect.x = WINDOW_WIDTH/2 -25; rect.y=WINDOW_HEIGHT-60; break; // B bottom
            case 2: rect.x = WINDOW_WIDTH-60; rect.y=WINDOW_HEIGHT/2 -25; break; // C right
            case 3: rect.x =10; rect.y=WINDOW_HEIGHT/2 -25; break; // D left
        }
        SDL_RenderFillRect(renderer,&rect);
    }
}

void drawText(const char* text, int x, int y){
    SDL_Color color = {0,0,0,255};
    SDL_Surface* surf = TTF_RenderText_Solid(font,text,color);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer,surf);
    SDL_Rect dst = {x,y,0,0};
    SDL_QueryTexture(tex,NULL,NULL,&dst.w,&dst.h);
    SDL_RenderCopy(renderer,tex,NULL,&dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

DWORD WINAPI readVehicles(LPVOID arg){
    while(1){
        FILE* file = fopen(VEHICLE_FILE,"r");
        if(!file){ Sleep(2000); continue; }

        SDL_LockMutex(sharedData.mutex);
        vehicleCount=0;
        for(int i=0;i<4;i++) sharedData.counts[i]=0;

        char line[50];
        while(fgets(line,sizeof(line),file) && vehicleCount<MAX_VEHICLES){
            line[strcspn(line,"\n")]=0;
            Vehicle v;
            if(sscanf(line,"%c %d %s",&v.road,&v.lane,v.id)==3){
                vehicleQueue[vehicleCount++] = v;
                if(v.lane==2){ // priority lane
                    int idx = v.road-'A';
                    if(idx>=0 && idx<4) sharedData.counts[idx]++;
                }
            }
        }
        SDL_UnlockMutex(sharedData.mutex);
        fclose(file);
        Sleep(1000);
    }
    return 0;
}

int getPriorityRoad(){
    SDL_LockMutex(sharedData.mutex);
    int road=-1;
    for(int i=0;i<4;i++){
        if(sharedData.counts[i]>10){ road=i; break; }
    }
    SDL_UnlockMutex(sharedData.mutex);
    return road;
}

DWORD WINAPI manageLights(LPVOID arg){
    int order[4] = {0,1,2,3};
    int idx=0;
    while(1){
        int prio = getPriorityRoad();
        SDL_LockMutex(sharedData.mutex);
        if(prio!=-1) sharedData.currentGreen=prio;
        else{
            sharedData.currentGreen = order[idx];
            idx = (idx+1)%4;
        }
        SDL_UnlockMutex(sharedData.mutex);
        Sleep(5000);
    }
    return 0;
}

void refreshScreen(){
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderClear(renderer);
    drawRoads();
    drawLights();
    SDL_RenderPresent(renderer);
}
