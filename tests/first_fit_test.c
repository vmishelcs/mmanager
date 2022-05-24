#include <stdio.h>
#include <unity.h>
#include <unity_fixture.h>

#include "mmanager.h"


#define HEADER_SIZE 16
#define MMRY_ALLOC_SIZE 2048
#define ONE 1
#define N1 4
#define N2 8
#define N3 32
#define N4 32

static int linear_search(void **arr, size_t n, void *target);


// Test group properties.
TEST_GROUP(mmry_alloc_first_fit);
TEST_SETUP(mmry_alloc_first_fit) {
    mmanager_initialize(MMRY_ALLOC_SIZE, FIRST_FIT);
}
TEST_TEAR_DOWN(mmry_alloc_first_fit) {
    mmanager_destroy();
}
TEST_GROUP_RUNNER(mmry_alloc_first_fit) {
    RUN_TEST_CASE(mmry_alloc_first_fit, Initialization);
    RUN_TEST_CASE(mmry_alloc_first_fit, SingleAllocation);
    RUN_TEST_CASE(mmry_alloc_first_fit, MultipleAllocations);
    RUN_TEST_CASE(mmry_alloc_first_fit, AllocAllMemory);
    RUN_TEST_CASE(mmry_alloc_first_fit, AllocAllMemoryAgain);
    RUN_TEST_CASE(mmry_alloc_first_fit, AllocTooMuch);
    RUN_TEST_CASE(mmry_alloc_first_fit, SingleAllocDealloc);
    RUN_TEST_CASE(mmry_alloc_first_fit, MultipleAllocDealloc);
    RUN_TEST_CASE(mmry_alloc_first_fit, MemoryCorruption);
    RUN_TEST_CASE(mmry_alloc_first_fit, SimpleCompaction);
    RUN_TEST_CASE(mmry_alloc_first_fit, AllocDeallocCompaction);
}
static void RunAllTests(void) {
    RUN_TEST_GROUP(mmry_alloc_first_fit);
}

