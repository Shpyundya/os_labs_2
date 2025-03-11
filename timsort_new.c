#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>


#define MIN_MERGE 32
#define MAX_ARRAY_SIZE 1000000

typedef struct {
    int* arr;
    int left;
    int right;
    HANDLE semaphore;
} ThreadData;

// Вспомогательная функция для вывода в stdout с использованием WriteFile
void PrintStdout(const char* message) {
    HANDLE hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOutput == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD bytesWritten;
    WriteFile(hStdOutput, message, strlen(message), &bytesWritten, NULL);
}

// Вспомогательная функция для вывода ошибок в stderr с использованием WriteFile
void PrintError(const char* message) {
    HANDLE hStdError = GetStdHandle(STD_ERROR_HANDLE);
    if (hStdError == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD bytesWritten;
    WriteFile(hStdError, message, strlen(message), &bytesWritten, NULL);
}

// Функция сортировки вставками
void insertionSort(int arr[], int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int temp = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > temp) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = temp;
    }
}

// Функция слияния двух отсортированных подмассивов
void merge(int arr[], int l, int m, int r) {
    int len1 = m - l + 1, len2 = r - m;
    int* left = (int*)malloc(len1 * sizeof(int));
    int* right = (int*)malloc(len2 * sizeof(int));

    if (left == NULL || right == NULL) {
        PrintError("Memory allocation failed in merge\n");
        free(left);
        free(right);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < len1; i++)
        left[i] = arr[l + i];
    for (int i = 0; i < len2; i++)
        right[i] = arr[m + 1 + i];

    int i = 0, j = 0, k = l;
    while (i < len1 && j < len2) {
        if (left[i] <= right[j])
            arr[k++] = left[i++];
        else
            arr[k++] = right[j++];
    }

    while (i < len1)
        arr[k++] = left[i++];
    while (j < len2)
        arr[k++] = right[j++];

    free(left);
    free(right);
}

// Функция для вычисления минимального размера блока для TimSort
int minRunLength(int n) {
    int r = 0;
    while (n >= MIN_MERGE) {
        r |= (n & 1);
        n >>= 1;
    }
    return n + r;
}

// Функция потока для сортировки вставками
DWORD WINAPI insertionSortThread(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    insertionSort(data->arr, data->left, data->right);
    ReleaseSemaphore(data->semaphore, 1, NULL);
    return 0;
}

