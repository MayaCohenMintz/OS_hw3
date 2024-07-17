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
        perror("message_reader: Invalid argument - Number of arguments was not 2\n");
        return -1;
    }

    file_path = argv[1];
    channel_id = (unsigned long)atoi(argv[2]);

    //printf("message_reader: argv[2] is %s, channel id is %lu. Are they the same?\n", argv[2], channel_id);

    // 1. Open specified message slot device file
    //printf("message_reader: Now trying to open file:\n");
    msg_slot_fd = open(file_path, O_RDWR);
    //printf("message_reader: msg_slot_fd: %i\n", msg_slot_fd);
    if(msg_slot_fd < 0)
    {
        perror("Error in opening device file\n");
        exit(1);
    }

    // 2. Set the channel id to the id specified on the command line
    //printf("message_reader: trying to invoke ioctl on msg_slot_fd = %i and channel id = %lu\n", msg_slot_fd, channel_id);
    if (ioctl(msg_slot_fd, MSG_SLOT_CHANNEL, channel_id) == -1) 
    {
        perror("message_reader: Error in setting channel\n");
        exit(1); 
    }
    //printf("message_reader: Channel set successfully with id %lu\n", channel_id);
    // 3. Read a message from the message slot file to a buffer. 
    // (reading up to BUF_LEN bytes, so no need to know length of actual message)
    //printf("message_reader: Trying to read message from buffer: \n");
    num_bytes_read = read(msg_slot_fd, buffer, BUF_LEN);
    //printf("num of bytes read is %i\n", num_bytes_read);
    if(num_bytes_read == -1)
    {
        perror("message_reader: Error in reading from channel\n");
        exit(1);
    }
    
    // 4. Close the device.
    //printf("message_reader: closing device\n");
    close(msg_slot_fd);
    // 5. Print the message to standard output (using the write() system call). Print only the message,
    // without any additional text
    //printf("message_reader: printing to std outpud: \n");
    num_bytes_written = write(STDOUT_FILENO, buffer, num_bytes_read);
    //printf("\nnum of bytes written to output is %i", num_bytes_written);
    if(num_bytes_written == -1)
    {
        perror("message_reader: Error in writing message to std output\n");
        exit(1);
    }
    // 6. Exit the program with exit value 0.
    exit(0);
}
