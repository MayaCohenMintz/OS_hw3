#include <fcntl.h>      
#include <unistd.h>     
#include <sys/ioctl.h>  
#include <stdio.h>
#include <stdlib.h>

#include "message_slot.h"

int main(int argc, char* argv[])
{
    char* file_path;
    unsigned long channel_id;
    char* message;
    int msg_slot_fd;
    char buffer[BUF_LEN] = {0}; // initialized to all 0's
    int num_bytes_read; 
    int num_bytes_written; 

    // Validate that 2 command line arguments were passed
    if(argc != 2)
    {
        perror("Number of arguments was not 2\n");
        return -1;
    }

    argv[1] = file_path;
    argv[2] = channel_id;
 
    // 1. Open specified message slot device file
    msg_slot_fd = open(file_path, O_RDWR);
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
