/**
 *  procfs4.c -  create a "file" in /proc
 * 	This program uses the seq_file library to manage the /proc file.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/jiffies.h> // for jiffies

#include <linux/delay.h> /** for msleep in linux kernel **/

#define PROC_NAME "sysmon_uid"

static int value = 1;

static int show(struct seq_file *m, void *v)
{
    seq_printf(m, "%d\n", value);
    return 0;
}

static int open(struct inode *inode, struct file *file)
{
    return single_open(file, show, NULL);
}

static ssize_t
write(struct file *file, const char * buf, size_t size, loff_t * ppos)
{
    value = 500;
    printk(KERN_INFO "Write = %d.\n", value);
    return value;
}

/**
 * This structure gather "function" that manage the /proc file
 *
 */
static const struct file_operations my_file_ops = {
	.owner   = THIS_MODULE,
	.open    = open,
    .write   = write,
	.read    = seq_read,
    .llseek = seq_lseek,
	.release = seq_release,
};
	
	
/**
 * This function is called when the module is loaded
 *
 */
static int __init
sysmon_uid_init(void)
{
	proc_create("sysmon_uid", 0600, NULL, &my_file_ops);
	
	return 0;
}

/**
 * This function is called when the module is unloaded.
 *
 */
static void __exit
sysmon_uid_exit(void)
{
    remove_proc_entry("sysmon_uid", NULL);
    printk(KERN_INFO "Unloading sysmon_uid module.\n");
}

module_init(sysmon_uid_init);
module_exit(sysmon_uid_exit);

MODULE_LICENSE("GPL");

