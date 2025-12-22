#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

#define PIPE_NAME "\\\\.\\pipe\\VehicleQueue"
#define MAX_TEXT 100

struct message {
    char vehicleQueue[MAX_TEXT];
};

// Generate a random vehicle number
// Format: <2 alpha><1 digit><2 alpha><3 digit>
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

// Generate a random lane
char generateLane() {
    char lanes[] = {'A', 'B', 'C', 'D'};
    return lanes[rand() % 4];
}

int main() {
    srand((unsigned int)time(NULL));

    HANDLE hPipe = CreateNamedPipe(
        PIPE_NAME,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,              // max instances
        MAX_TEXT,       // out buffer size
        MAX_TEXT,       // in buffer size
        0,              // default timeout
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("Failed to create pipe. Error: %d\n", GetLastError());
        return 1;
    }

    printf("Waiting for receiver to connect...\n");
    ConnectNamedPipe(hPipe, NULL);
    printf("Receiver connected!\n");

    struct message msg;

    while (1) {
        char vehicle[9];
        generateVehicleNumber(vehicle);
        char lane = generateLane();
        snprintf(msg.vehicleQueue, MAX_TEXT, "%s:%c", vehicle, lane);

        DWORD bytesWritten;
        if (!WriteFile(hPipe, msg.vehicleQueue, (DWORD)strlen(msg.vehicleQueue) + 1, &bytesWritten, NULL)) {
            printf("WriteFile failed. Error: %d\n", GetLastError());
            break;
        }

        printf("New vehicle added: %s\n", msg.vehicleQueue);
        Sleep(1000); // 1 second delay
    }

    CloseHandle(hPipe);
    return 0;
}
