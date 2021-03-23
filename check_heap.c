#include "umalloc.h"
static bool check_subsequent_blocks(memory_block_t *prev, memory_block_t *cur);
static void print_list(const char location[]);

// Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;
extern unsigned long num_free_blocks;

/*
 * check_heap - used to check that the heap is still in a consistent state.
 * Required to be completed for checkpoint 1.
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
    memory_block_t *prev = free_head;
    // Check for NULL free list
    assert(free_head != NULL);
    memory_block_t *cur = prev->next;
    bool all_marked_free = true;
    unsigned long free_blocks_count = 1;

    assert(get_size(prev) % ALIGNMENT == 0);
    while (cur != NULL) {
        // Check for infinite loop
        assert(cur != cur->next);
        // Check alignment of free list
        assert(get_size(cur) % ALIGNMENT == 0);
        // Check if every block in the free list is marked as unallocated
        if (is_allocated(prev) || is_allocated(cur)) {
            all_marked_free = false;
        } else {
            // Is every free block in the free list
            free_blocks_count++;
        }
        // Are there any contiguous free blocks that escaped coalescing
        // Is the free list in increasing address order
        if (check_subsequent_blocks(prev, cur)) {
            return EXIT_FAILURE;
        }

        prev = cur;
        cur = cur->next;
    }

    // confirm checks
    if (!all_marked_free || free_blocks_count != num_free_blocks) {
        puts("!all_marked_free || free_blocks_count != num_free_blocks");
        printf("%d, %d\n", !all_marked_free, free_blocks_count != num_free_blocks);
        printf("exp: %ld == act: %ld\n", free_blocks_count, num_free_blocks);
        print_list("check_heap");

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * Check if subsequent free_blocks did not escape coalescing.
 * Check if free blocks are in memory order.
 * 
 * @param prev is the previous memory_block_t in the free list.
 * @param cur is the current memory_block_t in the free list.
 * @return true if blocks not in increasing order of memory or escaped coalescing.
 */
static bool check_subsequent_blocks(memory_block_t *prev, memory_block_t *cur) {
    size_t prev_size = get_size(prev) + ALIGNMENT;
    if ((memory_block_t *)((char *)prev + prev_size) == cur || prev >= cur) {
        puts("check_subsequent_blocks(prev, cur)");
        printf("%d, %d ", (memory_block_t *)((char *)prev + prev_size) == cur, prev >= cur);
        printf("%p >= %p\n", prev, cur);
        print_list("check_heap");

        return true;
    }
    return false;
}

/*
 * print the address and size of each free block in the free list.
 * for debugging purposes.
 */
static void print_list(const char location[]) {
    memory_block_t *cur = free_head;
    printf("DEBUG: %s\n", location);
    printf("DEBUG: ");
    while (cur != NULL) {
        printf("%p: %zu, ", cur, get_size(cur));
        assert(cur != cur->next);
        cur = cur->next;
    }
}
