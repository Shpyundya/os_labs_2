#ifndef LIBRARY_H //макросы для защиты от повторного включения, если файл уже включен, его код не компилируется повторно
//определяет интерфейс для работы с динамической памятью
#define LIBRARY_H

typedef struct { //структура аллокатора
    void* memory;
    size_t size;
    void* free_list;
} Allocator;

Allocator* allocator_create(void* const memory, const size_t size); //функция управления памятью, возвращает указатель на структуру аллокатора
void allocator_destroy(Allocator* const allocator); //освобождает всю выделенную память
void* allocator_alloc_list(Allocator* const allocator, const size_t size); //выделяет память из списка свободных блоков
void allocator_free_list(Allocator* const allocator, void* const memory); //освобождает блок памяти и возвращает его в список свободных блоков
void* allocator_alloc_power(Allocator* const allocator, const size_t size); //выделяет память 2 в степени н 
void allocator_free_power(Allocator* const allocator, void* const memory); //освобождает блок памяти выделенный с= помощью степеней двойки

#endif  // LIBRARY_H
