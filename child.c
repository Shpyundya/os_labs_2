#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <locale.h>

#define BUFFER_SIZE 1024
#define SHARED_MEMORY_NAME "Local\\MySharedMemory"
#define SEM_EMPTY_NAME "Local\\MySemEmpty"
#define SEM_FULL_NAME "Local\\MySemFull"

typedef struct {
    char buffer[BUFFER_SIZE];
    int data_ready;
} SharedData;

int main() {
    setlocale(LC_ALL, "");

    // Открытие разделяемой памяти
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        fprintf(stderr, "OpenFileMapping failed: %d\n", GetLastError());
        return 1;
    }

    SharedData *sharedData = (SharedData *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (sharedData == NULL) {
        fprintf(stderr, "MapViewOfFile failed: %d\n", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    // Открытие семафоров
    HANDLE hSemEmpty = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_EMPTY_NAME);
    HANDLE hSemFull = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_FULL_NAME);
    if (hSemEmpty == NULL || hSemFull == NULL) {
        fprintf(stderr, "OpenSemaphore failed: %d\n", GetLastError());
        return 1;
    }

    // Получение имени файла
    WaitForSingleObject(hSemFull, INFINITE);
    char filename[BUFFER_SIZE];
    strcpy(filename, sharedData->buffer);
    ReleaseSemaphore(hSemEmpty, 1, NULL);

    printf("Дочерний процесс получил имя файла: %s\n", filename);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "Ошибка открытия файла.\n");
        return 1;
    }

    while (1) {
        WaitForSingleObject(hSemFull, INFINITE);

        if (strncmp(sharedData->buffer, "exit", 4) == 0) {
            ReleaseSemaphore(hSemEmpty, 1, NULL);
            break;
        }

        // Вычисление суммы чисел
        int sum = 0;
        char *token = strtok(sharedData->buffer, " \n");
        while (token != NULL) {
            sum += atoi(token);
            token = strtok(NULL, " \n");
        }

        // Запись результата в файл
        fprintf(file, "Сумма: %d\n", sum);
        fflush(file);

        // Отправка результата родительскому процессу
        snprintf(sharedData->buffer, BUFFER_SIZE, "%d", sum);
        ReleaseSemaphore(hSemFull, 1, NULL);
    }

    fclose(file);
    UnmapViewOfFile(sharedData);
    CloseHandle(hMapFile);
    CloseHandle(hSemEmpty);
    CloseHandle(hSemFull);

    return 0;
}