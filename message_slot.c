#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   
#include <linux/module.h>   
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include "message_slot.h"


MODULE_LICENSE("GPL");

static ch_node* devices_array [256] = {NULL}; // ith cell represents head of LL of that message slot's channels. 
// so each cell contains either NULL or the head of a LL of channel_nodes. 


ch_node* get_channel_ptr(unsigned long channel_id, ch_node* psentinel); // returns pointer to the channel, NULL if no channel with that
// channel id exists
int create_and_append(unsigned long channel_id, ch_node* psentinel); // create new ch_node with the channel_id and
// append to end of LL. Returns 0 on success and -1 on failure
void free_ch_node(ch_node* node); // re;eases memory associated with a single ch_node
void free_cell(ch_node* psentinel); //recursively releases linked list in cell of devices_array
void print_devices_array(void); // for debugging

static int device_open(struct inode* inode, struct file* file);
static ssize_t device_read(struct file* file, char __user* buffer,size_t length, loff_t* offset);
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset);
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param);
static int device_release(struct inode* inode, struct file* file);

//================== HELPER FUNCTIONS IMPLEMENTATION ===========================

ch_node* get_channel_ptr(unsigned long channel_id, ch_node* psentinel)
{
    ch_node* pcurr = psentinel;
    while(pcurr != NULL)
    {
        if(pcurr -> id == channel_id)
        {
            return pcurr;
        }
        pcurr = pcurr -> next;
    }
    return NULL;
}

int create_and_append(unsigned long channel_id, ch_node* psentinel)
{
    ch_node* pnew_channel;
    ch_node* pcurr = psentinel;

    if(psentinel == NULL)
    {
        printk(KERN_ERR "psentinel is NULL in create_and_append\n");
        return -1;
    }

    // debugging
    printk("printing psentinel info BEFORE initialization: \n");
    printk("psentinel -> id: %lu\n", psentinel -> id);
    printk("psentinel -> next: %p\n", psentinel -> next);
    printk("psentinel -> msg_len: %lu\n", psentinel -> msg_len);
    printk("psentinel -> msg: %s\n", psentinel -> msg);
    

    // Trying to allocate memory for a new ch_node. On error kmalloc returns NULL and sets errno
    pnew_channel = kmalloc(sizeof(ch_node), GFP_KERNEL);
    if(pnew_channel == NULL)
    {
        return -1;
    }
    // New ch_node successfully allocated - now filling in its attributes 
    pnew_channel -> id = channel_id;
    pnew_channel -> next = NULL;
    pnew_channel -> msg_len = 0;

    printk("printing psentinel info AFTER filling in attributes: \n");
    printk("psentinel == NULL: %i\n", psentinel == NULL);
    printk("psentinel -> id: %lu\n", psentinel -> id);
    printk("psentinel -> next: %p\n", psentinel -> next);
    printk("psentinel -> msg_len: %lu\n", psentinel -> msg_len);
    printk("psentinel -> msg: %s\n", psentinel -> msg);

    // Getting to last node of LL 
    while(pcurr -> next != NULL)
    {
        pcurr = pcurr -> next;
    }
    // Now pcurr is a pointer to the last ch_node in the LL
    pcurr -> next = pnew_channel;
    return SUCCESS;
}

void free_ch_node(ch_node* node)
{
    kfree(node);
}


void free_cell(ch_node* psentinel)
{
    // Recursive function to free all memory allocated to linked list
    ch_node* phead;
    ch_node* pprev;

    printk("Freeing linked list iteratively: \n");
    phead = psentinel;
    while(phead != NULL)
    {
        pprev = phead;
        phead = phead -> next;
        free_ch_node(pprev);
    }
    printk("finished freeing linked list\n");

}

void print_devices_array(void)
{
    int i;
    printk("printing non-null places in devices_array:   ");
    for(i = 0; i < 256; i++)
    {
        if(devices_array[i] != NULL)
        {
            printk("(%i) : %p", i, devices_array[i]);
        }   
    }
} 


