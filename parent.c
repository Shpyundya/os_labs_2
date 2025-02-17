#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <locale.h>

#define BUFFER_SIZE 1024

int main() {
    setlocale(LC_ALL, "RUSSIAN");
    // Создание канала
    HANDLE hReadPipe, hWritePipe; // hreadpipe - дискриптор для чтения, hwritepipe - дискриптор для записи
    SECURITY_ATTRIBUTES saAttr; // var in WinApi для настройки параметров безопасности
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE; // Наследование
    saAttr.lpSecurityDescriptor = NULL;

    // Создаем анонимные каналы
    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
        fprintf(stderr, "CreatePipe failed: %d\n", GetLastError());
        return 1;
    }

    // Создание дочернего процесса
    PROCESS_INFORMATION procInfo;
    STARTUPINFO startInfo; //для создания параметров нового процесса
    ZeroMemory(&startInfo, sizeof(STARTUPINFO));
    startInfo.cb = sizeof(STARTUPINFO);
    startInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    startInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startInfo.hStdInput = hReadPipe; // Устанавливаем стандартный ввод
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Запуск дочернего процесса
    if (!CreateProcess("child.exe", NULL, NULL, NULL, TRUE, 0, NULL, NULL, &startInfo, &procInfo)) {
        fprintf(stderr, "CreateProcess failed: %d\n", GetLastError());
        return 1;
    }

    // Закрытие ненужного дескриптора записи в родительском процессе
    CloseHandle(procInfo.hThread);

    // Получаем имя файла от пользователя
    char filename[BUFFER_SIZE];
    printf("Введите имя файла: ");
    fgets(filename, BUFFER_SIZE, stdin);
    filename[strcspn(filename, "\n")] = 0; // Убираем символ новой строки

    // Отправляем имя файла в дочерний процесс
    DWORD bytesWritten;
    if (!WriteFile(hWritePipe, filename, strlen(filename) + 1, &bytesWritten, NULL)) {
        fprintf(stderr, "WriteFile failed: %d\n", GetLastError());
        return 1;
    }

    // Основной цикл для получения команд от пользователя
    while (1) {
        char buffer[BUFFER_SIZE];
        printf("Введите команды (числа через пробел или 'exit' для выхода): ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Проверка на выход
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }

        // Отправка команд дочернему процессу
        if (!WriteFile(hWritePipe, buffer, strlen(buffer) + 1, &bytesWritten, NULL)) {
            fprintf(stderr, "WriteFile failed: %d\n", GetLastError());
            break;
        }

        // Чтение результата из дочернего процесса
        char resultBuffer[BUFFER_SIZE];
        DWORD bytesRead;
        if (!ReadFile(hReadPipe, resultBuffer, BUFFER_SIZE, &bytesRead, NULL)) {
            fprintf(stderr, "ReadFile failed: %d\n", GetLastError());
            break;
        }

        if (bytesRead > 0) {
            printf("Сумма: %s\n", resultBuffer);
        } else {
            printf("Дочерний процесс завершился.\n");
            break;
        }
    }

    // Закрытие дескрипторов
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);

    // Ожидание завершения дочернего процесса
    WaitForSingleObject(procInfo.hProcess, INFINITE);
    CloseHandle(procInfo.hProcess);

    return 0;
}