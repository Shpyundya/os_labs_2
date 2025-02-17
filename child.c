#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <locale.h>

#define BUFFER_SIZE 1024

int main() {
    setlocale(LC_ALL, "Rus");
    // Получение стандартного ввода и вывода
    HANDLE hReadPipe = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hWritePipe = GetStdHandle(STD_OUTPUT_HANDLE);

    // Чтение имени файла
    char filename[BUFFER_SIZE];
    DWORD bytesRead;
    if (!ReadFile(hReadPipe, filename, BUFFER_SIZE, &bytesRead, NULL)) {
        fprintf(stderr, "ReadFile failed: %d\n", GetLastError());
        return 1;
    }
    filename[bytesRead] = '\0'; // Убедимся, что строка корректно завершена
    printf("Дочерний процесс получил имя файла: %s\n", filename);

    // Открытие файла для записи
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        fprintf(stderr, "fopen failed: %d\n", GetLastError());
        return 1;
    }

    // Основной цикл обработки команд
    while (1) {
        char buffer[BUFFER_SIZE];
        DWORD bytesRead;
        if (!ReadFile(hReadPipe, buffer, BUFFER_SIZE, &bytesRead, NULL)) {
            fprintf(stderr, "ReadFile failed: %d\n", GetLastError());
            break; // Выйти из цикла, если чтение не удалось
        }
        
        // Проверка на завершение
        if (bytesRead == 0) {
            break;
        }

        // Проверка на выход
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }

        // Разбор чисел и вычисление суммы
        int sum = 0;
        char *token = strtok(buffer, " \n");
        while (token != NULL) {
            int num = atoi(token);
            sum += num;
            token = strtok(NULL, " \n");
        }

        // Запись суммы в файл
        fprintf(file, "Сумма: %d\n", sum);
        fflush(file);

        // Отправка суммы обратно через стандартный вывод
        snprintf(buffer, BUFFER_SIZE, "%d", sum);
        DWORD bytesWritten;
        WriteFile(hWritePipe, buffer, strlen(buffer) + 1, &bytesWritten, NULL);
    }

    // Закрытие ресурсов
    fclose(file);
    return 0;
}