// Основная функция TimSort
void timSort(int arr[], int n, int maxThreads) {
    if (n <= 1) return;

    HANDLE semaphore = CreateSemaphore(NULL, maxThreads, maxThreads, NULL);
    if (semaphore == NULL) {
        char errMsg[100];
        snprintf(errMsg, sizeof(errMsg), "CreateSemaphore failed: %lu\n", GetLastError());
        PrintError(errMsg);
        exit(EXIT_FAILURE);
    }

    int minRun = minRunLength(n);
    int numRuns = 0;
    int* runStarts = (int*)malloc((n / minRun + 1) * sizeof(int));
    if (runStarts == NULL) {
        PrintError("Memory allocation failed for runStarts\n");
        CloseHandle(semaphore);
        exit(EXIT_FAILURE);
    }

    HANDLE* threads = (HANDLE*)malloc((n / minRun + 1) * sizeof(HANDLE));
    ThreadData* threadData = (ThreadData*)malloc((n / minRun + 1) * sizeof(ThreadData));
    if (threads == NULL || threadData == NULL) {
        PrintError("Memory allocation failed for threads/threadData\n");
        free(runStarts);
        free(threads);
        free(threadData);
        CloseHandle(semaphore);
        exit(EXIT_FAILURE);
    }

    int currentThreadCount = 0;
    for (int i = 0; i < n; i += minRun) {
        int end = (i + minRun - 1 < n - 1) ? (i + minRun - 1) : (n - 1);
        runStarts[numRuns++] = i;

        WaitForSingleObject(semaphore, INFINITE);

        threadData[currentThreadCount].arr = arr;
        threadData[currentThreadCount].left = i;
        threadData[currentThreadCount].right = end;
        threadData[currentThreadCount].semaphore = semaphore;

        threads[currentThreadCount] = CreateThread(NULL, 0, insertionSortThread, &threadData[currentThreadCount], 0, NULL);

        if (threads[currentThreadCount] == NULL) {
            char errMsg[100];
            snprintf(errMsg, sizeof(errMsg), "CreateThread failed: %lu\n", GetLastError());
            PrintError(errMsg);
            ReleaseSemaphore(semaphore, 1, NULL);
            for (int j = 0; j < currentThreadCount; ++j) {
                WaitForSingleObject(threads[j], INFINITE);
                CloseHandle(threads[j]);
            }
            free(runStarts);
            free(threads);
            free(threadData);
            CloseHandle(semaphore);
            exit(EXIT_FAILURE);
        }
        currentThreadCount++;
    }

    if (currentThreadCount > 0) {
        WaitForMultipleObjects(currentThreadCount, threads, TRUE, INFINITE);
    }

    for (int i = 0; i < currentThreadCount; ++i) {
        CloseHandle(threads[i]);
    }

    for (int size = minRun; size < n; size = 2 * size) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = (left + 2 * size - 1 < n - 1) ? (left + 2 * size - 1) : (n - 1);
            if (mid < right) {
                merge(arr, left, mid, right);
            }
        }
    }

    free(runStarts);
    free(threads);
    free(threadData);
    CloseHandle(semaphore);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        char usageMsg[256];
        snprintf(usageMsg, sizeof(usageMsg), "Usage: %s <max_threads> <comma_separated_array>\n", argv[0]);
        PrintStdout(usageMsg);
        return 1;
    }

    int maxThreads = atoi(argv[1]);
    if (maxThreads <= 0) {
        PrintStdout("Error: Number of threads must be positive.\n");
        return 1;
    }

    char* strArray = argv[2];
    int arraySize = 0;
    int* arr = (int*)malloc(MAX_ARRAY_SIZE * sizeof(int));
    if (arr == NULL) {
        PrintError("Memory allocation failed for array\n");
        return 1;
    }

    char* token = strtok(strArray, ",");
    while (token != NULL) {
        if (arraySize >= MAX_ARRAY_SIZE) {
            char errorMsg[100];
            snprintf(errorMsg, sizeof(errorMsg), "Error: Array size exceeds maximum allowed size (%d).\n", MAX_ARRAY_SIZE);
            PrintStdout(errorMsg);
            free(arr);
            return 1;
        }
        arr[arraySize++] = atoi(token);
        token = strtok(NULL, ",");
    }

    if (arraySize == 0) {
        PrintStdout("Array is empty. Nothing to sort.\n");
        free(arr);
        return 0;
    }

    char startMsg[100];
    snprintf(startMsg, sizeof(startMsg), "Starting TimSort with a maximum of %d threads.\n", maxThreads);
    PrintStdout(startMsg);

    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    timSort(arr, arraySize, maxThreads);

    QueryPerformanceCounter(&end);
    double interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;

    PrintStdout("Sorted array: ");
    for (int i = 0; i < arraySize; i++) {
        char numStr[12]; // Enough for an int and a comma
        snprintf(numStr, sizeof(numStr), "%d", arr[i]);
        PrintStdout(numStr);
        if (i < arraySize - 1) {
            PrintStdout(",");
        }
    }
    PrintStdout("\n");

    char timeMsg[50];
    snprintf(timeMsg, sizeof(timeMsg), "Time taken: %f seconds\n", interval);
    PrintStdout(timeMsg);

    free(arr);

    PrintStdout("\nTo demonstrate the number of threads used by this program:\n");
    PrintStdout("1. Open Task Manager (Ctrl+Shift+Esc).\n");
    PrintStdout("2. Go to the 'Details' tab.\n");
    PrintStdout("3. Right-click on the table header and select 'Select columns'.\n");
    PrintStdout("4. Check the 'Threads' box and click 'OK'.\n");
    PrintStdout("5. Find the process of this program (e.g., by PID or name) and observe the 'Threads' column.\n");
    PrintStdout("Alternatively, you can use Process Explorer from Sysinternals for more detailed process information.\n");

    return 0;
}
