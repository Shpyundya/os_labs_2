#include <windows.h>
#include <locale.h>
#include <string.h>

#define BUFFER_SIZE 1024 // размер буфера для хранения данных
#define SHARED_MEMORY_NAME "Local\\MySharedMemory" // подключаемся к разделяемой памяти
#define SEM_EMPTY_NAME "Local\\MySemEmpty" // сигнализирует, что память пуста
#define SEM_FULL_NAME "Local\\MySemFull" // сигнализирует, что память заполнена
#define LOG_FILE "child_log.txt" // файл для логирования ошибок

typedef struct { // определение структуры для разделяемой памяти
    char buffer[BUFFER_SIZE]; // Массив символов, который используется для передачи данных между процессами
    int data_ready; // флаг, указывающий, что в памяти новые данные
} SharedData;

// Функция для записи данных в файл (используется только для логирования ошибок)
void WriteToFile(const char *filename, const char *data) {
    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return; // Если не удалось открыть файл, просто выходим
    }
    //OPEN_ALWAYS - указывает, что файл будет открыт, а если его нет, он будет создан.

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
    //wsprintf: Аналог sprintf, но предназначен для работы 
    //с широкими символами (в данном случае для форматирования строки).
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

int main() {
    setlocale(LC_ALL, "");

    // Открытие разделяемой памяти
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEMORY_NAME);
    if (hMapFile == NULL) {
        LogError("OpenFileMapping failed");
        return 1;
    }

    SharedData *sharedData = (SharedData *)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (sharedData == NULL) {
        LogError("MapViewOfFile failed");
        CloseHandle(hMapFile);
        return 1;
    }

    // Открытие семафоров
    HANDLE hSemEmpty = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_EMPTY_NAME);
    HANDLE hSemFull = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_FULL_NAME);
    if (hSemEmpty == NULL || hSemFull == NULL) {
        LogError("OpenSemaphore failed");
        CloseHandle(hMapFile);
        return 1;
    }

    // Получение имени файла
    WaitForSingleObject(hSemFull, INFINITE);
    char filename[BUFFER_SIZE];
    strcpy(filename, sharedData->buffer);
    ReleaseSemaphore(hSemEmpty, 1, NULL);

    WriteToConsole("Child process got the file name: ");
    WriteToConsole(filename);
    WriteToConsole("\n");

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

        // Вывод суммы на экран через WriteToConsole
        WriteToConsole("Sum: ");
        char sumStr[32];
        wsprintf(sumStr, "%d", sum);
        WriteToConsole(sumStr);
        WriteToConsole("\n");

        // Отправка результата родительскому процессу
        wsprintf(sharedData->buffer, "%d", sum);
        ReleaseSemaphore(hSemFull, 1, NULL);
    }

    UnmapViewOfFile(sharedData);
    CloseHandle(hMapFile);
    CloseHandle(hSemEmpty);
    CloseHandle(hSemFull);

    return 0;
}
