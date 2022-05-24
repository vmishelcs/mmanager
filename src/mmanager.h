#ifndef MMANAGER_H_
#define MMANAGER_H_

enum AllocationPolicy {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
};

// Initializes allocation mechanism.
void mmanager_initialize(size_t size, enum AllocationPolicy allocation_policy);

// Destroy allocator and frees all memory.
void mmanager_destroy(void);

// Returns a pointer to a memory block of size `block_size`. Returns NULL if
// a suitable block could not be found.
void *allocate(size_t block_size);

// Returns a pointer to a memory block for an array of `n` elements of `size`
// bytes. Returns NULL if the required memory could not be allocated.
// The returned memory is initialized to zero.
void *callocate(size_t n, size_t size);

// Changes the size of the memory block pointer to `ptr` to `new_size` bytes.
// Returns pointer to a (possibly) different memory block with `new_size` bytes.
// Contents of `ptr` will be unchanged up to the minimum of the old and new
// sizes. Returns NULL if unable to resize the memory block without changing
// contents `ptr`.
void *reallocate(void *ptr, size_t new_size);

// Frees the memory block pointed to by `ptr`. Results in undefined behavior
// if `ptr` does not point to allocated memory.
// Note: `ptr` must not be NULL.
void deallocate(void *ptr);


size_t mmanager_compact(void **before_addresses, void **after_addresses);

// Returns the amount of available memory in bytes.
size_t mmanager_available_memory(void);

// Debugging.
void mmanager_print_free_list(void);
void mmanager_print_alloc_list(void);

#endif // MMANAGER_H_
