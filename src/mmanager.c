#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mmanager.h"


#define HEADER_SIZE 16


typedef struct header {
    size_t block_size; 
    struct header *next;
    char block_memory[0]; // Must be the last field of this struct.
} header_t;


struct mmanager {
    enum AllocationPolicy allocation_policy;
    size_t size;
    void *memory;
    header_t *free_list;
    header_t *alloc_list;
};


struct mmanager memory_manager = { -1, 0, NULL, NULL, NULL };
static pthread_mutex_t lock;


// Returns the header to a free block of memory using the first-fit search policy.
// Returns NULL if no suitable free block could be found.
static header_t *first_fit_block_search(size_t block_size);

// Returns the header to a free block of memory using the best-fit search policy.
// Returns NULL if no suitable free block could be found.
static header_t *best_fit_block_search(size_t block_size);

// Returns the header to a free block of memory using the worst-fit search policy.
// Returns NULL if no suitable free block could be found.
static header_t *worst_fit_block_search(size_t block_size);

// Adds the block specified by `header_address` to the allocator's free list.
static void add_to_free_list(header_t *header_address);

// Removes the block specified by `header_address` from the allocator's free list.
// Assumes `header_address` is in the free list.
static void remove_from_free_list(header_t *header_address);

// Adds the block specified by `header_address` to the allocator's alloc list.
static void add_to_alloc_list(header_t *header_address);

// Removes the block specified by `header_address` from the allocator's alloc list.
// Assumes `header_address` is in the alloc list.
static void remove_from_alloc_list(header_t *header_address);

// Eliminates contiguous free blocks in the free list.
static void coalesce_free_blocks(void);


void allocator_initialize(size_t size, enum AllocationPolicy allocation_policy) {
    // Obtain 'size' bytes for the allocator and set allocation algorithm.
    memory_manager.memory = sbrk((intptr_t)size);
    if (!memory_manager.memory) {
        fprintf(stderr, "ERROR: failed to obtain %lu memory for the allocator.\n", size);
        raise(SIGABRT);
    }
    memory_manager.size = size;
    memory_manager.allocation_policy = allocation_policy;

    // Set initial free block properties.
    memory_manager.free_list = (header_t *)memory_manager.memory;
    memory_manager.free_list->block_size = memory_manager.size - HEADER_SIZE;
    memory_manager.free_list->next = NULL;

    // Set alloc list to empty.
    memory_manager.alloc_list = NULL;

    // Set user memory to 0.
    void *user_memory = (char *)memory_manager.memory + HEADER_SIZE;
    memset(user_memory, 0, memory_manager.size - HEADER_SIZE);

    // Initialize mutex.
    pthread_mutex_init(&lock, NULL);
}

void allocator_destroy(void) {
    sbrk(-((intptr_t)memory_manager.size));
    pthread_mutex_destroy(&lock);
}

void *allocate(size_t size) {
    assert(size > 0);
    void* ptr = NULL;
    header_t *free_block_header = NULL;

    pthread_mutex_lock(&lock);
    {
        switch (memory_manager.allocation_policy) {
            case FIRST_FIT:
                free_block_header = first_fit_block_search(size);
                break;

            case BEST_FIT:
                free_block_header = best_fit_block_search(size);
                break;
            
            case WORST_FIT:
                free_block_header = worst_fit_block_search(size);
                break;

            default:
                fprintf(stderr, "ERROR: no allocation algorithm specified.\n");
                raise(SIGABRT);
        }

        if (free_block_header) {
            // Rename `free_block_header` to `allocated_block_header` for clarity.
            header_t *allocated_block_header = free_block_header;
            // Remove it from free list.
            remove_from_free_list(allocated_block_header);

            // Check if there is more memory in this block for future allocation.
            if (allocated_block_header->block_size - size > HEADER_SIZE) {
                // Create a new free block.
                header_t *new_free_block_header = (header_t *)(allocated_block_header->block_memory + size);
                new_free_block_header->block_size = allocated_block_header->block_size - (HEADER_SIZE + size);

                // Add `new_free_block_header` to free list.
                add_to_free_list(new_free_block_header);

                // Set the size of the newly allocated block.
                allocated_block_header->block_size = size;
            }

            // Add `allocated_block_header` to alloc list.
            add_to_alloc_list(allocated_block_header);
            
            ptr = (void *)allocated_block_header->block_memory;
        }
    }
    pthread_mutex_unlock(&lock);

    return ptr;
}

void *callocate(size_t n, size_t size) {
    void *memory = allocate(n * size);
    memset(memory, 0, n * size);
    return memory;
}

void *reallocate(void *ptr, size_t new_size) {
    // TODO: Implement.

    return NULL;
}

