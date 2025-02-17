#include <Windows.h>
#include <stdio.h>
#include "library.h"
#include <locale.h>

int main() {
    setlocale(LC_ALL, "Russian");
    // Загрузка динамической библиотеки
    HMODULE hModule = LoadLibraryA("library.dll");
    if (hModule == NULL) {
        printf("Using system allocator\n");
    } else {
        // Получение функций аллокатора
        Allocator* (*allocator_create_ptr)(void* const, const size_t);
        allocator_create_ptr = (Allocator* (*)(void* const, const size_t))GetProcAddress(hModule, "allocator_create");
        void (*allocator_destroy_ptr)(Allocator* const);
        allocator_destroy_ptr = (void (*)(Allocator* const))GetProcAddress(hModule, "allocator_destroy");
        void* (*allocator_alloc_ptr)(Allocator* const, const size_t);
        allocator_alloc_ptr = (void* (*)(Allocator* const, const size_t))GetProcAddress(hModule, "allocator_alloc_list");
        void (*allocator_free_ptr)(Allocator* const, void* const);
        allocator_free_ptr = (void (*)(Allocator* const, void* const))GetProcAddress(hModule, "allocator_free_list");

        // Создание аллокатора
        Allocator* allocator = allocator_create_ptr(VirtualAlloc(NULL, 1024, MEM_COMMIT, PAGE_READWRITE), 1024);
        printf("Аллокатор создан\n");

        // Выделение памяти
        void* memory = allocator_alloc_ptr(allocator, 128);
        printf("Память выделена: %p\n", memory);

        // Освобождение памяти
        allocator_free_ptr(allocator, memory);
        printf("Память освобождена\n");

        // Уничтожение аллокатора
        allocator_destroy_ptr(allocator);
        printf("Аллокатор уничтожен\n");

        FreeLibrary(hModule);
    }

    return 0;
}