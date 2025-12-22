#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>  // For Sleep()

#define FILENAME "vehicles.data"

// Generate a random vehicle ID
void generateVehicleNumber(char* buffer) {
    buffer[0] = 'A' + rand() % 26;
    buffer[1] = 'A' + rand() % 26;
    buffer[2] = '0' + rand() % 10;
    buffer[3] = 'A' + rand() % 26;
    buffer[4] = 'A' + rand() % 26;
    buffer[5] = '0' + rand() % 10;
    buffer[6] = '0' + rand() % 10;
    buffer[7] = '0' + rand() % 10;
    buffer[8] = '\0';
}

// Generate random road A–D
char generateRoad() {
    char roads[] = {'A', 'B', 'C', 'D'};
    return roads[rand() % 4];
}

// Generate random lane number 1–3
int generateLane() {
    return (rand() % 3) + 1;
}

int main() {
    FILE* file = fopen(FILENAME, "a");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    srand(time(NULL));

    while (1) {
        char vehicle[9];
        generateVehicleNumber(vehicle);
        char road = generateRoad();
        int lane = generateLane();

        // Format: ROAD LANE VEHICLE_ID
        fprintf(file, "%c %d %s\n", road, lane, vehicle);
        fflush(file);

        printf("Generated: %c %d %s\n", road, lane, vehicle);

        Sleep(1000);  // Windows sleep (1 second)
    }

    fclose(file);
    return 0;
}
