#include <Windows.h>
#include "library.h"

// Функция для вывода ошибки в консоль
void consoleErrorOutput(const char* message) {
    DWORD bytesWritten;
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        WriteFile(hConsole, message, (DWORD)strlen(message), &bytesWritten, NULL);
    }
}

Allocator* allocator_create(void* const memory, const size_t size) { //создание структуры аллокатора из лайбрари аш
    Allocator* allocator = (Allocator*)memory; //мемори - начало выделенного блока памяти, сайз - общий размер памяти
    allocator->memory = memory; //фри лист - указатель на список свободных блоков
    allocator->size = size;
    allocator->free_list = memory;
    return allocator;
}

void allocator_destroy(Allocator* const allocator) { //уничтожение аллокатора, полное освобождение памяти
    if (!VirtualFree(allocator->memory, 0, MEM_RELEASE)) {
        consoleErrorOutput("VirtualFree failed\n");
    }
}

void* allocator_alloc_list(Allocator* const allocator, const size_t size) { //метод выписки свободных блоков
    // Поиск свободного блока, проход по списку свободных блоков, если найден подходящий блок: если он больше чем запрашиваемый размер:
    void* current = allocator->free_list;//создается новый свободный блок, если блок точно совпадают то выделяем его
    while (current != NULL) { 
        size_t current_size = *(size_t*)current;
        if (current_size >= size) {
            // Разделение блока
            if (current_size > size) {
                void* new_block = (void*)((size_t)current + size);
                *(size_t*)new_block = current_size - size;
                allocator->free_list = new_block;
            } else {
                allocator->free_list = *(void**)((size_t)current + sizeof(size_t));
            }
            return (void*)((size_t)current + sizeof(size_t));
        }
        current = *(void**)((size_t)current + sizeof(size_t));
    }

    // Если свободных блоков нет, запрашиваем новую память у ядра
    void* new_memory = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    if (new_memory == NULL) {
        consoleErrorOutput("VirtualAlloc failed\n");
        return NULL;
    }
    return new_memory;
}

void allocator_free_list(Allocator* const allocator, void* const memory) { //добавляем освобожденный блок обратно в список свободных
    // Добавление блока в список свободных блоков, сортируем блоки по размеру
    size_t size = *(size_t*)memory;
    void* current = allocator->free_list;
    if (current == NULL || size < *(size_t*)current) {
        *(void**)((size_t)memory + sizeof(size_t)) = current;
        allocator->free_list = memory;
    } else {
        while (current != NULL && *(size_t*)current < size) {
            current = *(void**)((size_t)current + sizeof(size_t));
        }
        if (current != NULL) {
            *(void**)((size_t)memory + sizeof(size_t)) = current;
            current = memory;
        }
    }
}

void* allocator_alloc_power(Allocator* const allocator, const size_t size) { //метод степеней двойки, освобождение памяти
    // Вычисление наименьшего блока размером 2^n
    size_t power = 1; // определяем ближайшую степень двойки достаточную для хранения данных, если такой нет, запрашиваем память у виндоус
    while (power < size) {
        power *= 2;
    }

    // Поиск выделенного блока
    void* current = allocator->memory;
    while (current != NULL) {
        size_t current_size = *(size_t*)current;
        if (current_size == power) {
            return (void*)((size_t)current + sizeof(size_t)); //возвращаем указатель на данные о размере блока
        }
        current = *(void**)((size_t)current + sizeof(size_t)); //читаем адрес слудеющего свободного блока
    }

    // Если блока нет, запрашиваем новую память у ядра
    void* new_memory = VirtualAlloc(NULL, power, MEM_COMMIT, PAGE_READWRITE); //MEM_COMMIT выделит физическую память
    if (new_memory == NULL) {
        consoleErrorOutput("VirtualAlloc failed\n");
        return NULL;
    }
    return new_memory;
}

void allocator_free_power(Allocator* const allocator, void* const memory) {
    // Освобождение блока
    if (!VirtualFree(memory, 0, MEM_RELEASE)) {
        consoleErrorOutput("VirtualFree failed\n");
    }
}
