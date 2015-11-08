#include <fcntl.h>         /* defines options flags */
#include <sys/types.h>     /* defines types used by sys/stat.h */
#include <sys/stat.h>      /* defines S_IREAD & S_IWRITE  */

static char message[] = "Something";

int main()
{
int fd;
char buffer[80];

fd = open("test.dat",O_RDWR | O_CREAT | O_EXCL, S_IREAD | S_IWRITE);
if (fd != -1)
   {
   //printf("test.dat opened for read/write access\n");
   int i=0;
   for (;i<100000000;i++){
   	write(fd, message, sizeof(message));
   }
   //lseek(fd, 0L, 0);     /* go back to the beginning of the file */
   close (fd);
   }
else
   printf("*** test.dat already exists ***\n");
exit (0);
}
