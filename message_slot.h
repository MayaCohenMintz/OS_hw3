#ifndef MSG_SLOT_H
#define MSG_SLOT_H

#include <linux/ioctl.h>

#define MAJOR_NUM 235
#define MESSAGE_SLOT_CHANNEL (_IOW(MAJOR_NUM, 0, unsigned long)) 
#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128 
#define DEVICE_FILE_NAME "message_slot_device"
#define SUCCESS 0
#define FAILURE 1

typedef struct channel_node
{
    struct channel_node next;
    size_t msg_len; // actual length of message - bounded by 128 bytes
    char msg[BUF_LEN]; // the message. Note that the size is already set at instantiation (it is O(1))
    unsigned long id; // this not the message_slot minor, so is not bounded in size
} ch_node ; 

#endif
