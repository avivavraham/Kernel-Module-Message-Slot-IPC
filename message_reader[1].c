#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include "message_slot.h"

int main(int argc, char* argv[]){
    if(argc != 3){
        perror("error, expected 2 arguments to reader main function");
        exit(1);
    }
    int to = open(argv[1],O_RDONLY);
    if (to < 0){
        perror("error, cannot open file");
        exit(1);
    }
    int result = ioctl(to, MSG_SLOT_CHANNEL, atoi(argv[2]));
    if(result != 0){
        perror("error, ioctl failed");
        exit(1);}
    char buffer[BUFFER_LENGTH];
    result = read(to,buffer,BUFFER_LENGTH);
    if(result < 0){
        perror("error, read to file failed");
        exit(1);
    }
    if(write(STDOUT_FILENO, buffer,result) == -1){
        perror("error, write to standard output failed");
        exit(1);
    }
    close(to);
    return 0;
}