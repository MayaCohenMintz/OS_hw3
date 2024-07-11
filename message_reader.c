#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv)
{
    char* file_path;
    int channel_id;
    char* message;
    int msg_slot_fd;
    int ret_val;

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
    }

    // 2. Set the channel id to the id specified on the command line
    // 3. Write the specified message to the message slot file. Don’t include the terminating null character
    // of the C string as part of the message.
    
    // 4. Close the device.
    close(msg_slot_fd);
    // 5. Exit the program with exit value 0.
    exit(0);
    return 0; // do I need this if I already have exit(0)?
}
