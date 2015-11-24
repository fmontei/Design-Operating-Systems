#include <string.h>
#include <unistd.h>
#include <stdio.h>

int main(void)
{
    char *link = "/home/osboxes/Desktop/hello.mp3";
    char buf[1024];
    int retval = readlink("/hello.mp3", link, sizeof(buf));
    printf("retval = %d\n", retval);

    retval = readlink("hello.mp3", buf, sizeof(buf));
    printf("retval = %d\n", retval);

    retval = readlink("/hello.mp3", buf, sizeof(buf) + 1);
    printf("retval = %d\n", retval);

    retval = readlink("hello.mp3", buf, sizeof(buf) + 1);
    printf("retval = %d\n", retval);

    retval = readlink("/hello.mp3", buf, sizeof(buf) - 1);
    printf("retval = %d\n", retval);

    retval = readlink("hello.mp3", buf, sizeof(buf) - 1);
    printf("retval = %d\n", retval);

    return 0;
}
