////////////////////////////////////////////////////////////////////////////////
// Main File:        p3Heap.c
// This File:        p3Heap.c
// Other Files:      N/A
// Semester:         CS 354 Lecture 002 Fall 2022
// Instructor:       deppeler
// 
// Author:           Jonathan Amalidosan
// Email:            jamalidosan@wisc.edu
// CS Login:         amalidosan
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          N/A
//
// Online sources:   N/A
//////////////////////////// 80 columns wide ///////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2022 Deb Deppeler based on work by Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission Fall 2022, CS354-deppeler
//
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "p3Heap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           

    int size_status;

    /*
     * Size of the block is always a multiple of 8.
     * Size is stored in all block headers and in free block footers.
     *
     * Status is stored only in headers using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *
     *   Bit1 => second last bit 
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     * 
     * End Mark: 
     *  The end of the available memory is indicated using a size_status of 1.
     * 
     * Examples:
     * 
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     * 
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heap_start = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int alloc_size;

/*
 * Additional global variables may be added as needed below
 */

 
/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 * This function must:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split 
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements 
 *       - Update all heap block header(s) and footer(s) 
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 * - If a BEST-FIT block found is NOT found, return NULL
 *   Return NULL unable to find and allocate block for desired size
 *
 * Note: payload address that is returned is NOT the address of the
 *       block header.  It is the address of the start of the 
 *       available memory for the requesterr.
 *
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* balloc(int size) {     

    //value that accounts for size request with header and padding
    int new_size;

    //Checks if size is positive to continue
    if(size <= 0)
    {
        return NULL;
    }

    //Checks if size is smaller than heap space
    if(size > alloc_size)
    {
        return NULL;
    }

    //Gives space for a header
    new_size = sizeof(blockHeader) + size;

    //Adds padding to make sure block size will be a multiple of 8
    while(new_size  % 8 != 0)
    {
        new_size++;
    }

    //Sets a pointer to beginning of heap
    blockHeader *current_block_header = heap_start;
    //pointer created to find the best fit
    blockHeader *smallest_block_header = current_block_header;

    //sizes for blocks
    int current_block_size;
    int smallest_block_size;

    //Value that turns to 1 if we find a best fit
    int best_fit_found = 0;

    while((current_block_header -> size_status) != 1)
    {
        //sizes get that neglect p and a bits for pure size
        current_block_size = (current_block_header -> size_status) - (current_block_header -> size_status & 0x3);
	    smallest_block_size = (smallest_block_header -> size_status) - (smallest_block_header -> size_status & 0x3);

        //sees if current block is free
	    if(((current_block_header -> size_status) & 0x1) == 0)
	    {   
            //Makes sure block being looked at is big enough for requested size
	        if(current_block_size >= new_size)
	        {
                //since we initalized the pointer for best fit at the same position as 
                //the current pointer block, we have to reevaluate the pointer for the
                //smallest block header to see if it is allocated or to too small for
                //the current block
                if((((smallest_block_header -> size_status) & 0x1) == 1) || new_size > smallest_block_size)
	            {
                    //updates the smallest block so we can properly continue
                    smallest_block_header = current_block_header;
                    continue;
                }

                //if block found is smaller than current best fit, we want to replace it
                if(current_block_size <= smallest_block_size)
                {
                    //a block that is a little bit bigger than size
	                if(current_block_size > new_size)
	                {
                        //we have a best fit
                        smallest_block_header = current_block_header;
                        best_fit_found = 1;
                    }

                    //a block that is the exact size requested is found
                    else if(current_block_size == new_size)
	                {
                        //We have a best fit
                        smallest_block_header = current_block_header;
                        best_fit_found = 1;
                        //since we found a matching size, can end search
                        break;
	                }
                }	
	        } 

            else
            {
                //the block we're at is too small so go to the next block
                current_block_header = current_block_size/sizeof(blockHeader) + current_block_header;
                continue;
            }
	    }
        //Traverses to the next block
	    current_block_header = current_block_size/sizeof(blockHeader) + current_block_header;
    }
    
    //In the chance of unsuccessfully finding a block
    if(best_fit_found == 0)  
    {
        return NULL;
    }

    //a header to the block after the best fit
    blockHeader *next_header;

    //For a perfect fit set next block's p-bit to 1 if it isn't the end of heap
    if((smallest_block_size == new_size))
    {
        //Alllocate the a-bit
        (smallest_block_header -> size_status) += 1;

        //Look at the next block
        next_header =  smallest_block_size/sizeof(blockHeader) + smallest_block_header;
        //Makes sure we aren't at end of heap
        if((next_header -> size_status) != 1)
        {
            //sets p-bit of next block to 0
            (next_header -> size_status) += 2;
        }
    }

    //split if best fit wasn't a perfect fit
    else if(smallest_block_size > new_size)
    {   
        //size of the to be free block
        int free_size = smallest_block_size - new_size;

        //Checks if p-bit of best fit is 1
        if(((smallest_block_header -> size_status) & 0x2) == 0x2)
        {
            //If yes, give new size with a-bit and p-bit set
            (smallest_block_header -> size_status) = new_size + 3;
        }
        else
        {
            //Otherwise just give the new size and a-bit set
            (smallest_block_header -> size_status) = new_size + 1;
        }

        //puts header for free block
	    next_header = new_size/sizeof(blockHeader) + smallest_block_header;
        //sets p-bit to 1
        next_header -> size_status = free_size + 2;

        //puts for footer at end of free block
	    blockHeader *block_footer = next_header + free_size/sizeof(blockHeader) - 4/sizeof(blockHeader);
        //footer holds overall size
	    block_footer -> size_status = free_size;  
    }
    
    //returns block data
    return (void *)(smallest_block_header + 1);
    
} 
 
