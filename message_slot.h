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

#endif
