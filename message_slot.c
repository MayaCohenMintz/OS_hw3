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
#include "errno.h"

MODULE_LICENSE("GPL");

static ch_node* devices_array [256] = {NULL}; // ith cell represents head of LL of that message slot's channels. 
// so each cell contains either NULL or the head of a LL of channel_nodes. 


ch_node* get_channel_ptr(unsigned long channel_id, ch_node* phead); // returns pointer to the channel 
int create_and_append(unsigned long channel_id, ch_node* phead); // create new ch_node with the channel_id and
// append to end of LL. Returns 0 on success and -1 on failure

// append ch_node to LL (receives head of that LL)
// free channel (that specific channel only? shouldn't there be something recursive here since it
// is a LL?)

static int device_open(struct inode* inode, struct file* file);
static ssize_t device_read(struct file* file, char __user* buffer,size_t length, loff_t* offset);
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset);
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param);

//================== HELPER FUNCTIONS IMPLEMENTATION ===========================

ch_node* get_channel_ptr(unsigned long channel_id, ch_node* phead)
{
    ch_node* pcurr = phead;
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

int create_and_append(unsigned long channel_id, ch_node* phead)
{
    ch_node new_channel;
    ch_node* pcurr = phead;

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
    new_channel.msg = "";

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
    // ensures that the ch_node pointer of this message_slot device is instantiated.
    // channel addition happens in device_ioctl
    int minor_num;
    ch_node* phead; // pointer to head of LL
    // getting open file's minor:
    minor_num = iminor(inode);
    phead = devices_array[minor_num];
    if(phead == NULL)
    {
        // this message_slot device was not yet opened. creating a dummy head ch_node with id = -1
        // (will be filled in properly in subsequent stages)
        ch_node head;
        head.next = NULL;
        head.id = -1;
        head.msg = "";
        head.msg_len = 0;
        phead = &head; // updating the pointer in devices_array[minor_number]
    }
    printk("Successfully invoked device_open(%p,%p)\n", inode, file);
    return SUCCESS;
}

// a process which has already opened the device file attempts to read from it
static ssize_t device_read(struct file* file, char __user* buffer,size_t length, loff_t* offset)
{
    return 0;
}

// a processs which has already opened the device file attempts to write to it
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
    return 0;
}
 
static long device_ioctl(struct file* file, unsigned int ioctl_command, unsigned long channel_id)
{
    int status = 0;
    int minor_num;
    ch_node* phead;

    // Validate input
    if(ioctl_command != MESSAGE_SLOT_CHANNEL)
    {
        printk(KERN_INFO "Invalid ioctl command: command was %lu and not MESSAGE_SLOT_CHANNEL\n", ioctl_param);
        errno = EINVAL; 
        return -1;
    }
    else if (channel_id == 0)
    {
        printk(KERN_INFO "Invalid channel id");
        errno = EINVAL; 
        return -1;
    }

    // Store the channel id to be associated with the message_slot device that invoked the ioctl:
    file -> private_data = (void*)channel_id; 
    // Check if the channel is in the linked list, if not create a ch_node for it
    minor_num = iminor(file_inode(file)); // get minor number of the message_slot device that invoked the ioctl
    phead = devices_array[minor_num];
    if(get_channel_ptr(channel_id, phead) == NULL)
    {
        status = create_and_append(channel_id, phead);
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
struct file_operations Fops = {
  .owner = THIS_MODULE, 
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .ioctl = device_ioctl
};

// Initialize the module - Register the character device
static int __init message_slot_init(void)
{
    return 0;
}

static void __exit message_slot_cleanup(void)
{
    return 0;
}


module_init(message_slot_init);
module_exit(message_slot_cleanup);

