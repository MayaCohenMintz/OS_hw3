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
    int msg_slot_fd;
    char buffer[BUF_LEN] = {0}; // initialized to all 0's
    int num_bytes_read; 
    int num_bytes_written; 

    // Validate that 2 command line arguments were passed
    if(argc != 3) // argv[0] is name of program
    { 
        perror("Number of arguments was not 2\n");
        return -1;
    }

    file_path = argv[1];
    channel_id = (unsigned long)argv[2];

    // debugging
    printf("argv[1] should be filepath and is actually: %s\n", argv[1]);
    printf("argv[2] should be channel id and is actually: %lu\n", (unsigned long)argv[2]);
 
    // 1. Open specified message slot device file
    msg_slot_fd = open(file_path, O_RDWR);
    printf("msg_slot_fd: %i\n", msg_slot_fd);
    if(msg_slot_fd < 0)
    {
        perror("Error in opening device file\n");
        exit(1);
    }

    // 2. Set the channel id to the id specified on the command line
    if (ioctl(msg_slot_fd, MESSAGE_SLOT_CHANNEL, channel_id) == -1) 
    {
        perror("Error in setting channel\n");
        exit(1); 
    }
    printf("Channel set successfully with id %lu\n", channel_id);
    // 3. Read a message from the message slot file to a buffer. 
    // (reading up to BUF_LEN bytes, so no need to know length of actual message)
    num_bytes_read = read(msg_slot_fd, buffer, BUF_LEN);
    if(num_bytes_read == -1)
    {
        perror("Error in reading from channel\n");
        exit(1);
    }
    
    // 4. Close the device.
    close(msg_slot_fd);
    // 5. Print the message to standard output (using the write() system call). Print only the message,
    // without any additional text.
    num_bytes_written = write(STDOUT_FILENO, buffer, num_bytes_read);
    if(num_bytes_written == -1)
    {
        perror("Error in writing message to std output\n");
        exit(1);
    }
    // 6. Exit the program with exit value 0.
    exit(0);
}
