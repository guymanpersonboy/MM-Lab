#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>
static void update_free_list(memory_block_t *block);

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Christopher Carrasco cc66496" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;
// keeps count of the number of free blocks that should be in the free list
unsigned long num_free_blocks;
// used to help indicate if malloc was called on an allocated block
static uint64_t magic_number = 0xbfda36e4132bbaed;
// intially 4KB (4096 bytes)
static size_t heap_size = PAGESIZE;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1 && block->block_size_alloc1 & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
    block->block_size_alloc1 |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
    block->block_size_alloc1 &= ~0x1;
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
    block->block_size_alloc1 = size | alloc;
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
    memory_block_t *result = free_head->next;

    // first fit
    while (result != NULL) {
        // TODO debug
        size_t debug = get_size(result);
        printf("%zu\n", debug);

        if (get_size(result) >= size) {
            return result;
        }

        result = result->next;
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
    put_block(result, size, false);
    // doubled heap_size
    heap_size += size;
    
    return result;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    // allocate the free block (internal fragmentation!)
    allocate(block);

    // create new split free block
    memory_block_t *new_free_block = block + (get_size(block) - size);
    // put split free in memory
    put_block(new_free_block, (get_size(block) - size), false);
    // put split allocated block in memory
    put_block(block, size, true);
    block->block_size_alloc1 = magic_number;

    update_free_list(new_free_block);
    
    return block;
}

/**
 * insert the given block into the front of the free list if it is a free block.
 * (have free head point to block).
 * 
 * @param block the free block to insert into the head of the free list.
 */
static void update_free_list(memory_block_t *block) {
    assert(block != NULL && !is_allocated(block));
    memory_block_t *temp = free_head->next;
    free_head->next = block;
    block->next = temp;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 * 
 * @return a pointer to the new free block
 */
memory_block_t *coalesce(memory_block_t *block) {
    assert(block != NULL);
    uint64_t block_size = get_size(block);

    // the next block is a free block
    if (block->next != NULL && block + get_size(block) == block->next && !is_allocated(block->next)) {
        // coalesce
        uint64_t next_size = get_size(block->next);
        block->block_size_alloc = block_size + next_size;
        block->block_size_alloc1 = block_size + next_size;
        // set pointers
        block->next = block->next->next;
    }
    // the previous block is a free block
    if (block - ALIGNMENT != NULL && get_size(block - ALIGNMENT) ==
            get_size(block - get_size(block - ALIGNMENT)) && !is_allocated(block - ALIGNMENT)) {
        // coalesce
        memory_block_t *prev = block - get_size(block - ALIGNMENT);
        uint64_t prev_size = get_size(prev);

        prev->block_size_alloc = prev_size + block_size;
        prev->block_size_alloc1 = prev_size + block_size;
        // set pointers
        update_free_list(prev);

        return prev;
    }

    return block;
}



/*
 * uinit - Used to initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    memory_block_t free_root;
    free_head = &free_root;
    put_block(free_head, 0, false);
    free_head->next = extend(heap_size);
    put_block(free_head->next, heap_size, false);
    num_free_blocks = 1;
    if (get_size(free_head) != 0 || get_size(free_head->next) != PAGESIZE) {
        return -1;
    }

    return EXIT_SUCCESS;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    // find free block to put it
    memory_block_t *to_alloc = find(size);
    // split the free block into an allocated and free block and return allocated
    return split(to_alloc, size);
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    // ufree code here
    assert(ptr != NULL);
    memory_block_t *new_free_block = (memory_block_t *) ptr;
    uint64_t number = new_free_block->block_size_alloc1;

    if (number == magic_number) {
        deallocate(new_free_block);
        // TODO consider deferred coalescing (slide 32)
        coalesce(new_free_block);
    }
}