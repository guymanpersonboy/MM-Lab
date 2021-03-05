#include "umalloc.h"

//Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;
extern num_free_blocks;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 * Required to be completed for checkpoint 1.
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
    memory_block_t *prev = free_head;
    memory_block_t *cur = free_head->next;
    bool all_marked_free = true;
    long free_blocks_count = 0;



    // identify if bit0 of the current block is marked
    // as allocated, 1 if yes, 0 if no.
    while (cur != NULL) {
        char prev_mark = is_allocated(prev);
        char cur_mark = is_allocated(cur);
        // 1. check if every block in the free list is marked as free?
        if (prev_mark || cur_mark) {
            all_marked_free = false;
        } else  {
            // 2. Is every free block on the free list?
            free_blocks_count++;
        }
        // 6. Do any allocated blocks overlap with each other?
        // 7. Do any allocated blocks overlap with a free block?
        if (check_for_overlap(prev)) {
            return EXIT_FAILURE;
        }



        cur = cur->next;
    }
    // exexute 1. and 2. failure confirmation
    if (!all_marked_free || free_blocks_count != num_free blocks) {
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}

/**
 * 6. Check if any allocated blocks overlap with each other.
 * 7. Check if any allocated blocks overlap with a free block.
 * 
 * @param prev is the previous memory_block_t in the free list.
 * @param cur is the current memory_block_t in the free list.
 * @return true if allocated blocks overlaps with each other, false otherwise. 
 */
static bool check_for_overlap(memory_block_t *block) {

    // TODO memory_block_t is just free blocks so like?? uh...
    size_t size = get_size(block);

    return 0;
}
