#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Christopher Carrasco cc66496" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;
// keeps count of the number of free blocks that should be in the free list
unsigned long long num_free_blocks;
// intially 4KB (4096 bytes)
static size_t heap_size = 4096;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & ~(ALIGNMENT-1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t *get_next(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) {
    memory_block_t *result = free_head;

    // first fit
    while (result->next != NULL) {
        if (get_size(result) >= size) {
            return result;
        }

        result = result->next;
    }
    // check final free block in the free list
    if (get_size(result) >= size) {
        return result;
    }
    // need more room! set last free block next to extend result
    result->next = extend(heap_size);

    return result->next;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    // creates new free block to represent new heap memory
    memory_block_t *result = (memory_block_t *) csbrk(size);
    assert(result != NULL);
    result->block_size_alloc = size;
    result->next = NULL;
    put_block(result, size, false);
    // doubled heap_size
    heap_size += size;
    
    return result;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    allocate(block);
    // allocate space for new blocks
    memory_block_t new_alloc;
    memory_block_t new_free;
    memory_block_t *new_allocated_block = &new_alloc;
    memory_block_t *new_free_block = &new_free;
    // put split blocks in memory
    put_block(new_allocated_block, size, true);
    put_block(new_free_block, get_size(block) - size, false);

    // udpate free list
    memory_block_t *temp = free_head->next;
    free_head->next = new_free_block;
    new_free_block->next = temp;
    
    return new_allocated_block;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {

    return NULL;
}



/*
 * uinit - Used to initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    free_head->next = extend(4096);
    num_free_blocks = 1;
    // more code here

    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    // umalloc code here

    return NULL;
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    // ufree code here

    // coalesce immediately
    // TODO if a contiguous free blocks in memory
    // ? if (block + get_size(block) == block->next) {
    //     coalesce(block);
    // }
}