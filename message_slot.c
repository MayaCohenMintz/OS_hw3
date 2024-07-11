#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#define DEVICE_RANGE_NAME "char_dev"
#define BUF_LEN 80
#define DEVICE_FILE_NAME "message_slot_dev"
#define SUCCESS 0

#include <linux/kernel.h>   
#include <linux/module.h>   
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/

MODULE_LICENSE("GPL");

static int device_open(struct inode* inode, struct file* file);
static ssize_t device_read(struct file* file, char __user* buffer,size_t length, loff_t* offset);
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset);
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param);

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode* inode, struct file* file)
{
    return 0;
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

