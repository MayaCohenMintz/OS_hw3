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

    printf("num of args is argc = %i\n", argc);
    
    // Validate that 3 command line arguments were passed
    if(argc != 4) // argv[0] is name of program
    {
        perror("Number of arguments was not 3\n");
        return -1;
    }

    printf("argv[0] is: %s\n", argv[0]);

    file_path = argv[1];
    channel_id = (unsigned long)argv[2];
    message = argv[3];

    // debugging
    printf("argv[1] should be filepath and is actually: %s\n", argv[1]);
    printf("argv[2] should be channel id and is actually: %lu\n", (unsigned long)argv[2]);
    printf("argv[3] should be message and is actually: %s\n", argv[3]);


    // 1. Open specified message slot device file
    msg_slot_fd = open(file_path, O_RDWR);
    printf("msg_slot_fd: %i\n", msg_slot_fd);
    if(msg_slot_fd == -1)
    {
        perror("Error in opening device file");
        exit(1);
    }

    printf("Now setting channel id to %lu : \n", channel_id);
    // 2. Set the channel id to the id specified on the command line
    // printf("ioctl gives: %i \n", ioctl(msg_slot_fd, MESSAGE_SLOT_CHANNEL, channel_id));
    if (ioctl(msg_slot_fd, MESSAGE_SLOT_CHANNEL, channel_id) == -1) 
    {
        perror("Error in setting channel\n");
        exit(1); 
    }
    printf("Channel set successfully with id %lu\n", channel_id);

    // 3. Write the specified message to the message slot file. Don’t include the terminating null 
    // character of the C string as part of the message.
    num_bytes_written = write(msg_slot_fd, message, strlen(message)); // make sure that sizeof should be used and not strlength(message)
    if(num_bytes_written != sizeof(message))
    {
        perror("Error in writing message to channel");
        exit(1);
    }

    // 4. Close the device.
    close(msg_slot_fd);
    // 5. Exit the program with exit value 0.
    exit(0);
}