void deallocate(void *ptr) {
    assert(ptr != NULL);

    pthread_mutex_lock(&lock);
    {
        header_t *dealloc_block_header = (header_t *)((char *)ptr - HEADER_SIZE);

        // Remove the block from alloc list.
        remove_from_alloc_list(dealloc_block_header);

        // Add the block back to free list.
        add_to_free_list(dealloc_block_header);

        // Merge any contiguous free blocks.
        coalesce_free_blocks();
    }
    pthread_mutex_unlock(&lock);
}

size_t allocator_compact(void **before_addresses, void **after_addresses) {
    int index = 0;

    pthread_mutex_lock(&lock);
    {
        // Check that there is memory allocated and that there is free memory.
        if (memory_manager.alloc_list && memory_manager.free_list) {
            header_t *current_alloc_block = memory_manager.alloc_list;

            // To determine if we must move `current_alloc_block`, look at the
            // address of the first free block. If it is less than the address of
            // `current_alloc_block`, compaction needs to occur. Otherwise, look
            // at the next allocated block.
            // Note: We only have to interact with the head of the free list.
            while (current_alloc_block) {
                header_t *first_free_block = memory_manager.free_list;

                if (first_free_block < current_alloc_block) {
                    // Remove the allocated block and the free block from their lists.
                    remove_from_alloc_list(current_alloc_block);
                    remove_from_free_list(first_free_block);

                    // Set before-compaction address.
                    before_addresses[index] = current_alloc_block->block_memory;
                    
                    // Remember the size of `first_free_block` to set it later.
                    size_t free_block_size = first_free_block->block_size;

                    // Copy header info from `current_alloc_block` to `first_free_block`.
                    memcpy((void *)first_free_block, (void *)current_alloc_block, HEADER_SIZE);

                    // Move `current_alloc_block` data.
                    memcpy((void *)first_free_block->block_memory,
                        (void *)current_alloc_block->block_memory,
                        current_alloc_block->block_size);
                    
                    // At this point, `current_alloc_block` has been entirely moved.
                    // Redefine for clarity.
                    current_alloc_block = first_free_block;
                    first_free_block = (header_t *)(current_alloc_block->block_memory + current_alloc_block->block_size);

                    // Set free block size, since at the moment it stores garbage data.
                    first_free_block->block_size = free_block_size;

                    // Add the compacted blocks back to their lists.
                    add_to_alloc_list(current_alloc_block);
                    add_to_free_list(first_free_block);

                    // Eliminate any contiguous free blocks.
                    coalesce_free_blocks();

                    // Set after-compaction address.
                    after_addresses[index] = current_alloc_block->block_memory;
                    ++index;
                }

                // Look at next allocated block.
                current_alloc_block = current_alloc_block->next;
            }
        }
    }
    pthread_mutex_unlock(&lock);

    // Return size of the argument arrays.
    return index;
}

size_t allocator_available_memory(void) {
    size_t size = 0;
    
    pthread_mutex_lock(&lock);
    {
        header_t *current_block = (header_t *)memory_manager.free_list;
        while (current_block) {
            size += current_block->block_size;
            current_block = current_block->next;
        }
    }
    pthread_mutex_unlock(&lock);

    return size;
}


/* * * * * * * * * * * * * * * * * * *
 * Memory allocation policies.
 * * * * * * * * * * * * * * * * * * */

static header_t *first_fit_block_search(size_t block_size) {
    header_t *current_block = (header_t *)memory_manager.free_list;
    while (current_block) {
        if (current_block->block_size >= block_size) {
            return current_block;
        }
        current_block = current_block->next;
    }
    return NULL;
}

static header_t *best_fit_block_search(size_t block_size) {
    // To find the best fitting block, we take the difference between the size
    // of `current_block` and `block_size`. Let this difference be `delta`. We
    // then iterate through the free list to find the minimal delta.

    header_t *best_fit_block = NULL;
    header_t *current_block = (header_t *)memory_manager.free_list;
    size_t min_delta = SIZE_MAX;

    while (current_block) {
        // Check if `current_block` can fit `block_size`.
        if (current_block->block_size >= block_size) {
            size_t delta = current_block->block_size - block_size;
            if (delta < min_delta) {
                min_delta = delta;
                best_fit_block = current_block;
            }
        }
        current_block = current_block->next;
    }

    return best_fit_block;
}

static header_t *worst_fit_block_search(size_t block_size) {

    // To find the worst fitting block, we take the difference between the size
    // of `current_block` and `block_size`. Let this difference be `delta`. We
    // then iterate through the free list to find the maximal delta.

    header_t *worst_fit_block = NULL;
    header_t *current_block = (header_t *)memory_manager.free_list;
    size_t max_delta = 0;

    while (current_block) {
        // Check if `current_block` can fit `block_size`.
        if (current_block->block_size >= block_size) {
            size_t delta = current_block->block_size - block_size;
            if (delta > max_delta) {
                max_delta = delta;
                worst_fit_block = current_block;
            }
        }
        current_block = current_block->next;
    }

    return worst_fit_block;
}


