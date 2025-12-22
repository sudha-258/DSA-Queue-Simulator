#include <windows.h>
#include <stdio.h>
#include <string.h>

#define PIPE_NAME "\\\\.\\pipe\\VehicleQueue"
#define MAX_TEXT 100

int main() {
    HANDLE hPipe = CreateFile(
        PIPE_NAME,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("Failed to open pipe. Error: %d\n", GetLastError());
        return 1;
    }

    char msg[MAX_TEXT] = "Car-Bus-Bike";
    DWORD bytesWritten;

    if (!WriteFile(hPipe, msg, (DWORD)strlen(msg) + 1, &bytesWritten, NULL)) {
        printf("WriteFile failed. Error: %d\n", GetLastError());
        return 1;
    }

    printf("Message sent: %s\n", msg);

    CloseHandle(hPipe);
    return 0;
}
