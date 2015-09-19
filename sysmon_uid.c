#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "globals.h"


static int
sysmon_uid_show(struct seq_file *m, void *v)
{
    seq_printf(m, "%d\n", uid);
    return 0;
}

static int
sysmon_uid_open(struct inode *inode, struct file *file)
{
    return single_open(file, sysmon_uid_show, NULL);
}

static const struct file_operations sysmon_uid_fops = {
    .owner      = THIS_MODULE,
    .open       = sysmon_uid_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};

static int __init
sysmon_uid_init(void)
{
    printk(KERN_INFO "Loading sysmon_uid module.\n");
    /* 0600 grants the proc file read/write privileges */
    proc_create("sysmon_uid", 0600, NULL, &sysmon_uid_fops);
    return 0;
}

static void __exit
sysmon_uid_exit(void)
{
    remove_proc_entry("sysmon_uid", NULL);
    printk(KERN_INFO "Unloading sysmon_uid module.\n");
}

module_init(sysmon_uid_init);
module_exit(sysmon_uid_exit);

MODULE_LICENSE("GPL");

