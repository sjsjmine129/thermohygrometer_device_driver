#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
 
int main() {
    int dev;
    char buff[1024];
 
    dev = open("/dev/AAAA", O_RDWR);
    printf("dev = %d\n", dev);
 
    int ret = read(dev, buff, 4);
    if(ret>0)
    {
        printf("1. read: %d %s\n", ret, buff);
    }

    ret = write(dev, "B", 3);
    printf("write: %d \n", ret);
    
    memset(buff, '\0', sizeof(buff)); //reset first

    ret = read(dev, buff, 3);
    if(ret>0)
    {
        printf("2. read: %d %s\n", ret, buff);
    }

    close(dev);
 
    exit(EXIT_SUCCESS);
}