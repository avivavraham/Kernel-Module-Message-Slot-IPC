#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "message_slot.h"


int main(int argc, char* argv[]){
    if(argc != 4){
        perror("error, expected 3 arguments to sender main function");
        exit(1);
    }
    int to = open(argv[1],O_WRONLY);
    if (to < 0){
        perror("error, cannot open file");
        exit(1);
    }
    int result = ioctl(to, MSG_SLOT_CHANNEL, atoi(argv[2]));
    if(result != 0){
        perror("error, ioctl failed");
        exit(1);}
    int length = strlen(argv[3]);
    result = write(to,argv[3],length);
    if(result != length){
        perror("error, write to file failed");
        exit(1);
    }
    close(to);
    return 0;
}