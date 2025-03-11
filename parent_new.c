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

    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;  // Дескрипторы должны наследоваться
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
        PrintError("pipe creation error\n");
        return 1;
    }

    // Отключаем наследование hWritePipe в родительском процессе
    if (!SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0)) {
        PrintError("error SetHandleInformation\n");
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return 1;
    }

    // Создание дочернего процесса
    PROCESS_INFORMATION procInfo;
    STARTUPINFOW startInfo;
    ZeroMemory(&startInfo, sizeof(STARTUPINFOW));
    startInfo.cb = sizeof(STARTUPINFOW);
    startInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    startInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startInfo.hStdInput = hReadPipe;  // Передаём дескриптор чтения
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    wchar_t cmdLine[] = L"child.exe";
    if (!CreateProcessW(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &startInfo, &procInfo)) {
        PrintError("child process creation error\n");
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return 1;
    }

    CloseHandle(hReadPipe);  // Родительский процесс не использует hReadPipe

    // Ввод имени файла
    wchar_t wFilename[BUFFER_SIZE];
    fputws(L"enter file name: ", stdout);
    fgetws(wFilename, BUFFER_SIZE, stdin);
    wFilename[wcslen(wFilename) - 1] = L'\0';  // Убираем символ новой строки

    // Отправка имени файла дочернему процессу
    DWORD bytesWritten;
    if (!WriteFile(hWritePipe, wFilename, wcslen(wFilename) * sizeof(wchar_t), &bytesWritten, NULL)) {
        PrintError("error sending file name\n");
        CloseHandle(hWritePipe);
        return 1;
    }

    // Отправка данных
    char buffer[BUFFER_SIZE];
    while (1) {
        fputs("enter numbers or exit: ", stdout);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strlen(buffer) - 1] = '\0';

        if (strcmp(buffer, "exit") == 0) break;

        if (!WriteFile(hWritePipe, buffer, strlen(buffer) + 1, &bytesWritten, NULL)) {
            PrintError("error sending data\n");
            break;
        }
    }

    CloseHandle(hWritePipe);  // Теперь можно закрыть hWritePipe
    WaitForSingleObject(procInfo.hProcess, INFINITE);
    CloseHandle(procInfo.hProcess);

    return 0;
}
