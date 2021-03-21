#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>
static void update_free_list(memory_block_t *old_block,
        memory_block_t *new_free_block);

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Christopher Carrasco cc66496" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;
// keeps count of the number of free blocks that should be in the free list
unsigned long num_free_blocks;
// uinit will set to 4096 bytes
static size_t heap_size = 0;

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
    return (void *)(block + 1);
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
    assert(free_head != NULL);

    // first fit
    while (result != NULL) {
        if (get_size(result) >= size + ALIGNMENT) {
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
    // no need to split
    if (get_size(block) - size <= ALIGNMENT * 2)  {
        return block;
    }

    // put split allocated block in memory
    if (size % ALIGNMENT != 0) {
        size += ALIGNMENT - (size % ALIGNMENT);
    }
    size_t free_size = get_size(block) - size;
    put_block(block, size, true);
    // create new split free block by setting pointer to block address + size
    memory_block_t *new_free_block = (memory_block_t *) ((char *) block + size);
    if (free_size % ALIGNMENT != 0) {
        free_size -= free_size % ALIGNMENT;
    }
    put_block(new_free_block, free_size, false);
    update_free_list(block, new_free_block);
    
    return block;
}

/**
 * insert the given block into the free list by increasing address order.
 * 
 * @param block the free block to insert into the free list.
 */
static void update_free_list(memory_block_t *old_block,
        memory_block_t *new_free_block) {
    assert(old_block != NULL);
    assert(new_free_block != NULL && !is_allocated(new_free_block));
    if (num_free_blocks == 1) {
        free_head = new_free_block;
        return;
    }
    memory_block_t *prev = free_head;
    memory_block_t *cur = free_head;

    while (cur != old_block) {
        prev = cur;
        cur = cur->next;
    }
    prev->next = new_free_block;
    new_free_block->next = cur->next;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 * 
 * @return a pointer to the new free block (same if no coalescing occured).
 */
void coalesce(memory_block_t *block) {
    assert(block != NULL);

    memory_block_t *prev = free_head;
    memory_block_t *cur = free_head;

    while (cur != block && cur != NULL) {
        prev = cur;
        cur = cur->next;
    }
    assert(cur != NULL);
    size_t prev_size = get_size(prev);
    size_t cur_size = get_size(cur);

    // a free block after block
    if (cur + cur_size == cur->next) {
        size_t size = cur_size + get_size(cur->next);
        if (size % ALIGNMENT != 0) {
            size -= size % ALIGNMENT;
        }
        put_block(cur, size, false);
        cur->next = cur->next->next;
        cur->next->next = NULL;
        num_free_blocks--;
    }
    // a free block before block
    if (prev + prev_size == cur) {
        size_t size = prev_size + cur_size;
        if (size % ALIGNMENT != 0) {
            size -= size % ALIGNMENT;
        }
        put_block(prev, size, false);
        prev->next = cur->next;
        cur->next = NULL;
        num_free_blocks--;
    }
}



/*
 * uinit - Used to initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    // put initial heap size
    free_head = extend(PAGESIZE * 2);
    put_block(free_head, heap_size, false);
    num_free_blocks = 1;
    // check for errors
    if (get_size(free_head) != PAGESIZE * 2 || heap_size != PAGESIZE * 2) {
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

    if (is_allocated(ptr)) {
        put_block(new_free_block, get_size(new_free_block), false);

        memory_block_t *cur = free_head;
        // update free list
        if (num_free_blocks == 2 && new_free_block < cur) {
            memory_block_t *temp = free_head;
            free_head = new_free_block;
            new_free_block->next = temp;

        } else {
            while (cur->next != NULL && cur->next < new_free_block) {
                cur = cur->next;
            }
            new_free_block->next = cur->next;
            cur->next = new_free_block;
        }
        num_free_blocks++;
        // TODO consider deferred coalescing (slide 32)
        coalesce(new_free_block);
    }
}