// Tests.
TEST(mmry_alloc_first_fit, Initialization) {
    size_t available_memory = mmanager_available_memory();
    TEST_ASSERT_EQUAL_size_t(MMRY_ALLOC_SIZE - HEADER_SIZE, available_memory);
}
TEST(mmry_alloc_first_fit, SingleAllocation) {
    size_t bytes_to_alloc = 8;
    void *ptr = allocate(bytes_to_alloc);
    TEST_ASSERT_NOT_NULL(ptr);

    size_t available_memory = mmanager_available_memory();
    TEST_ASSERT_EQUAL_size_t(MMRY_ALLOC_SIZE - (bytes_to_alloc + 2 * HEADER_SIZE), available_memory);
}
TEST(mmry_alloc_first_fit, MultipleAllocations) {
    void **ptrs[N1];
    size_t mem_used = HEADER_SIZE;
    size_t bytes_to_alloc = 4;
    for (int i = 0; i < N1; ++i) {
        bytes_to_alloc *= 2;
        ptrs[i] = allocate(bytes_to_alloc);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
        mem_used += (HEADER_SIZE + bytes_to_alloc);
    }

    size_t available_memory = mmanager_available_memory();
    TEST_ASSERT_EQUAL_size_t(MMRY_ALLOC_SIZE - mem_used, available_memory);
}
TEST(mmry_alloc_first_fit, AllocAllMemory) {
    void *ptr = allocate(mmanager_available_memory());
    TEST_ASSERT_NOT_NULL(ptr);
    size_t available_memory = mmanager_available_memory();
    TEST_ASSERT_EQUAL_size_t(0, available_memory);
    
    void *ptr2 = allocate(1);
    TEST_ASSERT_NULL(ptr2);
}
TEST(mmry_alloc_first_fit, AllocAllMemoryAgain) {
    size_t bytes_to_alloc = 16;
    while (mmanager_available_memory() > bytes_to_alloc) {
        void *ptr = allocate(bytes_to_alloc);
        ptr += 0;
    }

    if (mmanager_available_memory() != 0) {
        void *ptr = allocate(mmanager_available_memory());
        ptr += 0;
    }

    TEST_ASSERT_EQUAL_size_t(0, mmanager_available_memory());
    void *ptr = allocate(1);
    TEST_ASSERT_NULL(ptr);
}
TEST(mmry_alloc_first_fit, AllocTooMuch) {
    size_t available_memory = mmanager_available_memory();
    void *ptr = allocate(available_memory + 1);
    TEST_ASSERT_NULL(ptr);
    TEST_ASSERT_EQUAL_size_t(mmanager_available_memory(), available_memory);
}
TEST(mmry_alloc_first_fit, SingleAllocDealloc) {
    size_t mem_size = mmanager_available_memory();

    size_t n = 8;
    void *ptr = allocate(n);
    TEST_ASSERT_NOT_NULL(ptr);

    deallocate(ptr);
    TEST_ASSERT_EQUAL_size_t(mem_size, mmanager_available_memory());
}
TEST(mmry_alloc_first_fit, MultipleAllocDealloc) {
    void **ptrs[N2];
    size_t bytes_to_alloc = 8;
    size_t bytes_available = mmanager_available_memory();
    for (int i = 0; i < N2; ++i) {
        ptrs[i] = allocate(bytes_to_alloc);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
        bytes_available -= (bytes_to_alloc + HEADER_SIZE);
    }
    TEST_ASSERT_EQUAL_size_t(bytes_available, mmanager_available_memory());

    // Deallocate every other ptr.
    for (int i = 0; i < N2; i += 2) {
        deallocate(ptrs[i]);
        bytes_available += bytes_to_alloc;
    }
    TEST_ASSERT_EQUAL_size_t(bytes_available, mmanager_available_memory());
    for (int i = 1; i < N2; i += 2) {
        deallocate(ptrs[i]);
        bytes_available += (bytes_to_alloc + 2 * HEADER_SIZE);
    }
    TEST_ASSERT_EQUAL_size_t(bytes_available, mmanager_available_memory());
}
TEST(mmry_alloc_first_fit, MemoryCorruption) {
    double control_arr[N1] = {
        1.0f,
        2.0f,
        4.0f,
        8.0f
    };

    size_t bytes_available = mmanager_available_memory();

    double **arr = allocate(N2 * sizeof(*arr));
    bytes_available -= ((N2 * sizeof(*arr)) + HEADER_SIZE);

    for (int i = 0; i < N2; ++i) {
        arr[i] = allocate(N1 * sizeof(**arr));
        bytes_available -= ((N1 * sizeof(**arr)) + HEADER_SIZE);

        for (int j = 0; j < N1; ++j) {
            arr[i][j] = control_arr[j] * (i + 1);
        }
    }
    TEST_ASSERT_EQUAL_size_t(bytes_available, mmanager_available_memory());

    for (int i = 0; i < N2; i += 2) {
        deallocate(arr[i]);
        arr[i] = allocate(N1 * sizeof(**arr));
        for (int j = 0; j < N1; ++j) {
            arr[i][j] = 0.0f;
        }
    }

    for (int i = 1; i < N2; i += 2) {
        for (int j = 0; j < N1; ++j) {
            TEST_ASSERT_EQUAL_DOUBLE(control_arr[j] * (i + 1), arr[i][j]);
        }
    }
}
TEST(mmry_alloc_first_fit, SimpleCompaction) {
    double control_val = 2.0f;
    double *ptr1 = allocate(sizeof(*ptr1));
    *ptr1 = control_val * 2;
    double *ptr2 = allocate(sizeof(*ptr2));
    *ptr2 = control_val;
    deallocate(ptr1);

    void *before[ONE];
    void *after[ONE];
    size_t n = mmanager_compact(before, after);
    TEST_ASSERT_EQUAL_size_t(ONE, n);
    TEST_ASSERT_EQUAL_PTR(ptr2, before[0]);
    ptr2 = after[0];
    TEST_ASSERT_EQUAL_DOUBLE(control_val, *ptr2);
}
TEST(mmry_alloc_first_fit, AllocDeallocCompaction) {
    // Create a ground truth array.
    int control_arr[N4] = { 0 };
    for (int i = 0; i < N4; ++i) {
        control_arr[i] = i * i;
    }

    // Array that we will run our test on.
    int *arr[N4];
    for (int i = 0; i < N4; ++i) {
        arr[i] = allocate(sizeof(*arr[i]));
        *arr[i] = control_arr[i];
    }

    // Deallocate random entries of `arr`.
    for (int i = 0; i < N4; ++i) {
        if (i % 5 == 0 || i % 7 == 0|| i % 11  == 0|| i % 13 == 0) {
            deallocate(arr[i]);
            arr[i] = NULL;
        }
    }

    // Perform compaction.
    int *before[N4];
    int *after[N4];
    size_t n = mmanager_compact((void *)before, (void *)after);

    // For every value in `arr`...
    for (int i = 0; i < N4; ++i) {
        // If it hasn't been deallocated...
        if (arr[i]) {
            // Get its location in the `after` array.
            int idx = linear_search((void **)before, n, (void *)arr[i]);
            TEST_ASSERT_NOT_EQUAL_INT(-1, idx);
            // Adjust its after-compaction location.
            arr[i] = after[idx];
        }
    }

    // Compare with ground truth array.
    for (int i = 0; i < N4; ++i) {
        if (arr[i]) {
            TEST_ASSERT_EQUAL_INT(control_arr[i], *arr[i]);
        }
    }
}

static int linear_search(void **arr, size_t n, void *target) {
    for (int i = 0; i < n; ++i) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}

int main(int argc, const char **argv) {
    return UnityMain(argc, argv, RunAllTests);
}
