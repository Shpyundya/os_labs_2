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
    int data_ready;  // Флаг для синхронизации
} SharedData;

int main() {
    setlocale(LC_ALL, "");

    // Создание объекта разделяемой памяти
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedData), SHARED_MEMORY_NAME);

    if (hMapFile == NULL) {
        fprintf(stderr, "CreateFileMapping failed: %d\n", GetLastError());
        return 1;
    }

    // Отображение разделяемой памяти в адресное пространство
    SharedData *sharedData = (SharedData *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (sharedData == NULL) {
        fprintf(stderr, "MapViewOfFile failed: %d\n", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    // Инициализация семафоров
    HANDLE hSemEmpty = CreateSemaphore(NULL, 1, 1, SEM_EMPTY_NAME);
    HANDLE hSemFull = CreateSemaphore(NULL, 0, 1, SEM_FULL_NAME);
    if (hSemEmpty == NULL || hSemFull == NULL) {
        fprintf(stderr, "CreateSemaphore failed: %d\n", GetLastError());
        return 1;
    }

    // Запуск дочернего процесса
    PROCESS_INFORMATION procInfo;
    STARTUPINFO startInfo;
    ZeroMemory(&startInfo, sizeof(STARTUPINFO));
    startInfo.cb = sizeof(STARTUPINFO);

    if (!CreateProcess("child.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &procInfo)) {
        fprintf(stderr, "CreateProcess failed: %d\n", GetLastError());
        return 1;
    }

    // Запрос имени файла
    printf("Введите имя файла: ");
    fgets(sharedData->buffer, BUFFER_SIZE, stdin);
    sharedData->buffer[strcspn(sharedData->buffer, "\n")] = 0;  // Удаление символа новой строки

    // Отправка имени файла дочернему процессу
    WaitForSingleObject(hSemEmpty, INFINITE);
    sharedData->data_ready = 1;
    ReleaseSemaphore(hSemFull, 1, NULL);

    while (1) {
        // Ввод команд от пользователя
        printf("Введите числа (через пробел) или 'exit' для выхода: ");
        fgets(sharedData->buffer, BUFFER_SIZE, stdin);

        if (strncmp(sharedData->buffer, "exit", 4) == 0) {
            WaitForSingleObject(hSemEmpty, INFINITE);
            sharedData->data_ready = 1;
            ReleaseSemaphore(hSemFull, 1, NULL);
            break;
        }

        // Отправка данных дочернему процессу
        WaitForSingleObject(hSemEmpty, INFINITE);
        sharedData->data_ready = 1;
        ReleaseSemaphore(hSemFull, 1, NULL);

        // Получение результата от дочернего процесса
        WaitForSingleObject(hSemFull, INFINITE);
        printf("Сумма: %s\n", sharedData->buffer);
        ReleaseSemaphore(hSemEmpty, 1, NULL);
    }

    // Освобождение ресурсов
    UnmapViewOfFile(sharedData);
    CloseHandle(hMapFile);
    CloseHandle(hSemEmpty);
    CloseHandle(hSemFull);
    WaitForSingleObject(procInfo.hProcess, INFINITE);
    CloseHandle(procInfo.hProcess);

    return 0;
}