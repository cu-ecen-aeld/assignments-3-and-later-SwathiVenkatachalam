/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 * @updated Swathi Venkatachalam
 * @date    2024-03-17
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"

#include <linux/slab.h>  // For memory allocation functions

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("SWATHI VENKATACHALAM"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

#define SUCCESS (0)  // Return cod checking macro

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
     
    // Ref: From my A7 scull_open function https://github.com/cu-ecen-aeld/assignment-7-SwathiVenkatachalam/blob/master/scull/main.c
    struct aesd_dev *dev; // device information
    
    // Ref: https://radek.io/2012/11/10/magical-container_of-macro/
    // container_of(ptr, type, member) macro 
    // Get ptr to aesd_dev struct; i_cdev is a memeber of inode struct that holds ptr to cdev (char device struct mem of struct aesd_dev)
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    
    // Set file ptr filp->private_data with aesd_dev device struct
    filp->private_data = dev; 

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

// Ref: Assignment-8-overview lecture slides
/*
Steps to read data from kernel space to user space buf

1. Check input parameters validity
2. Get aesd_dev using file ptr private data
3. Lock with error handling
4. Find entry offset with error handling
5. Get bytes to read and bytes that can be read checking bounding param count (so wk if it's going to be a complete/ partial read)
6. Copy to user space buf with error handling
7. Update f_pos to point to next offset
8. Retval set to complete/ partial read bytes
9. Unlock and exit

*/
// Read data from CB of device considering partial read, EOF and errors.

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
     
    // Check input parameters validity first
    // filp - file pointer private_data member used to get aesd_dev
    // buf - buffer to fill
    // f_pos - pointer to the read offset; references loc in virtual device (specific byte of CB linear content)
    if(filp == NULL || buf == NULL || f_pos == NULL)
    {
    	PDEBUG("Input parameters of read function invalid; memory access failure\n");
        return -EFAULT; // mem access failure error code
    }
    
    // Check input parameters validity first
    // count - max number of bytes to write to buff; may want/need to write less than this
	if (count == 0)
	{
		PDEBUG("No data to write; return success\n");
		return 0;
	}
		
	// file pointer filp private_data used to get aesd_dev
	struct aesd_dev *dev = filp->private_data;
	
	int rc; // return code storage variable
	
	// Lock for safe multi-threaded op
	rc = mutex_lock_interruptible(&dev->lock); //  check in rc if lock acquisition interrupted by a signal
	if (rc != SUCCESS)
	{
		PDEBUG("Lock failure in read;  lock acquisition was interrupted by a signal");
		return -ERESTARTSYS;  //restart syscall code
	}
	
	size_t entry_offset_byte_rtn = 0; // contains offset
	struct aesd_buffer_entry *data_entry = NULL; // initialize to get data entry in CB
	
	// From aesd-circular-buffer.h 
	// struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer, size_t char_offset, size_t *entry_offset_byte_rtn );
	data_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&device->buffer, *f_pos, &entry_offset_byte_rtn);
	
	// Check if entry doesn't exist
    if(data_entry == NULL)
    {
        PDEBUG("No data to read; exit");
        mutex_unlock(&dev->lock);  // unlock mutex
        return retval;             // return
    }
    
    size_t bytes_to_read_in_entry = data_entry->size - entry_offset_byte_rtn;
    size_t bytes_can_be_read = 0;
    
    // If bytes to read exceeds max allowed count, can only read till count; considering partial read possibility
    if (bytes_to_read_in_entry > count)
    {
    	PDEBUG("Partial read!\n");
    	bytes_can_be_read = count;
    }
    else
    {
        // Considering complete read possible
     	PDEBUG("Complete read!\n");
    	bytes_can_be_read = bytes_to_read_in_entry;   	
    }
    
    // use copy_to_user to fill buf from kernel to user space, as we cannot access buf directly
    // Ref: https://manpages.org/__copy_to_user/9
    // copy_to_user(void __user * to, const void * from, unsigned long n);
    // buf = Destination address, in user space.
    // data_entry->buffptr + entry_offset_byte_rtn = Source address, in kernel space.(starting pt within buffer to copy from)
    // bytes_can_be_read = Number of bytes to copy.
    rc = copy_to_user(buf, data_entry->buffptr + entry_offset_byte_rtn, bytes_can_be_read);
    if (rc)
    {
        PDEBUG("Failed to copy to user space buf; exit");
        mutex_unlock(&dev->lock);  // unlock mutex
        return -EFAULT;             // return    	
    }
    
    retval = bytes_can_be_read;  // bytes read stored in retval depending on complete/ partial read
	*f_pos += retval;            // update f_pos to point to next offset
     
    PDEBUG("Read success!");
    mutex_unlock(&dev->lock);  // unlock mutex 
    return retval;
}


