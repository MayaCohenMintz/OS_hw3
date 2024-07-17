#include <fcntl.h>     
#include <unistd.h>     
#include <sys/ioctl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message_slot.h"

int main(int argc, char* argv[])
{
    char* file_path;
    unsigned long channel_id;
    char* message;
    int msg_slot_fd;
    int num_bytes_written;
    
    // Validate that 3 command line arguments were passed
    if(argc != 4) // argv[0] is name of program
    {
        perror("message_sender: Invalid argument: Number of arguments was not 3\n");
        return -1;
    }

    file_path = argv[1];
    channel_id = (unsigned long)atoi(argv[2]);
    message = argv[3];

    printf("message_sender: argv[2] is %s, channel id is %lu. Are they the same?\n", argv[2], channel_id);
    printf("message_sender: file_path is %s\n", file_path);

    // 1. Open specified message slot device file
    printf("message_sender: Now trying to invoke open\n");
    msg_slot_fd = open(file_path, O_RDWR);
    printf("message_sender: msg_slot_fd: %i\n", msg_slot_fd);
    if(msg_slot_fd == -1)
    {
        perror("message_sender: Error in opening device file");
        exit(1);
    }

    printf("message_sender: Now setting channel id to %lu : \n", channel_id);
    // 2. Set the channel id to the id specified on the command line
    // printf("ioctl gives: %i \n", ioctl(msg_slot_fd, MESSAGE_SLOT_CHANNEL, channel_id));
    if (ioctl(msg_slot_fd, MSG_SLOT_CHANNEL, channel_id) == -1) 
    {
        perror("message_sender: Error in setting channel\n");
        exit(1); 
    }
    printf("message_sender: Channel set successfully with id %lu\n", channel_id);

    // 3. Write the specified message to the message slot file. Don’t include the terminating null 
    // character of the C string as part of the message.
    printf("message_sender: now trying to write message into message_slot_fd: ");
    num_bytes_written = write(msg_slot_fd, message, strlen(message)); // make sure that sizeof should be used and not strlength(message)
    printf("message_sender: num bytes written are %i\n", num_bytes_written);
    if(num_bytes_written != strlen(message))
    {
        perror("message_sender: Error in writing message to channel");
        exit(1);
    }

    printf("message_sender: closing the device\n");
    // 4. Close the device.
    close(msg_slot_fd);
    // 5. Exit the program with exit value 0.
    exit(0);
}