/* * * * * * * * * * * * * * * * * * *
 * List helpers.
 * * * * * * * * * * * * * * * * * * */

static void add_to_free_list(header_t *header_address) {
    // If the free list is empty, set `header_address` as free list head.
    if (!memory_manager.free_list) {
        memory_manager.free_list = header_address;
    }
    // If the free list head has a bigger address than `header_address`, set
    // new free list head.
    else if (header_address < memory_manager.free_list) {
        header_address->next = memory_manager.free_list;
        memory_manager.free_list = header_address;
    }
    // Otherwise, find the first block whose address is greater than `header_address`
    // and add the new block before it.
    else {
        header_t *current_block = memory_manager.free_list;
        while (current_block) {
            if (current_block->next && current_block->next > header_address) {
                header_address->next = current_block->next;
                current_block->next = header_address;
                return;
            }
            current_block = current_block->next;
        }
    }
}

static void remove_from_free_list(header_t *header_address) {
    // If `header_address` is the free list head, set new free list head.
    if (header_address == memory_manager.free_list) {
        memory_manager.free_list = header_address->next;
    }
    // Otherwise, look for it in the free list and change its predecessor's `next`
    // pointer.
    else {
        header_t *current_block = memory_manager.free_list;
        while (current_block->next != header_address) {
            current_block = current_block->next;
        }
        // Here, current_block->next == header_address.
        current_block->next = header_address->next;
    }
}

static void add_to_alloc_list(header_t *header_address) {
    // If the alloc list is empty, set `header_address` as alloc list head.
    if (!memory_manager.alloc_list) {
        memory_manager.alloc_list = header_address;
    }
    // If the alloc list head has a bigger address than `header_address`, set
    // new alloc list head.
    else if (header_address < memory_manager.alloc_list) {
        header_address->next = memory_manager.alloc_list;
        memory_manager.alloc_list = header_address;
    }
    // Otherwise, find the first block whose address is greater than `header_address`
    // and add the new block before it.
    else {
        header_t *current_block = memory_manager.alloc_list;
        while (current_block) {
            if (current_block->next && current_block->next > header_address) {
                header_address->next = current_block->next;
                current_block->next = header_address;
                return;
            }
            // If `current_block->next` is NULL, then we must be at the end of the list.
            else if (!current_block->next) {
                // All of the blocks in the alloc list have addresses less than
                // `header_address`, so add the new block to the end of the list.
                current_block->next = header_address;
                header_address->next = NULL;
            }
            current_block = current_block->next;
        }
    }
}

static void remove_from_alloc_list(header_t *header_address) {
    // If `header_address` is the alloc list head, set new alloc list head.
    if (header_address == memory_manager.alloc_list) {
        memory_manager.alloc_list = header_address->next;
    }
    // Otherwise, look for it in the alloc list and change its predecessor's `next`
    // pointer.
    else {
        header_t *current_block = memory_manager.alloc_list;
        // Make sure we don't dereference a NULL pointer.
        while (current_block && current_block->next != header_address) {
            current_block = current_block->next;
        }
        if (current_block) {
            // Here, current_block->next == header_address.
            current_block->next = header_address->next;
        }
    }
}

static void coalesce_free_blocks(void) {
    header_t *current_block_header = memory_manager.free_list;
    header_t *next_block_header = current_block_header->next;
    while (next_block_header) {
        // If we found contiguous free blocks, merge them together.
        if (current_block_header->block_memory + current_block_header->block_size == (char *)next_block_header) {
            // Add total size of the next block to the current block.
            current_block_header->block_size += (HEADER_SIZE + next_block_header->block_size);
            remove_from_free_list(next_block_header);

            next_block_header = next_block_header->next;
        }
        // Otherwise, shift to the next pair of blocks.
        else {
            current_block_header = current_block_header->next;
            next_block_header = current_block_header->next;
        }
    }
}

void allocator_print_free_list(void) {
    printf("Free list:\n");
    pthread_mutex_lock(&lock);
    {
        header_t *current_block = memory_manager.free_list;
        while (current_block) {
            printf("\t(%p, %lu, %p)\n", current_block, current_block->block_size, current_block->next);
            current_block = current_block->next;
        }
    }
    pthread_mutex_unlock(&lock);
}

void allocator_print_alloc_list(void) {
    printf("Alloc list:\n");
    pthread_mutex_lock(&lock);
    {
        header_t *current_block = memory_manager.alloc_list;
        while (current_block) {
            printf("\t(%p, %lu, %p)\n", current_block, current_block->block_size, current_block->next);
            current_block = current_block->next;
        }
    }
    pthread_mutex_unlock(&lock);
}