// Write data from user space buf to device in kernel space
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
     
    // Check input parameters validity first
    // filp - file pointer private_data member used to get aesd_dev
    // buf - buffer to fill from
    // f_pos - point to loc where write would start
    if(filp == NULL || buf == NULL || f_pos == NULL)
    {
    	PDEBUG("Input parameters of read function invalid; memory access failure\n");
        return -EFAULT; // mem access failure error code
    }
    
    // Check input parameters validity first
	if (count == 0)
	{
		PDEBUG("No data to write; return success\n");
		return 0;
	}
		
	// file pointer filp private_data used to get aesd_dev
	struct aesd_dev *dev = filp->private_data;
	
	// Allocate mem to store write data from user space onto kernel space
	// Ref: https://manpages.org/kmalloc/9
	// void * kmalloc(size_t size, gfp_t flags);
	char *write_data_uspace = kmalloc(count, GFP_KERNEL); //GFP_KERNEL - Allocate normal kernel ram. May sleep.
	if (write_data_uspace == NULL)
	{
		PDEBUG("Kmalloc failure!\n");
	    mutex_unlock(&dev->lock);  // unlock mutex 
        return retval;		
	}
	
	int rc; // return code storage variable
	
	// Lock for safe multi-threaded op
	rc = mutex_lock_interruptible(&dev->lock); //  check in rc if lock acquisition interrupted by a signal
	if (rc != SUCCESS)
	{
		PDEBUG("Lock failure in write;  lock acquisition was interrupted by a signal");
		return -ERESTARTSYS;  //restart syscall code
	}	
	
	// use copy_from_user to fill buf from user to kernel spce, as we cannot access buf directly
    // Ref: https://manpages.debian.org/testing/linux-manual-4.8/__copy_from_user.9.en.html
    // copy_from_user(void __user * to, const void * from, unsigned long n);
    // write_data_uspace = Destination address, in kernel space.
    // buf = Source address, in user space
    // count = Number of bytes to copy.
    rc = copy_from_user(write_data_uspace, buf, count);
    if (rc)
    {
        PDEBUG("Failed to copy from user space buf; exit");
        mutex_unlock(&dev->lock);  // unlock mutex
        kfree(write_data_uspace);
        return -EFAULT;             // return    	
    }   
    
    // Check if CB empty
	if (dev->write_entry.size == 0)
	{
		dev->write_entry.buffptr = kmalloc(count,GFP_KERNEL);
	}
	else // if not reallocate mem
	{
	    // Ref: https://manpages.org/krealloc/9
    	// void * krealloc(const void * p, size_t new_size, gfp_t flags);
    	// dev->write_entry.buffptr = obj to reallocate mem for
    	// dev->write_entry.size + count = new size
    	// flag = GFP_KERNEL (Allocate normal kernel ram. May sleep.)
		dev->write_entry.buffptr = krealloc(dev->write_entry.buffptr, dev->write_entry.size + count, GFP_KERNEL);
	}
		
	// Check mem alloc failure
	if (dev->write_entry.buffptr == NULL)
	{ 
	    PDEBUG("K Alloc failure!\n");
	    mutex_unlock(&dev->lock);  // unlock mutex 
	    kfree(write_data_uspace);
        return retval;
	}
	
	// copy data to write entry, most recent, append
	// Ref: https://www.man7.org/linux/man-pages/man3/memcpy.3.html
	// void *memcpy(dest, src, n)
	void *dest = dev->write_entry.buffptr + dev->write_entry.size;
    memcpy(dest, write_data_uspace, count);
    dev->write_entry.size += count;
    
    // Check new line char
    // Ref: https://www.man7.org/linux/man-pages/man3/memchr.3.html
    // Scan mem for '\n' char
    void *newline_ptr = memchr(write_data_uspace, '\n', count);
    if(newline_ptr != NULL)  // Found '\n'
    {
        // add to CB(buffer, entry)
        return_entry = aesd_circular_buffer_add_entry(&dev->buffer, &dev->write_entry);
        if(return_entry != NULL)  
    		kfree(return_entry); // free overwritten if failure

        // clear current
        dev->write_entry.buffptr = NULL;
        dev->write_entry.size = 0;
    }
    
    retval = count;
	
    PDEBUG("Write success!");
    mutex_unlock(&dev->lock);  // unlock mutex 
    kfree(write_data_uspace);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
     
	mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.buffer);
    aesd_device.write_entry.buffptr = NULL;
    aesd_device.write_entry.size    = 0;

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    struct aesd_buffer_entry *entryptr = NULL;
    int index = 0;
     /*
    #define AESD_CIRCULAR_BUFFER_FOREACH(entryptr,buffer,index) \
    for(index=0, entryptr=&((buffer)->entry[index]); \
            index<AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; \
            index++, entryptr=&((buffer)->entry[index]))*/
            
    AESD_CIRCULAR_BUFFER_FOREACH(entryptr,&aesd_device.buffer,index) 
    {
        kfree(entryptr->buffptr);
    }
	mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
