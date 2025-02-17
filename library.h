#ifndef LIBRARY_H

#define LIBRARY_H



typedef struct {

    void* memory;

    size_t size;

    void* free_list;

} Allocator;



Allocator* allocator_create(void* const memory, const size_t size);

void allocator_destroy(Allocator* const allocator);

void* allocator_alloc_list(Allocator* const allocator, const size_t size);

void allocator_free_list(Allocator* const allocator, void* const memory);

void* allocator_alloc_power(Allocator* const allocator, const size_t size);

void allocator_free_power(Allocator* const allocator, void* const memory);



#endif  // LIBRARY_H