//================== DEVICE FUNCTIONS IMPLEMENTATION ===========================
static int device_open(struct inode* inode, struct file* file)
{
    // Ensures that the sentinel this message_slot device is instantiated.
    // channel allocation happens in read/write. 
    int minor_num;
    ch_node* psentinel; // pointer to sentinel of LL

    printk("Hey there. Invoking device_open (%p,%p):\n", inode, file);
    // getting open file's minor:
    minor_num = iminor(inode);
    psentinel = devices_array[minor_num];
    printk("minor_num is %u\n", minor_num);
    if(psentinel == NULL)
    {
        printk("I am in device_open and psentinel is NULL\n");
        // this message_slot device was not yet opened. creating a sentinel ch_node with id = -1
        psentinel = kmalloc(sizeof(ch_node), GFP_KERNEL);
        if(psentinel == NULL)
        {
            printk(KERN_ERR "Failed to allocate memory for sentinel ch_node\n");
            return -ENOMEM;  
        }
        psentinel -> next = NULL;
        psentinel -> id = -1;
        // no need to instantiate message itself (it is a char array of length BUF_LEN)
        psentinel -> msg_len = -1;
        devices_array[minor_num] = psentinel;
        printk("devices_array[%i] is %p, and psentinel is %p", minor_num, devices_array[minor_num], psentinel);
    }
    else
    {
        printk("I am in device_open and psentinel is not NULL, rather it is: %p\n", psentinel);
    }
    print_devices_array();
    printk("Successfully invoked device_open(%p,%p). psentinel is %p\n", inode, file, psentinel);
    return SUCCESS;
}

static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset)
{
    int minor_num;
    unsigned long channel_id;
    ch_node* ptarget;
    ch_node* psentinel;
    int i;
    int status;

    printk("Invoked device_read (%p,%p,%zu)\n", file, buffer, length);
    // Getting minor of device that invoked this + the channel id to write to 
    minor_num = iminor(file_inode(file));
    channel_id = (unsigned long)file -> private_data;
    psentinel = devices_array[minor_num];

    printk("psentinel is: %p", psentinel);
    printk("doing get_channel_ptr\n");
    // Getting pointer to the ch_node corresponding to the channel_id.
    ptarget = get_channel_ptr(channel_id, psentinel);

    // Checking if a channel has been set on the file descriptor (i.e. if ioctl has already been called on it)
    if(ptarget == NULL) 
    {
        printk("No channel set on file descriptor\n");
        return -EINVAL;
    }

    // Check if message exists on channel
    printk("checking if message exists on channel: \n");
    if(ptarget -> msg_len == -1)
    {
        printk(KERN_ERR "No message exists on channel with id %lu \n", channel_id);
        return -EWOULDBLOCK;
    }
    printk("checking buufer size: ");
    // Check if provided buffer in user space is of sufficient size
    if(length > BUF_LEN)
    {
        printk("User bufer too small for message on channel with id %lu\n", channel_id);
        return -ENOSPC;
    }
    printk("buffer size is legal\n");

    // Reading message (i.e. putting it from channel to user buffer)
    printk("reading message from channel to user buffer\n");
    for(i = 0; i < ptarget -> msg_len; i++)
    {
        status = put_user(ptarget -> msg[i], &buffer[i]);
        if(status == -1)
        {
            printk(KERN_ERR "Failed to read message from channel\n");
            return -EFAULT;
        }
    }
    printk("number of bytes read is %zu\n", ptarget -> msg_len);
    return ptarget -> msg_len; // returning number of bytes read
}

// a processs which has already opened the device file attempts to write to it
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    int minor_num;
    unsigned long channel_id;
    ch_node* ptarget;
    ch_node* psentinel;
    int i;
    int status;

    printk("Invoked device_write (%p,%p,%zu)\n", file, buffer, length);
    print_devices_array();

    // Getting minor of device that invoked this + the channel id to write to 
    minor_num = iminor(file_inode(file));
    channel_id = (unsigned long)file -> private_data;
    psentinel = devices_array[minor_num];
    printk("minor is %i, channel id is %lu, devices_array[minor_num} is %p, psentintel is %p\n", minor_num, channel_id, devices_array[minor_num], psentinel);

    printk("doing get_channel_ptr\n");
    // Getting pointer to the ch_node corresponding to the channel_id.
    ptarget = get_channel_ptr(channel_id, psentinel);
    
    printk("checking if channel has been set on file descriptor\n");
    // Checking if a channel has been set on the file descriptor (i.e. if ioctl has already been called on it)
    if(ptarget == NULL) 
    {
        printk("ptarget is NULL! \n");
        printk("No channel set on file descriptor\n");
        return -EINVAL;
    }
    else
    {
    printk("ptarget is %p (not NULL) and the ch_node it points to has id of %lu\n", ptarget, ptarget -> id);
    }

    // Checking message length is legal
    if(length == 0 || length > BUF_LEN)
    {
        printk("Message length is illegal\n");
        return -EMSGSIZE;
    }
    // Do I need more error checking here?
    printk("message length is %zu", length);

    // No need for dynamic memory allocation for message since the msg attribute is set to be of size
    // BUF_LEN upon initiation
    printk("Now trying to write message from user buffer to channel: \n");
    for(i = 0; i < length; i++)
    {
        status = get_user(ptarget -> msg[i], &buffer[i]);
        if(status == -1)
        {
            printk(KERN_ERR "Failed to write message from user buffer to channel\n");
            return -EFAULT;
        }
    }
    ptarget -> msg_len = length;
    // Note that there is no need for overwriting previous messages since next read() will only read msg_len bytes. 
    printk("num of bytes written was %zu", ptarget -> msg_len);
    return length; // returning number of bytes written
}
 
