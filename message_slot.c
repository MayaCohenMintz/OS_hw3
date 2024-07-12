#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   
#include <linux/module.h>   
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
# include <linux/slab.h>
#include "message_slot.h"

MODULE_LICENSE("GPL");

static ch_node* devices_array [256] = {NULL}; // ith cell represents head of LL of that message slot's channels. 
// so each cell contains either NULL or the head of a LL of channel_nodes. 


ch_node* get_channel_ptr(unsigned long channel_id, ch_node* psentinel); // returns pointer to the channel, NULL if no channel with that
// channel id exists
int create_and_append(unsigned long channel_id, ch_node* psentinel); // create new ch_node with the channel_id and
// append to end of LL. Returns 0 on success and -1 on failure

// free channel (that specific channel only? shouldn't there be something recursive here since it
// is a LL?)

static int device_open(struct inode* inode, struct file* file);
static ssize_t device_read(struct file* file, char __user* buffer,size_t length, loff_t* offset);
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset);
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param);

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
        pcurr = pcurr.next;
    }
    return NULL;
}

int create_and_append(unsigned long channel_id, ch_node* psentinel)
{
    ch_node new_channel;
    ch_node* pcurr = psentinel;

    // Trying to allocate memory for a new ch_node. On error kmalloc returns NULL and sets errno
    &new_channel = kmalloc(sizeof(ch_node), GFP_KERNEL);
    if(&new_channel == NULL)
    {
        return -1;
    }
    // New ch_node successfully allocated - now filling in its attributes 
    new_channel.id = channel_id;
    new_channel.next = NULL;
    new_channel.msg_len = 0;

    // Getting to last node of LL 
    while(pcurr.next != NULL)
    {
        pcurr = pcurr.next;
    }
    // Now pcurr is a pointer to the last ch_node in the LL
    pcurr.next = &new_channel;
    return SUCCESS;
}



//================== DEVICE FUNCTIONS IMPLEMENTATION ===========================
static int device_open(struct inode* inode, struct file* file)
{
    // Ensures that the ch_node pointer of this message_slot device is instantiated.
    // channel addition happens in device_ioctl
    int minor_num;
    ch_node* psentinel; // pointer to sentinel of LL

    printk("Invoked device_open (%p,%p)\n", inode, file);
    // getting open file's minor:
    minor_num = iminor(inode);
    psentinel = devices_array[minor_num];
    if(psentinel == NULL)
    {
        // this message_slot device was not yet opened. creating a sentinel ch_node with id = -1
        ch_node sentinel;
        sentinel.next = NULL;
        sentinel.id = -1;
        sentinel.msg = "sentinel";
        sentinel.msg_len = -1;
        // updating the pointer in devices_array[minor_number]
        psentinel = &psentinel; 
    }
    printk("Successfully invoked device_open(%p,%p)\n", inode, file);
    return SUCCESS;
}

// a process which has already opened the device file attempts to read from it
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

    // Getting pointer to the ch_node corresponding to the channel_id.
    ptarget = get_channel_ptr(channel_id, psentinel);

    // Checking if a channel has been set on the file descriptor (i.e. if ioctl has already been called on it)
    if(ptarget == NULL) 
    {
        printk("No channel set on file descriptor\n");
        return -EINVAL;
    }

    // Check if message exists on channel
    if(ptarget -> msg_len == -1)
    {
        printk(KERN_ERR "No message exists on channel with id %lu \n", channel_id);
        return -EWOULDBLOCK;
    }

    // Check if provided buffer in user space is of sufficient size
    if(sizeof(buffer) < length)
    {
        printk("User bufer too small for message on channel with id %lu\n", channel_id);
        return -ENOSPC;
    }

    // Reading message. No need for dynamic memory allocation for message since the msg attribute is set 
    // to be of size BUF_LEN upon initiation
    for(i = 0; i < length; i++)
    {
        status = get_user(&buffer[i], ptarget -> msg[i])
        if(status == -1)
        {
            printk(KERN_ERR "Failed to write message from channel to user buffer\n");
            return -EFAULT;
        }
    }
    return SUCCESS;
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

    // Getting minor of device that invoked this + the channel id to write to 
    minor_num = iminor(file_inode(file));
    channel_id = (unsigned long)file -> private_data;
    psentinel = devices_array[minor_num];

    // Getting pointer to the ch_node corresponding to the channel_id.
    ptarget = get_channel_ptr(channel_id, psentinel);

    // Checking if a channel has been set on the file descriptor (i.e. if ioctl has already been called on it)
    if(ptarget == NULL) 
    {
        printk("No channel set on file descriptor\n");
        return -EINVAL;
    }

    // Checking message length is legal
    if(length == 0 || length > BUF_LEN)
    {
        printk("Message length is illegal\n");
        return -EMSGSIZE;
    }
    // Do I need more error checking here?

    // No need for dynamic memory allocation for message since the msg attribute is set to be of size
    // BUF_LEN upon initiation
    for(i = 0; i < length; i++)
    {
        status = get_user(ptarget -> msg[i], &buffer[i])
        if(status == -1)
        {
            printk(KERN_ERR "Failed to write message from user buffer to channel\n");
            return -EFAULT;
        }
    }
    ptarget -> msg_len = length;
    // Note that there is no need for overwriting previous messages since next read() will only read msg_len bytes. 
    return SUCCESS;
}
 
static long device_ioctl(struct file* file, unsigned int ioctl_command, unsigned long channel_id)
{
    int status = 0;
    int minor_num;
    ch_node* psentinel;

    printk("Invoked device_ioctl (%p,%u,%ld)\n", file, ioctl_command, channel_id);


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

    // Store the channel id to be associated with the message_slot device that invoked the ioctl:
    file -> private_data = (void*)channel_id; 
    // Check if the channel is in the linked list, if not create a ch_node for it
    minor_num = iminor(file_inode(file)); // get minor number of the message_slot device that invoked the ioctl
    psentinel = devices_array[minor_num];
    if(get_channel_ptr(channel_id, psentinel) == NULL)
    {
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

//==================== DEVICE SETUP =============================
struct file_operations fops = {
  .owner = THIS_MODULE, 
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .ioctl = device_ioctl
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
    // Implement last to make sure all memory is freed! 
    // Check if it may be better to free memory in other places too.
    return 0;
}


module_init(message_slot_init);
module_exit(message_slot_cleanup);

