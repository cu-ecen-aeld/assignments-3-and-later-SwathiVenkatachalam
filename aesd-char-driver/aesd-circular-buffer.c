/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    uint8_t out_offs = 0;
    // Check input for NULL ptr before dereferencing it
	if(buffer == NULL)
    	return NULL;
    	
    // Check input for NULL ptr before dereferencing it
	if(entry_offset_byte_rtn == NULL)
    	return NULL;
    	
    // The first location in the entry structure to read from; stored in local var
    out_offs = buffer->out_offs;
    
    // Variable to loop through buffer
    uint8_t entry_num = 0;
    
    // Var to store total offset
    size_t total_offset = 0;
    
    // Loop until max possible reached
    while (entry_num < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        // If char_offset less than size, offset within buffer
        if (char_offset < total_offset + buffer->entry[out_offs].size) 
        {
            // pointer specifying a location to store the byte of the returned aesd_buffer_entry buffptr mem corresponding to char_offset
            // set when a matching char_offset is found in aesd_buffer
            *entry_offset_byte_rtn = char_offset - total_offset;
            // struct aesd_buffer_entry structure representing the position described by char_offset returned
            return &(buffer->entry[out_offs]);
        }
        
        // Increase total_offset when going to next entry
        total_offset += buffer->entry[out_offs].size;
        
        //  Increment out_offs; If max support val reached, wrap around index to 0, CB
        out_offs = (out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;    	
    	
    	// Increment entry_num to loop
    	entry_num++;
    }
    
    // NULL returned if pos not available in buffer (not enough data is written).	
    return NULL;	
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    const char* ret = NULL;
    // Check input for NULL ptr before dereferencing it
	if(buffer == NULL)
    	return ret;
    	
    // Check input for NULL ptr before dereferencing it
	if(add_entry == NULL)
    	return ret;
    	
    if (buffer->full)
    {
    	ret = buffer->entry[buffer->in_offs].buffptr;  //return buffer to free b4 overwrite
    }
    // 1. Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs; Structure members of add_entry added to buffer

    // const char *buffptr; A location where the buffer contents in buffptr are stored
    // buffptr member of entry struct inside buffer at index in_offs loaded with buffptr of add_entry struct
    buffer->entry[buffer->in_offs].buffptr = add_entry->buffptr;
    
    // size_t size; Number of bytes stored in buffptr
    // size member of entry struct inside buffer at index in_offs loaded with buffptr of add_entry struct
    buffer->entry[buffer->in_offs].size = add_entry->size;
    
    // Increment in_offs; If max support val reached, wrap around index to 0, CB
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    
    // 2. If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the new start location.
    // in_offs = current location in the entry structure where the next write should be stored.
    if (buffer->full)
    {
    	ret = buffer->entry[buffer->in_offs].buffptr;  //return buffer to free b4 overwrite
        buffer->out_offs = buffer->in_offs;
    }
    else if (buffer->out_offs == buffer->in_offs)   // Check if both indexes match, if so CB full
        buffer->full = true; 
        
    return ret;	
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