static long device_ioctl(struct file* file, unsigned int ioctl_command, unsigned long channel_id)
{
    int status = 0;
    int minor_num;
    ch_node* psentinel;

    printk("Invoked device_ioctl (%p,%u,%ld)\n", file, ioctl_command, channel_id);
    print_devices_array(); 
    printk("MESSAGE_SLOT_CHANNEL: %lu", MESSAGE_SLOT_CHANNEL);


    // Validate input
    if(ioctl_command != MESSAGE_SLOT_CHANNEL)
    {
        printk(KERN_INFO "Invalid ioctl command: command was %lu and not MESSAGE_SLOT_CHANNEL\n", channel_id);
        return -EINVAL;
    }
    
    else if (channel_id == 0)
    {
        printk(KERN_INFO "Invalid channel id");
        return -EINVAL; 
    }
    printk("channel id is: %lu (non zero)\n", channel_id);

    // Store the channel id to be associated with the message_slot device that invoked the ioctl:
    file -> private_data = (void*)channel_id; 
    // Check if the channel is in the linked list, if not create a ch_node for it
    minor_num = iminor(file_inode(file)); // get minor number of the message_slot device that invoked the ioctl (iminor returns an unsigned int)
    psentinel = devices_array[minor_num];
    printk("minor is %u\n", minor_num);
    printk("devices_array[%i] is %p. This should be psentinel which is %p\n", minor_num, devices_array[minor_num], psentinel);
    print_devices_array(); 
    printk("Now trying to do get_channel_ptr:\n");
    if(psentinel == NULL)
    {
        printk("psentinel is NULL!\n");
        return -1;
    }
    if(get_channel_ptr(channel_id, psentinel) == NULL)
    {
        printk("get_channel_ptr returned NULL. Trying to create new channel\n");
        status = create_and_append(channel_id, psentinel);
        if(status == -1)
        {
            printk(KERN_ERR "Failed to allocate memory for new channel\n");
            return -1;
        }
    }
    // If we got here, new channel was successfully added!
    return SUCCESS;
}

static int device_release(struct inode* inode, struct file* file)
{
    // NOTE: this method frees memory associated with a specific message slot device file, i.e.
    // a single cell in devices_array
    // returns 0 on success
    // unsigned long minor_num;
    // ch_node* psentinel;

    // minor_num = iminor(inode);
    // psentinel = devices_array[minor_num];
    // free_cell(psentinel);
    printk("Invoking device_release \n");
    return SUCCESS;
}


//==================== DEVICE SETUP =============================
struct file_operations fops = {
  .owner = THIS_MODULE, 
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .unlocked_ioctl = device_ioctl,
  .release = device_release
};

// Initialize the module - Register the character device
static int __init message_slot_init(void)
{
    int rc = -1;
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &fops);
    if(rc < 0)
    {
        printk(KERN_ALERT "%s registration failed for %d\n", DEVICE_FILE_NAME, MAJOR_NUM);
        return rc;
    }
    printk("Successful registration!");
    printk("To talk to the device driver, you must create a device file:\n" );
    printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
    printk("You can echo/cat to/from the device file.\n");
    printk("Don't forget to rm the device file and ""rmmod when you're done\n");
    return 0;
}

static void __exit message_slot_cleanup(void)
{
    int i;
    ch_node* psentinel;

    // Unregistering the device
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);

    // Each cell in devices_array is freed by device_release when that file is closed,
    // here just making sure
    for(i = 0; i < BUF_LEN; i++)
    {
        psentinel = devices_array[i];
        if(psentinel != NULL)
        {
            free_cell(psentinel);
        }
    }
}


module_init(message_slot_init);
module_exit(message_slot_cleanup);

