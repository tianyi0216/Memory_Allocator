/* name:  Tianyi Xu
 * netid: txu223
*/
#include "mem.h"                      
extern BLOCK_HEADER* first_header;
// function prototypes for this project
int Is_Allocated(BLOCK_HEADER* header);
void Set_Free(BLOCK_HEADER* header);
void Set_Allocated(BLOCK_HEADER* header);
void *Get_User_Pointer(BLOCK_HEADER* header);
BLOCK_HEADER* Get_Next_Pointer(BLOCK_HEADER* header);
int Get_Block_Size(BLOCK_HEADER* header);
int Min_Block_Size(int size);

/*
 * Check if the header is for an allocated block or not by using bitwise and.
 * Return 1 indicate the block is allocated and 0 indicate this block is free
 */
int Is_Allocated(BLOCK_HEADER* header){
    return header -> size_alloc & 1;
}

/*
 * Set the block for the header to free by using bitwise and to change last
 * bit to 0.
 */
void Set_Free(BLOCK_HEADER* header){
    header -> size_alloc = header -> size_alloc & 0xFFFFFFFE;
}

/*
 * Set the block from the header to allocated by change the last bit to 1.
 */
void Set_Allocated(BLOCK_HEADER* header){
    header -> size_alloc += 1;
}

/*
 * Return the user pointer from the header.
 */
void * Get_User_Pointer(BLOCK_HEADER* header){
    // cast to int so can do arithmetic, user pointer is 8 byte ahead of header address	
    void * user_ptr = (void *)((unsigned long)header + 8);
    return user_ptr;
}

/*
 * Return the next header's address from the header in the parameter.
 */
BLOCK_HEADER* Get_Next_Pointer(BLOCK_HEADER * header){
    // find the current block size and then add to header cast to long type.
    // cast back to a block header after computation
    int block_size = header -> size_alloc & 0xFFFFFFFE;	
    BLOCK_HEADER* next_pointer = (BLOCK_HEADER *)((unsigned long)header + block_size);
    return next_pointer;
}

/*
 * Return the block size of a current header by set the last bit to 0
 */
int Get_Block_Size(BLOCK_HEADER* header){
    return header -> size_alloc & 0xFFFFFFFE;
}

/*
 * return the minimum block size needed that matches the alignment requirment of 16 for a given size
 */
int Block_Size(int size){
    int block_size = size + 8;
    if((block_size) <= 16) return 16; // return 16 if the needed is less than 16
    while(block_size % 16) block_size++; // else round up to the nearest multiple of 16
    return block_size;
}

// return a pointer to the payload
// if a large enough free block isn't available, return NULL
void* Mem_Alloc(int size){
    // traverse the list to find a free block here
    // find a free block that's big enough
    BLOCK_HEADER *current = first_header;
    BLOCK_HEADER *to_use = NULL;

    if(size == 0) return NULL;

    int min_block_size = Block_Size(size); // the minimum size of block required for the given size

    while(1){
        if(current -> size_alloc == 1) break; // reached the end, break
	
	// check whether the current block is allocated or not, if allocated, move on.
	int alloc = Is_Allocated(current);
	if(alloc){
	    current = Get_Next_Pointer(current);    
	} else{
	    // if not allocated, check if the payload is big enough for the given size, if it is,
	    // allocate the block for user
	    if(current -> size_alloc >= min_block_size){
	        to_use = current;
		break;
	    }
	    current = Get_Next_Pointer(current);
	} 
    }
    
    // return NULL if we didn't find a block
    if(to_use == NULL) return NULL;

    // allocate the block
    Set_Allocated(to_use);
    to_use -> payload = size;

    // split if necessary (if padding size is greater than or equal to 16 split the block)
    // calculate the actual size block allocated
    int block_size = Get_Block_Size(to_use);
    // calculate padding by minus the block needed minus from acutal block size
    int padding = block_size - min_block_size;

    // splitting if necessary
    BLOCK_HEADER* split_block;
    if(padding >= 16){
	// create a new header at where the allocated block + minimum size required
        unsigned long address = (unsigned long)to_use + min_block_size;
	split_block = (BLOCK_HEADER*)address;
	// set the size_allocated and paylpoad for the new size
	split_block -> size_alloc = block_size - min_block_size;
	split_block -> payload = split_block -> size_alloc - 8;
	// change size_alloc for the allocated block to relfect new sizes
	to_use -> size_alloc = min_block_size + 1;
    }

    return Get_User_Pointer(to_use);
}


// return 0 on success
// return -1 if the input ptr was invalid
int Mem_Free(void *ptr){
    BLOCK_HEADER *current = first_header;
    BLOCK_HEADER *to_free = NULL;

    // traverse the list and check all pointers to find the correct block
    while(1){
        if(current -> size_alloc == 1) break; // break loop if reach the end
	
	int alloc = Is_Allocated(current);
	// look from allocated blocks only
	if(alloc){
	    // check if ptr is match
	    if((unsigned long)current + 8 == (unsigned long)ptr){
	        // match, free this block
		to_free = current;
		break;
	    } else {
	    	// does not match, go to next
		current = Get_Next_Pointer(current);
	    }
	} else {
            current = Get_Next_Pointer(current); 
	}
    }  
    
    // if you reach the end of the list without finding it return -1
    if(to_free == NULL) return -1;

    int free_alloc = Is_Allocated(to_free);
    if(free_alloc == 0) return -1; // if ptr is already freed
    
    // free the block 
    Set_Free(to_free);
    to_free -> payload = to_free -> size_alloc - 8;  
    
    // coalesce adjacent free blocks
    // do block after first
    BLOCK_HEADER* after = Get_Next_Pointer(to_free); // find block after
    int after_alloc = Is_Allocated(after);

    // if block is free, set the block just free to include the block after.
    if(after_alloc == 0){
	to_free -> size_alloc = to_free -> size_alloc + after -> size_alloc;
	to_free -> payload = to_free -> size_alloc - 8;
    }

    // do before
    BLOCK_HEADER *before = first_header;
    // traverse to find the block before 
    while(1){
	if(before -> size_alloc == 1) break;
	BLOCK_HEADER* next = Get_Next_Pointer(before);
	if(next == to_free) break;
	before = Get_Next_Pointer(before);
    }
    
    // if block is free, set this block to incude the one just free
    int before_alloc = Is_Allocated(before);
    if(before_alloc == 0){
	before -> size_alloc = before -> size_alloc + to_free -> size_alloc;
	before -> payload = before -> size_alloc - 8;
    }
    
    return 0;
}


