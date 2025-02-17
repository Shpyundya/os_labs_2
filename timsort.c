#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MIN_MERGE 32
#define MAX_ARRAY_SIZE 1000000

typedef struct {
    int* arr;
    int left;
    int right;
    HANDLE semaphore;
} ThreadData;

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

void merge(int arr[], int l, int m, int r) {
    int len1 = m - l + 1, len2 = r - m;
    int* left = (int*)malloc(len1 * sizeof(int));
    int* right = (int*)malloc(len2 * sizeof(int));
    
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

DWORD WINAPI threadFunc(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    insertionSort(data->arr, data->left, data->right);
    ReleaseSemaphore(data->semaphore, 1, NULL);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <max_threads> <array>\n", argv[0]);
        return 1;
    }

    int maxThreads = atoi(argv[1]);
    if (maxThreads <= 0 || maxThreads > 64) {
        printf("Invalid number of threads. Must be between 1 and 64.\n");
        return 1;
    }

    char* strArray = argv[2];
    int arraySize = 0;
    int* arr = (int*)malloc(MAX_ARRAY_SIZE * sizeof(int));

    char* token = strtok(strArray, ",");
    while (token != NULL) {
        if (arraySize >= MAX_ARRAY_SIZE) {
            printf("Array size exceeds maximum allowed size.\n");
            return 1;
        }
        arr[arraySize++] = atoi(token);
        token = strtok(NULL, ",");
    }

    // Корректировка количества потоков
    maxThreads = (maxThreads > arraySize) ? arraySize : maxThreads;

    HANDLE semaphore = CreateSemaphore(NULL, maxThreads, maxThreads, NULL);
    HANDLE* threads = (HANDLE*)malloc(maxThreads * sizeof(HANDLE));
    ThreadData* threadData = (ThreadData*)malloc(maxThreads * sizeof(ThreadData));

    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    // Разделение массива на части и создание потоков
    int chunkSize = arraySize / maxThreads;
    int extra = arraySize % maxThreads;

    for (int i = 0; i < maxThreads; i++) {
        WaitForSingleObject(semaphore, INFINITE);
        
        int start_pos = i * chunkSize + min(i, extra);
        int end_pos = (i + 1) * chunkSize + min(i + 1, extra) - 1;
        
        threadData[i].arr = arr;
        threadData[i].left = start_pos;
        threadData[i].right = end_pos;
        threadData[i].semaphore = semaphore;

        threads[i] = CreateThread(NULL, 0, threadFunc, &threadData[i], 0, NULL);
    }

    WaitForMultipleObjects(maxThreads, threads, TRUE, INFINITE);

    // Слияние отсортированных частей
    for (int size = 1; size < maxThreads; size = 2 * size) {
        for (int left = 0; left < maxThreads - size; left += 2 * size) {
            int mid = threadData[left + size - 1].right;
            int right = (left + 2 * size - 1 < maxThreads) ? 
                       threadData[left + 2 * size - 1].right : 
                       threadData[maxThreads - 1].right;
            merge(arr, threadData[left].left, mid, right);
        }
    }

    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
    printf("Execution time: %.3f seconds\n", elapsed);

    printf("Sorted array: ");
    for (int i = 0; i < arraySize; i++) {
        printf("%d", arr[i]);
        if (i < arraySize - 1) printf(",");
    }
    printf("\n");

    // Очистка
    for (int i = 0; i < maxThreads; i++)
        CloseHandle(threads[i]);
    CloseHandle(semaphore);
    free(threads);
    free(threadData);
    free(arr);

    return 0;
}