#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

#include "message_slot.h"

int main(int argc, char* argv)
{
    char* file_path;
    unsigned long channel_id;
    char* message;
    int msg_slot_fd;
    int ret_val;
    char buffer[BUF_LEN];
    int msg_len;
    int write_status;
    
    // Validate that 3 command line arguments were passed
    if(argc != 3)
    {
        printf("Number of args passed was not 3 - returning -1\n");
        return -1;
    }

    argv[1] = file_path;
    argv[2] = channel_id;
    argv[3] = message;

    // 1. Open specified message slot device file
    msg_slot_fd = open(file_path, O_RDWR);
    if(msg_slot_fd < 0)
    {
        perror("Error in opening device file");
        exit(1);
    }

    // 2. Set the channel id to the id specified on the command line
    if (ioctl(msg_slot_fd, MESSAGE_SLOT_CHANNEL, channel_id) == -1) 
    {
        perror("Error in setting channel");
        exit(1); 
    }
    printf("Channel set successfully with id %lu\n", channel_id);

    // 3. Write the specified message to the message slot file. Don’t include the terminating null 
    // character of the C string as part of the message.
    // how to not include the terminating null character??
    write_status = write(msg_slot_fd, buffer, sizeof(message));
    if(write_status == -1)
    {
        perror("Error in writing message to channel");
        exit(1);
    }

    // 4. Close the device.
    close(msg_slot_fd);
    // 5. Exit the program with exit value 0.
    exit(0);
}