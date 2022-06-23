/*
 * mm.c
 *
 * Name: Matthew Sites
 * SID: mjs7938
 * Date: 06/15/2022
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * Also, read malloclab.pdf carefully and in its entirety before beginning.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"
#include "stree.h"
#include "config.h"

/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 */
 #define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER

/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */

/* What is the correct alignment?*/
#define ALIGNMENT 16


/* Global Variables: Only allowed 128 bytes, pointers are 8 bytes each */

size_t TOH_bytes_left = 0; // What byte the top of the unallocated heap is

// static char *free_root = NULL; // The root of the the free list
static char *TOH = NULL; // next free payload pointer of the unallocated heap area


/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x){
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

/*
 * Initialize: returns false on error, true on success.
 */
bool mm_init(void){

    // Initial allocate of 8 words
    char *mem_brk = mem_sbrk(32);

    // Initial allocation failed
    if(mem_brk == NULL || *(int*)mem_brk == -1){
        printf("Initial 16 byte allocation failed\n");
        return false;
    }

    // Set unused blocks
    put(mem_brk, 0);

    // Set prologue block 
    put(mem_brk + 8, pack(16, 1));
    put(mem_brk + 16, pack(16, 1));

    // Set initial epilogue block 
    put(mem_brk + 24 , pack(0, 1));

    // Update TOH_bytes_left 
    TOH_bytes_left += 32;

    // Allocate the first free block
    if(!allocate_page()){
        printf("Initial page allocation failed\n");
        return false;
    }

    return true;
}

/*
 * malloc
 */
void* malloc(size_t size){
    // size + header + footer and allign (in bytes)
    int block_size = align(size+16);

    // Error check; sbreak failing is checked lower
    if(size == 0){ // size is 0
        printf("Malloc Returning NULL: size == 0\n");
        return NULL;
    }

    // Search free list for a block that will fit size

    /* 
    * If you find a block, put it there and add 
    * the remaining bits to the free list.
    */

    /////////////////////////////
    // When there is no blocks 
    // in the free list suitable
    // to fit the block size,
    // add it to the top of heap
    /////////////////////////////

    /* 
    * Increment TOH by block_size bytes;
    * Allocate pages if needed;
    * cast TOH to char to work in bytes
    */
    void *tmp_pos = TOH + block_size; // tmp_pos = how far the block will extend (before allocation) also next PP

    // allocate page if tmp_pos exceeds the current heap size (ADD = minus the epilogiue)
    while(tmp_pos >= mem_heap_hi()){
        if(!allocate_page()){
            printf("Page allocation failed during malloc");
            return NULL;
        }
    }

    // set the header and footer of the allocated block
    put(GHA(TOH), pack(block_size, 1));
    put(GFA(TOH), pack(block_size, 1)); 

    /*
    * set the newly allocated bits that arent
    * used in the malloc to the free list
    */

   /*
   * Set header and footer for un-used bytes after updating what byte TOH is at
   * Only update TOH_bytes_lefts if the block is placed at the end of the heap
   */
    TOH_bytes_left += block_size;
    put(GFA(TOH) + 8, pack(mem_heapsize() - TOH_bytes_left, 0));
    put((char*)mem_heap_hi() - 15, pack(mem_heapsize() - TOH_bytes_left, 0)); // -15 because off by 1 error?

    // update 
    TOH = (char*)tmp_pos;

    // return payload location 
    return (TOH - block_size);
}

/*
 * free
 */
void free(void* payload_pointer)
{   
    size_t size = get_size(GHA(payload_pointer));

    put(GHA(payload_pointer), pack(size, 0));
    put(GFA(payload_pointer), pack(size, 0));
    coalesce(payload_pointer); // Add this to free list when you get there
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    /* IMPLEMENT THIS */
    return NULL;
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/*
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Returns whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno)
{
#ifdef DEBUG
    /* Write code to check heap invariants here */
    /* IMPLEMENT THIS */
#endif /* DEBUG */
    return true;
}

/*
* Allocates a page and coalesces to set TOH to the first unused payload pointer of allocated memory
*/
bool allocate_page(){

    // Allocate a page (4096 bytes);
    void *payload_pointer = mem_sbrk(4096); // mem-brk returns a PP in this implimentation

    // Initial allocation failed
    if(payload_pointer == NULL || *(int*)payload_pointer == -1){
        printf("Page allocation failed: heap size %zu/%llu bytes\n", mem_heapsize() + 4096, MAX_HEAP_SIZE);
        return false;
    }

    // Set footer and header blocks for allocated block
    put(GHA(payload_pointer), pack(4096,0)); // Overwrites old epilogue header
    put(GFA(payload_pointer), pack(4096,0));

    // Set prev/next free block
    // memset((char*)block_pointer + 4, &free_root, 8);

    // Set new epilogue header
    put(GHA(next_blk(payload_pointer)), pack(0,1));
    
    // Update TOH 
    TOH = coalesce(payload_pointer);

    // dbg_printf("Page allocated: heap size %zu/%llu bytes\n", mem_heapsize(), MAX_HEAP_SIZE);

    return true;
}

/*
* Pack: create a value for the header/footer
*/
size_t pack(size_t size, int alloc){
    // Bitwise or size and alloc into one 8 byte number
    return (size | alloc);
}

/*
* GHA: returns the addressof the header via a payload pointer
*/
void *GHA(void *payload_pointer){
    return((char*)payload_pointer - 8);
}

/*
* GFA: returns the address of the footer via a payload pointer
*/
void *GFA(void *payload_pointer){
    return((char*)payload_pointer + get_size(GHA(payload_pointer)) - 16);    
}

/*
* get: returns a word from addr as a size_t
*   - Used in conjuction with get_size & get_alloc
*/
size_t get(void *addr){
    return(*(size_t *)addr);
}

/*
* get_size: gets the size of a header/footer in bytes
*/
size_t get_size(void *addr){
    return(get(addr) & ~0xF); 
}

/*
* get_alloc: returns if the addr block is allocated
*/
int get_alloc(void *addr){
    return(get(addr) & 0x1);
}

/*
* put: puts a header/footer value at addr
*/ 
void put(void* addr, size_t val){
    *(size_t *)addr = val;
}

/*
* prev_fblk: gets the address of the previous free blocks payload pointer
*/
char *prev_blk(void* payload_pointer){
    return((char*)payload_pointer - get_size((char*)payload_pointer - 16));
}

/*
* next_fblk: gets the address of the next free blocks payload pointer
*/
char *next_blk(void* payload_pointer){
    return((char*)payload_pointer + get_size((char*)payload_pointer - 8));
}

/*
* coalesce: merges adjacent free blocks, returns the payload pointer
*/
void* coalesce(void *payload_pointer){
    size_t prev_block = get_alloc(GFA(prev_blk(payload_pointer)));
    size_t next_block = get_alloc(GHA(next_blk(payload_pointer)));
    size_t block_size = get_size(GHA(payload_pointer));

    // prev and next, allocated
    if(prev_block && next_block){
        return(payload_pointer);
    }
    // prev allocated, next not allocated
    else if(prev_block && !next_block){
        block_size += get_size(GHA(next_blk(payload_pointer)));
        put(GHA(payload_pointer), pack(block_size,0));
        put(GFA(next_blk(payload_pointer)), pack(block_size, 0));
    }

    // prev not allocated, next allocated
    else if(!prev_block && next_block){
        block_size += get_size(GHA(prev_blk(payload_pointer)));
        put(GHA(prev_blk(payload_pointer)), pack(block_size, 0));
        put(GFA(payload_pointer), pack(block_size,0));
        payload_pointer = prev_blk(payload_pointer);
    }

    // prev and next, not allocated
    else{
        block_size += get_size(GHA(prev_blk(payload_pointer))) + get_size(GFA(next_blk(payload_pointer)));
        put(GHA(prev_blk(payload_pointer)), pack(block_size,0));
        put(GFA(next_blk(payload_pointer)), pack(block_size,0));
        payload_pointer = prev_blk(payload_pointer);
    }

    return(payload_pointer);
}


