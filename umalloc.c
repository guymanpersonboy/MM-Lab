#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Christopher Carrasco cc66496" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;
// keeps count of the number of free blocks that should be in the free list
unsigned long num_free_blocks;
// the size of the heap minus headers
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
    while (result->next != NULL) {
        if (get_size(result) == size || get_size(result) > size + ALIGNMENT) {
            return result;
        }
        assert(result != result->next);
        result = result->next;
    }
    // check last free block
    if (get_size(result) == size || get_size(result) > size + ALIGNMENT) {
        return result;
    }
    // need more room! set last free block next to extend result
    result->next = extend(heap_size + ALIGNMENT);
    result = result->next;
    while (get_size(result) < size) {
        result->next = extend(heap_size + ALIGNMENT);
        coalesce(result);
    }    
    num_free_blocks++;

    return result;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    if (size > PAGESIZE * ALIGNMENT - ALIGNMENT) {
        size = PAGESIZE * ALIGNMENT - ALIGNMENT;
    }
    // creates new free block to represent new heap memory
    memory_block_t *result = (memory_block_t *) csbrk(size + ALIGNMENT);
    assert(result != NULL);
    put_block(result, size, false);
    // double heap_size until larger than PAGESIZE * ALGNMENT - ALIGNMENT
    heap_size += size;
    
    return result;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    // put split allocated block in memory
    size_t free_size = get_size(block) - size;
    block->block_size_alloc = (size - ALIGNMENT) | true;
    // create new split free block by setting pointer to block address + size
    memory_block_t *new_free_block = (memory_block_t *) ((char *) block + size);
    if (free_size % ALIGNMENT != 0) {
        free_size -= free_size % ALIGNMENT;
    }
    put_block(new_free_block, free_size, false);
    update_list(block, new_free_block);
    assert(get_size(block) == size - ALIGNMENT);
    assert(free_head->next == NULL || free_head < free_head->next);
    
    return get_payload(block);
}

/**
 * remove the old_block from the free list and insert the new_free_block. 
 * 
 * @param old_block the split block to be allocated
 * @param new_free_block the split free block
 */
void update_list(memory_block_t *old_block,
        memory_block_t *new_free_block) {
    if (num_free_blocks == 1) {
        free_head = new_free_block;
        return;
    }
    memory_block_t *prev = free_head;
    memory_block_t *cur = free_head;
    // search for old_block to remove
    while (cur != NULL && cur != old_block) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == free_head) {
        free_head = new_free_block;
        new_free_block->next = old_block->next;
    } else {
        prev->next = new_free_block;
        new_free_block->next = cur->next;
    }
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
void coalesce(memory_block_t *block) {
    assert(block != NULL);
    memory_block_t *prev = free_head;
    memory_block_t *cur = free_head;

    while (cur != NULL && cur != block) {
        prev = cur;
        cur = cur->next;
    }
    size_t prev_size = get_size(prev) + ALIGNMENT;
    size_t cur_size = get_size(cur) + ALIGNMENT;

    // a free block after block
    if ((memory_block_t *) ((char *) cur + cur_size) == cur->next) {
        cur_size = cur_size + get_size(cur->next);
        if (cur_size % ALIGNMENT != 0) {
            cur_size -= cur_size % ALIGNMENT;
        }
        cur->block_size_alloc = cur_size | false;
        cur->next = cur->next->next;
        num_free_blocks--;
    }
    // a free block before block
    if ((memory_block_t *) ((char *) prev + prev_size) == cur) {
        prev_size = prev_size + get_size(cur);
        if (prev_size % ALIGNMENT != 0) {
            prev_size -= prev_size % ALIGNMENT;
        }
        prev->block_size_alloc = prev_size | false;
        prev->next = cur->next;
        num_free_blocks--;
    }
}



/*
 * uinit - Used to initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    // put initial heap size to 8192B + hidden 16 for header
    free_head = extend(PAGESIZE * 2);
    num_free_blocks = 1;
    // check for errors
    if (free_head == NULL || get_size(free_head) != PAGESIZE * 2 || heap_size != PAGESIZE * 2) {
        return -1;
    }

    return EXIT_SUCCESS;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    // align
    if (size % ALIGNMENT != 0) {
        size += ALIGNMENT - (size % ALIGNMENT);
    }
    // find free block to put it
    memory_block_t *result = find(size);

     // no need to split
    if (get_size(result) - size < ALIGNMENT * 2)  {
        result->block_size_alloc = size | true;
        // found block is only one left, extend heap
        if (num_free_blocks == 1) {
            free_head = extend(heap_size);
            return get_payload(result);
        }
        memory_block_t *prev = free_head;
        // find the free block before result to remove result
        if (prev == result) {
            free_head = prev->next;
        } else {
            while (prev->next != NULL && prev->next != result) {
                prev = prev->next;
            }
            prev->next = prev->next->next;
        }
        num_free_blocks--;
        assert(free_head->next == NULL || free_head < free_head->next);

        return get_payload(result);
    }
    // split the free block into an allocated and free block
    // and return allocated payload address.
    return split(result, size + ALIGNMENT);
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    memory_block_t *new_free = (memory_block_t *) get_block(ptr);

    if (is_allocated(new_free)) {
        num_free_blocks++;
        put_block(new_free, get_size(new_free), false);

        memory_block_t *cur = free_head;
        // update free list in increasing address order
        if (new_free < free_head) {
            memory_block_t *temp = free_head;
            free_head = new_free;
            new_free->next = temp;
        } else {
            while (cur->next != NULL && cur->next < new_free) {
                cur = cur->next;
            }
            new_free->next = cur->next;
            cur->next = new_free;
        }

        coalesce(new_free);
    }
    assert(free_head->next == NULL || free_head < free_head->next);
}