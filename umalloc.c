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
unsigned long num_free_blocks;
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
 * print the size of each free block in the free list.
 * for debugging purposes.
 */
void print_list(const char location[]) {
    memory_block_t *cur = free_head;
    printf("DEBUG: %s\n", location);
    printf("DEBUG: ");
    while (cur != NULL) {
        printf("%p: %zu, ", cur, get_size(cur));
        assert(cur != cur->next);
        cur = cur->next;
    }
    printf("heap size: %zu\n", heap_size);
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
    if (size > PAGESIZE * ALIGNMENT) {
        size = PAGESIZE * ALIGNMENT - ALIGNMENT;
    }
    // creates new free block to represent new heap memory
    memory_block_t *result = (memory_block_t *) csbrk(size + ALIGNMENT);
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
    // put split allocated block in memory
    size_t free_size = get_size(block) - size;
    memory_block_t *temp = block->next;
    put_block(block, size - ALIGNMENT, true);
    block->next = temp;
    // create new split free block by setting pointer to block address + size
    memory_block_t *new_free_block = (memory_block_t *) ((char *) block + size);
    if (free_size % ALIGNMENT != 0) {
        free_size -= free_size % ALIGNMENT;
    }
    put_block(new_free_block, free_size, false);
    update_list(block, new_free_block);
    assert(get_size(block) == size - ALIGNMENT);
    // print_list("AFTER update_list");
    assert(free_head->next == NULL || free_head < free_head->next);
    
    return get_payload(block);
}

/**
 * insert the given block into the free list by increasing address order.
 * 
 * @param block the free block to insert into the free list.
 */
void update_list(memory_block_t *old_block,
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
 * 
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
        memory_block_t *temp = cur->next->next;
        put_block(cur, cur_size, false);
        cur->next = temp;
        num_free_blocks--;
    }
    // a free block before block
    if ((memory_block_t *) ((char *) prev + prev_size) == cur) {
        size_t size = prev_size + get_size(cur);
        if (size % ALIGNMENT != 0) {
            size -= size % ALIGNMENT;
        }
        memory_block_t *temp = cur->next;
        put_block(prev, size, false);
        prev->next = temp;
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
    if (get_size(free_head) != PAGESIZE * 2 || heap_size != PAGESIZE * 2) {
        return -1;
    }

    return EXIT_SUCCESS;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    if (size % ALIGNMENT != 0) {
        size += ALIGNMENT - (size % ALIGNMENT);
    }
    // find free block to put it
    memory_block_t *result = find(size);

     // no need to split
    if (get_size(result) - size < ALIGNMENT * 2)  {
        assert(get_size(result) - size - ALIGNMENT >= 0);
        memory_block_t *temp = result->next;
        put_block(result, size, true);
        result->next = temp;
        if (num_free_blocks == 1) {
            free_head = extend(heap_size);
            return get_payload(result);
        }
        memory_block_t *prev = free_head;
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
    assert(ptr != NULL);
    memory_block_t *new_free = (memory_block_t *) get_block(ptr);

    if (is_allocated(new_free)) {
        num_free_blocks++;
        put_block(new_free, get_size(new_free), false);

        memory_block_t *cur = free_head;
        // update free list
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
        // TODO consider deferred coalescing (slide 32)
        // print_list("before coalesce");
        coalesce(new_free);
        // print_list("AFTER coalesce");
    }
    assert(free_head->next == NULL || free_head < free_head->next);
}