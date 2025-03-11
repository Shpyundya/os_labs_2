#include <windows.h>
#include <locale.h>
#include <string.h>

#define BUFFER_SIZE 1024 // размер буфера для хранения данных
#define SHARED_MEMORY_NAME "Local\\MySharedMemory" // имя разделяемой памяти и семафоров
#define SEM_EMPTY_NAME "Local\\MySemEmpty"
#define SEM_FULL_NAME "Local\\MySemFull"
#define LOG_FILE "parent_log.txt" // файл для логирования ошибок

typedef struct { // определение структуры разделяемой памяти
    char buffer[BUFFER_SIZE];
    int data_ready;  // Флаг для синхронизации, есть ли в памяти новые данные
} SharedData;

// Функция для записи данных в файл
void WriteToFile(const char *filename, const char *data) {
    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return; // Если не удалось открыть файл, просто выходим
    }

    // Перемещаем указатель в конец файла
    SetFilePointer(hFile, 0, NULL, FILE_END);

    DWORD bytesWritten;
    WriteFile(hFile, data, strlen(data), &bytesWritten, NULL);
    CloseHandle(hFile);
}

// Функция для обработки ошибок с записью в лог-файл
void LogError(const char *message) {
    char errorMsg[256];
    wsprintf(errorMsg, "%s: %d\n", message, GetLastError());
    WriteToFile(LOG_FILE, errorMsg);
}

// Функция для вывода строки в консоль
void WriteToConsole(const char *message) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD bytesWritten;
    WriteConsole(hConsole, message, strlen(message), &bytesWritten, NULL);
}

// Функция для чтения строки из консоли
void ReadFromConsole(char *buffer, DWORD bufferSize) {
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD bytesRead;
    ReadConsole(hConsole, buffer, bufferSize - 1, &bytesRead, NULL);
    buffer[bytesRead] = '\0'; // Добавляем завершающий нулевой символ
}

int main() {
    setlocale(LC_ALL, "");

    // Создание объекта разделяемой памяти
    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedData), SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        LogError("CreateFileMapping failed");
        return 1;
    }

    // Отображение разделяемой памяти в адресное пространство
    SharedData *sharedData = (SharedData *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (sharedData == NULL) {
        LogError("MapViewOfFile failed");
        CloseHandle(hMapFile);
        return 1;
    }

    // Инициализация семафоров
    HANDLE hSemEmpty = CreateSemaphore(NULL, 1, 1, SEM_EMPTY_NAME);
    HANDLE hSemFull = CreateSemaphore(NULL, 0, 1, SEM_FULL_NAME);
    if (hSemEmpty == NULL || hSemFull == NULL) {
        LogError("CreateSemaphore failed");
        CloseHandle(hMapFile);
        return 1;
    }

    // Запуск дочернего процесса
    PROCESS_INFORMATION procInfo;
    STARTUPINFO startInfo;
    ZeroMemory(&startInfo, sizeof(STARTUPINFO));
    startInfo.cb = sizeof(STARTUPINFO);

    if (!CreateProcess("child.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &procInfo)) {
        LogError("CreateProcess failed");
        CloseHandle(hMapFile);
        return 1;
    }

    // Запрос имени файла
    WriteToConsole("Введите имя файла: ");
    ReadFromConsole(sharedData->buffer, BUFFER_SIZE);
    sharedData->buffer[strcspn(sharedData->buffer, "\n")] = 0;  // Удаление символа новой строки

    // Отправка имени файла дочернему процессу
    WaitForSingleObject(hSemEmpty, INFINITE);
    sharedData->data_ready = 1;
    ReleaseSemaphore(hSemFull, 1, NULL);

    while (1) {
        // Ввод команд от пользователя
        WriteToConsole("enter number or exit: ");
        ReadFromConsole(sharedData->buffer, BUFFER_SIZE);

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
        WriteToConsole("Summ: ");
        WriteToConsole(sharedData->buffer);
        WriteToConsole("\n");
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
