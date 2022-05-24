#include <assert.h>
#include <stdlib.h>
#include <unity.h>
#include <unity_fixture.h>

#include "list.h"

#define N_1 64
#define N_2 128

static struct list_node *head = NULL;


// Comparison function for qsort.
static int compar(const void *a, const void *b) {
    unsigned int x = *((unsigned int *)a);
    unsigned int y = *((unsigned int *)b);
    if (x < y) {
        return -1;
    }
    if (x > y) {
        return 1;
    }
    return 0;
}


TEST_GROUP(list);

TEST_SETUP(list) {
    head = list_initialize();
}
TEST_TEAR_DOWN(list) {
    list_destroy(&head);
}

TEST_GROUP_RUNNER(list) {
    RUN_TEST_CASE(list, initialization);
    RUN_TEST_CASE(list, single_insert_search);
    RUN_TEST_CASE(list, search_non_exist);
    RUN_TEST_CASE(list, multiple_insert_search);
    RUN_TEST_CASE(list, single_remove_search);
    RUN_TEST_CASE(list, multiple_remove_search);
    RUN_TEST_CASE(list, sort);
}

static void RunAllTests(void) {
    RUN_TEST_GROUP(list);
}


// Tests.
TEST(list, initialization) {
    TEST_ASSERT_EQUAL_INT(0, list_size(head));
}

TEST(list, single_insert_search) {
    void *data = (void *)0xfeedbeef;

    // Add one node.
    struct list_node *node = list_create_node(data);
    list_add_node(&head, node);
    TEST_ASSERT_EQUAL_INT(1, list_size(head));

    // Find the node.
    struct list_node *target = list_search(head, data);
    TEST_ASSERT_NOT_NULL(target);
    TEST_ASSERT_EQUAL_PTR(data, target->address);
}

TEST(list, search_non_exist) {
    void *data = (void *)0xfeedbeef;
    struct list_node *target = list_search(head, data);
    TEST_ASSERT_NULL(target);
}

TEST(list, multiple_insert_search) {
    // Fill up an array with addresses.
    void *arr[N_2] = { 0 };
    void *start_addr = (void *)0xffff0000;
    for (int i = 0; i < N_2; ++i) {
        arr[i] = start_addr + i;
    }

    // Insert the contents of the array into the list.
    for (int i = 0; i < N_2; ++i) {
        struct list_node *node = list_create_node(arr[i]);
        list_add_node(&head, node);
    }
    TEST_ASSERT_EQUAL_INT(N_2, list_size(head));

    // Search for each element.
    for (int i = 0; i < N_2; ++i) {
        struct list_node *target = list_search(head, arr[i]);
        TEST_ASSERT_NOT_NULL(target);
    }
}

TEST(list, single_remove_search) {
    void *data = (void *)0xfeedbeef;

    // Insert a node.
    struct list_node *node = list_create_node(data);
    list_add_node(&head, node);
    TEST_ASSERT_EQUAL_INT(1, list_size(head));

    // Take out the node from the list.
    struct list_node *take_out_node = list_remove_node(&head, node);
    TEST_ASSERT_NOT_NULL(take_out_node);
    TEST_ASSERT_EQUAL_PTR(node, take_out_node);
    TEST_ASSERT_EQUAL_INT(0, list_size(head));
    struct list_node *target = list_search(head, data);
    TEST_ASSERT_NULL(target);

    free(take_out_node);
}

TEST(list, multiple_remove_search) {
    // Fill up an array with addresses.
    void *arr[N_2] = { 0 };
    void *start_addr = (void *)0x11110000;
    for (int i = 0; i < N_2; ++i) {
        arr[i] = start_addr + i;
    }

    // Insert the contents of the array into the list.
    for (int i = 0; i < N_2; ++i) {
        struct list_node *node = list_create_node(arr[i]);
        list_add_node(&head, node);
    }
    TEST_ASSERT_EQUAL_INT(N_2, list_size(head));

    // Remove every other element from the list.
    for (int i = 0; i < N_2; i += 2) {
        struct list_node *target = list_search(head, arr[i]);
        TEST_ASSERT_NOT_NULL(target);

        struct list_node *take_out_node = list_remove_node(&head, target);
        TEST_ASSERT_NOT_NULL(take_out_node);
        TEST_ASSERT_EQUAL_PTR(take_out_node, target);

        struct list_node *non_exist_node = list_search(head, arr[i]);
        TEST_ASSERT_NULL(non_exist_node);

        free(take_out_node);
    }
    TEST_ASSERT_EQUAL_INT(N_2 / 2, list_size(head));
}

TEST(list, sort) {
    // Create an array with random addresses.
    void *arr[N_1] = { 0 };
    void *start_addr = (void *)0x00000001;
    for (int i = 0; i < N_1; ++i) {
        arr[i] = (void *)(((char *)start_addr) + rand() % 256);
    }

    // Insert the contents of the array into the list.
    for (int i = 0; i < N_1; ++i) {
        struct list_node *node = list_create_node(arr[i]);
        list_add_node(&head, node);
    }
    TEST_ASSERT_EQUAL_INT(N_1, list_size(head));

    // Sort the array.
    qsort(arr, N_1, sizeof(*arr), compar);

    // Sort the list.
    list_sort(&head);
 
    // Obtain list as an array.
    void **list_arr = list_to_array(head);

    // Check that `arr` and `list_arr` have the same data.
    for (int i = 0; i < N_1; ++i) {
        TEST_ASSERT_EQUAL_PTR(arr[i], list_arr[i]);
    }

    free(list_arr);
}


int main(int argc, const char **argv) {
    srand(0);
    return UnityMain(argc, argv, RunAllTests);
}
