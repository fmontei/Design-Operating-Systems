#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>

static void write_file(char *filename, char *data)
{
  struct file *file;
  loff_t pos = 0;
  int fd;

  mm_segment_t old_fs = get_fs();
  set_fs(KERNEL_DS);

  file = filp_open(filename, O_WRONLY|O_CREAT, 0644);
  if (file) {
    //sys_write(fd, data, strlen(data));
    //file = fget(fd);
    //if (file) {
      vfs_write(file, data, strlen(data), &pos);
      fput(file);
    //}
    //sys_close(fd);
  }
  set_fs(old_fs);
}

static int __init my_init(void)
{
  write_file("/root/log.txt", "Evil file.\n");
  return 0;
}

static void __exit my_exit(void)
{ }

MODULE_LICENSE("GPL");
module_init(my_init);
module_exit(my_exit);
