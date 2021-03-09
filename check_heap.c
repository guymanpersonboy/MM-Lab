#include "umalloc.h"

//Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;
extern unsigned long long num_free_blocks;

/*
 * check_heap - used to check that the heap is still in a consistent state.
 * Required to be completed for checkpoint 1.
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
    memory_block_t *prev = free_head->next;
    memory_block_t *cur = prev->next;
    bool all_marked_free = true;
    unsigned long long free_blocks_count = 1;

    while (cur != NULL) {
        char prev_mark = is_allocated(prev);
        char cur_mark = is_allocated(cur);
        // 1. check if every block in the free list is marked as free
        if (prev_mark || cur_mark) {
            all_marked_free = false;
        } else  {
            // 2. Is every free block on the free list?
            free_blocks_count++;
        }
        // check address order of prev and cur, and check
        // 9. are there any contiguous free blocks that somehow escaped coalescing.
        if (check_subsequent_blocks(prev, cur)) {
            return EXIT_FAILURE;
        }
        // 6. do any allocated blocks overlap with each other?
        // 7. do any allocated blocks overlap with a free block?
        if (check_for_overlap(prev)) {
            return EXIT_FAILURE;
        }

        prev = cur;
        cur = cur->next;
    }

    // exexute 1. and 2. failure confirmation
    if (!all_marked_free || free_blocks_count != num_free_blocks) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * Check if subsequent free_blocks in the free list are in
 * increasing order of their address in memory, and
 * they did not escape coalescing.
 * 
 * @param prev is the previous memory_block_t in the free list.
 * @param cur is the current memory_block_t in the free list.
 * @return true if blocks not in increasing order of memory or escaped coalescing.
 */
static bool check_subsequent_blocks(memory_block_t *prev, memory_block_t *cur) {
    size_t prev_size = get_size(prev);
    return prev >= cur || prev + prev_size == cur;
}

/**
 * Check if any allocated blocks overlap with each other. Check if any allocated
 * blocks overlap with a free block.
 * 
 * @param prev is the previous memory_block_t in the free list.
 * @param cur is the current memory_block_t in the free list.
 * @return true if allocated blocks overlaps with each other, false otherwise. 
 */
static bool check_for_overlap(memory_block_t *block) {
    // TODO not sure how to acces allocated blocks yet
    // TODO check if block overlaps with a allocated block
    
    return 0;
}
