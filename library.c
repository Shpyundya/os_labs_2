#include <Windows.h>
#include "library.h"

Allocator* allocator_create(void* const memory, const size_t size) {
    Allocator* allocator = (Allocator*)memory;
    allocator->memory = memory;
    allocator->size = size;
    allocator->free_list = memory;
    return allocator;
}

void allocator_destroy(Allocator* const allocator) {
    VirtualFree(allocator->memory, 0, MEM_RELEASE);
}

void* allocator_alloc_list(Allocator* const allocator, const size_t size) {
    // Поиск свободного блока
    void* current = allocator->free_list;
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
        return NULL;
    }
    return new_memory;
}

void allocator_free_list(Allocator* const allocator, void* const memory) {
    // Добавление блока в список свободных блоков
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

void* allocator_alloc_power(Allocator* const allocator, const size_t size) {
    // Вычисление наименьшего блока размером 2^n
    size_t power = 1;
    while (power < size) {
        power *= 2;
    }

    // Поиск выделенного блока
    void* current = allocator->memory;
    while (current != NULL) {
        size_t current_size = *(size_t*)current;
        if (current_size == power) {
            return (void*)((size_t)current + sizeof(size_t));
        }
        current = *(void**)((size_t)current + sizeof(size_t));
    }

    // Если блока нет, запрашиваем новую память у ядра
    void* new_memory = VirtualAlloc(NULL, power, MEM_COMMIT, PAGE_READWRITE);
    if (new_memory == NULL) {
        return NULL;
    }
    return new_memory;
}

void allocator_free_power(Allocator* const allocator, void* const memory) {
    // Освобождение блока
    VirtualFree(memory, 0, MEM_RELEASE);
}