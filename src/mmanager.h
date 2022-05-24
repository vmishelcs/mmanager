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
void *allocate(size_t size);

void *allocate_debug(size_t size, int *i);

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

// Merges allocated memory chunks together to maximize free space. Requires
// the caller to pass in two arrays of pointers, `before_addresses` and 
// `after_addresses`. These arrays will be written to so that `before_addresses` 
// contains addresses of allocated memory before compaction, and `after_addresses`
// contains addresses of allocated memory after compaction. Returns the number of
// valid entries in `before_addresses`/`after_addresses`.
size_t mmanager_compact(void **before_addresses, void **after_addresses);

// Returns the amount of available memory in bytes.
size_t mmanager_available_memory(void);

// Debugging.
void mmanager_print_free_list(void);
void mmanager_print_alloc_list(void);

#endif // MMANAGER_H_
