#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   
#include <linux/module.h>   
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include "message_slot.h"

MODULE_LICENSE("GPL");

static ch_node* devices_array [256] = {NULL}; // ith cell represents head of LL of that message slot's channels. 
// so each cell contains either NULL or the head of a LL of channel_nodes. 

//================== HELPER FUNCTIONS FOR CH_NODE LINKED LISTS ===========================
// search for channel_id in a LL of ch_nodes
// append ch_node to LL (receives head of that LL)
// free channel (that specific channel only? shouldn't there be something recursive here since it
// is a LL?)

static int device_open(struct inode* inode, struct file* file);
static ssize_t device_read(struct file* file, char __user* buffer,size_t length, loff_t* offset);
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset);
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param);

//================== DEVICE FUNCTIONS ===========================
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
 
static long device_ioctl(struct file* file, unsigned int ioctl_command, unsigned long ioctl_param)
{
    switch(ioctl_command) 
    {
        case MESSAGE_SLOT_CHANNEL:
            printk(KERN_INFO "message slot: ioctl command MESSAGE_SLOT_CHANNEL received with param: %lu\n", ioctl_param);
            // Handle the command here, e.g., set the channel number
            break;
        default:
            return -EINVAL; // Invalid command
    }
    return 0;
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