/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */                    
int bfree(void *ptr) {    

    //Checks if ptr is null
    if(ptr == NULL)
    {
        return -1;
    }

    //creates a block header that points to where ptr points to
    blockHeader *block_header = ptr;
    //have to move block_header back to the header 
    //since ptr is pointing to the data of the block
    block_header -= 1;
    //the pure size of the block
    int ptr_size = (block_header -> size_status) - (block_header -> size_status & 0x3);

    //checks if ptr isn't multiple of 8
    if((((int)block_header - (int)heap_start) % 8) != 0)
    {
        return -1;
    }

    //checks if ptr is in the heap space
    if(block_header < heap_start || block_header >= (heap_start + alloc_size/sizeof(blockHeader)))
    {
       return -1;
    }

    //makes sure that block we want to free isn't already free
    if(((block_header -> size_status) & 0x1) == 0)
    {
        return -1;
    }
    else if(((block_header -> size_status) & 0x1) == 1)
    {
        //Set a-bit to 0 to be free
        (block_header -> size_status) -= 1;

        //If we aren't at the end of the heap then set the p-bit of next block to 0
        if(((block_header + ptr_size/sizeof(blockHeader)) -> size_status) != 1)
        {
            ((block_header + ptr_size/sizeof(blockHeader)) -> size_status) -= 2;
        }

        //Sets footer at the end of free block that shows the raw size
        blockHeader *block_footer = block_header + ptr_size/sizeof(blockHeader) - 4/sizeof(blockHeader);
        block_footer -> size_status = ptr_size;
    }
    
    return 0;
} 

/*
 * Function for traversing heap block list and coalescing all adjacent 
 * free blocks.
 *
 * This function is used for delayed coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {

    //pointer to the block we want to try to coalesce
    blockHeader *current_header = heap_start; 
    int current_size = (current_header -> size_status) - (current_header -> size_status & 0x3); 

    //pointer to the block after what we are currently looking at
    blockHeader *next_header = current_header + current_size/sizeof(blockHeader);   
    int next_size = next_header -> size_status - (next_header -> size_status & 0x3);

    //keeps track of many times we coalesced, if at all 
    int coalesced = 0;

    //makes sure we never go past the heap when merging
    while((next_header -> size_status) != 1)
    {   
        //Merges two adjacent blocks if both free
        if((((next_header -> size_status) & 0x1) == 0) && (((current_header -> size_status) & 0x1) ==0))
        {
            //Now we are current header represents the new merged block
            current_header -> size_status = (current_header -> size_status) + next_size;
            current_size = (current_header -> size_status) - (current_header -> size_status & 0x3);

            //The next block is the one right after the merged block
	        next_header = current_header + current_size/sizeof(blockHeader);
	        next_size = (next_header -> size_status) - (next_header -> size_status & 0x3);

            //Updates that we had a successful coalescion
	        coalesced++;
        }

	    else
	    {
            //If coalescion is not possible due to the current or next block being
            //allocated we move the current header to the next block
            current_header = next_header;
	        current_size = (current_header -> size_status) - (current_header -> size_status & 0x3);

            //the next block will go to the block after the current block
            //this might repeat for a while until we find a two adjacent
            //blocks or reach the end
            next_header = current_header + current_size/sizeof(blockHeader);	
            next_size = next_header -> size_status - (next_header -> size_status & 0x3);
	    }

    }

	return coalesced;
}

 
/* 
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int init_heap(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;   // page size
    int padsize;    // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* end_mark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    alloc_size -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heap_start = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    end_mark = (blockHeader*)((void*)heap_start + alloc_size);
    end_mark->size_status = 1;

    // Set size in header
    heap_start->size_status = alloc_size;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heap_start->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heap_start + alloc_size - 4);
    footer->size_status = alloc_size;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void disp_heap() {     
 
    int counter;
    char status[6];
    char p_status[6];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heap_start;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, 
	"*********************************** Block List **********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
	"*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
	"*********************************************************************************\n");
    fflush(stdout);

    return;  
} 

// end of myHeap.c (Spring 2022)                                         


