#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <locale.h>

#define BUFFER_SIZE 1024

void PrintError(const char* message) {
    HANDLE hStdError = GetStdHandle(STD_ERROR_HANDLE);
    DWORD bytesWritten;
    WriteFile(hStdError, message, strlen(message), &bytesWritten, NULL);
}

int main() {
    setlocale(LC_ALL, "RUSSIAN");
    SetConsoleOutputCP(CP_UTF8);  // Устанавливаем кодировку UTF-8

    HANDLE hReadPipe = GetStdHandle(STD_INPUT_HANDLE);
    if (hReadPipe == INVALID_HANDLE_VALUE) {
        PrintError("error getting STD_INPUT_HANDLE\n");
        return 1;
    }

    // Читаем имя файла
    wchar_t wFilename[BUFFER_SIZE];
    DWORD bytesRead;
    if (!ReadFile(hReadPipe, wFilename, sizeof(wFilename) - sizeof(wchar_t), &bytesRead, NULL)) {
        PrintError("reading file name error\n");
        return 1;
    }

    if (bytesRead > 0) {
        wFilename[bytesRead / sizeof(wchar_t)] = L'\0';  // Завершаем строку
    }

    // Открываем файл для записи
    HANDLE hFile = CreateFileW(
        wFilename,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        PrintError("open file error\n");
        return 1;
    }

    // Чтение данных
    while (1) {
        char buffer[BUFFER_SIZE];
        if (!ReadFile(hReadPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL)) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                break;
            }
            PrintError("reading data error\n");
            break;
        }

        if (bytesRead == 0) break;
        buffer[bytesRead] = '\0';

        if (strncmp(buffer, "exit", 4) == 0) break;

        int sum = 0;
        char* token = strtok(buffer, " ");
        while (token != NULL) {
            sum += atoi(token);
            token = strtok(NULL, " ");
        }

        char outputBuffer[BUFFER_SIZE];
        int outputLength = snprintf(outputBuffer, BUFFER_SIZE, "Summ: %d\n", sum);
        DWORD bytesWritten;
        WriteFile(hFile, outputBuffer, outputLength, &bytesWritten, NULL);
    }

    CloseHandle(hFile);
    CloseHandle(hReadPipe);
    return 0;
}
