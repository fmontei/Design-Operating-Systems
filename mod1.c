#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <asm-generic/uaccess.h>

MODULE_AUTHOR("Felipe"); 
MODULE_DESCRIPTION("KPROBE MODULE"); 
MODULE_LICENSE("GPL");

#define NUM_KPROBES 30
#define SYSMON_LOG "sysmon_log"
#define SYSMON_UID "sysmon_uid"
#define SYSMON_TOGGLE "sysmon_toggle"

static struct kprobe kprobes[NUM_KPROBES];
static const char *symbol_names[NUM_KPROBES] = {
    "sys_access", "sys_brk", "sys_chdir", 
    "sys_chmod", "sys_clone", "sys_close", "sys_dup", "sys_dup2",
    "sys_execve", "sys_exit_group", "sys_fcntl", "sys_fork", "sys_getdents",
    "sys_getpid", "sys_gettid", "sys_ioctl", "sys_lseek", "sys_mkdir",
    "sys_mmap", "sys_munmap", "sys_open", "sys_pipe", "sys_read",
    "sys_rmdir", "sys_select", "sys_stat", "sys_fstat", "sys_lstat",
    "sys_wait4", "sys_write"
};

#define LOG_MAX_LENGTH 1024 // Max length of Log
static char *log;  // Log character array
static char *temp; // Temporary log character array

static void substring(char s[], char sub[], int p, int l);

static void add_entry_to_log(char *entry)
{
    if (strlen(log) + strlen(entry) < LOG_MAX_LENGTH)
    {
        strcat(log, entry);
    }
    else if (strlen(log) + strlen(entry) >= LOG_MAX_LENGTH)
    {
        char token = '\n';
        int i = 0; 
        while (token != log[i])
        {
            i++;
        }
        i += 2; // Include \n character
        
        temp = (char *) kmalloc(sizeof(char) * LOG_MAX_LENGTH, __GFP_REPEAT);
        substring(log, temp, i, LOG_MAX_LENGTH - i);
        
        kfree(log);
        log = temp;
        
        if (strlen(log) + strlen(entry) < LOG_MAX_LENGTH)
            strcat(log, entry);
        else if (strlen(log) + strlen(entry) >= LOG_MAX_LENGTH)
            add_entry_to_log(entry);
    }
}

static void substring(char s[], char sub[], int p, int l) {
   int c = 0;
 
   while (c < l) {
      sub[c] = s[p + c - 1];
      c++;
   }
}

/* Sysmon TOGGLE Code: reference is www.pageframes.com/?p=89 */
static int sysmon_toggle_len_check = 1;
static unsigned int on_or_off = 1;

static int sysmon_toggle_open(struct inode * sp_inode, struct file *sp_file)
{
    return 0;
}

static int sysmon_toggle_release(struct inode *sp_indoe, struct file *sp_file)
{
    return 0;
}

static int sysmon_toggle_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset)
{
    if (sysmon_toggle_len_check) 
    {
        sysmon_toggle_len_check = 0;
        copy_to_user(buf, log, strlen(log));
        return strlen(log);
    }
    else 
    {
        sysmon_toggle_len_check = 1;
        return 0;
    }
}

static int sysmon_toggle_write(struct file *sp_file, const char __user *buf, size_t size, loff_t *offset)
{
    if (size == 2) // 2 includes the number and \0
    {
        const char number = buf[0];
        if (number == '0') 
        {
            on_or_off = 0;
            printk(KERN_INFO "Sysmon log disabled.\n");
            return size;
        }
        else if (number == '1') 
        {
            on_or_off = 1;
            printk(KERN_INFO "Sysmon log enabled.\n");
            return size;
        }
    }
    return -EINVAL;
}

static const struct file_operations sysmon_toggle_fops = {
    .open = sysmon_toggle_open,
    .read = sysmon_toggle_read,
    .write = sysmon_toggle_write,
    .release = sysmon_toggle_release
};

/* Sysmon Log Code */
static int show(struct seq_file *m, void *v)
{
    seq_printf(m, "%s\n", log);
    return 0;
}

static int open(struct inode *inode, struct file *file)
{
    return single_open(file, show, NULL);
}

static const struct file_operations sysmon_log_fops = {
    .owner   = THIS_MODULE,
    .open    = open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
};

/* Sysmon UID Code: write not complete */
static int sysmon_uid_len_check = 1;
static int cur_uid = 2152;
//static char msg[128];

static int sysmon_uid_open(struct inode * sp_inode, struct file *sp_file)
{
    return 0;
}

static int sysmon_uid_release(struct inode *sp_indoe, struct file *sp_file)
{
    return 0;
}

static int sysmon_uid_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset)
{
    if (sysmon_uid_len_check) 
    {
        sysmon_uid_len_check = 0;
        copy_to_user(buf, log, strlen(log));
        return strlen(log);
    }
    else 
    {
        sysmon_uid_len_check = 1;
        return 0;
    }
}

static int sysmon_uid_write(struct file *sp_file, const char __user *buf, size_t size, loff_t *offset)
{
    if (size >= 2) 
    {
 	int i = 0;
	//long* number;
	printk(KERN_INFO "buf from sysmon_uid is %s\n",buf);
	/*	
	kstrtol(buf,10,number);
	printk(KERN_INFO "number from sysmon_uid is %lu\n",*number);
	
	while (buf[i]!='\0')
	{
    	    number[i] =buf[i];
	    i++;
	}*/
        //printk(KERN_INFO "number from sysmon_uid is %s\n",number);
	/*int num = number[0]-'0';
        if (num>0) 
        {
            cur_uid = num;
            printk(KERN_INFO "Set uid to %d\n",cur_uid);
            return size;
        }
	*/
    }
    return -EINVAL;
}

static const struct file_operations sysmon_uid_fops = {
    .open = sysmon_uid_open,
    .read = sysmon_uid_read,
    .write = sysmon_uid_write,
    .release = sysmon_uid_release
};


/* The example on T-Square uses regs->rax, but I had to use regs->ax
 * See documentation for struct pt_regs here:
 * http://lxr.free-electrons.com/source/arch/x86/include/asm/ptrace.h 
 */
int sysmon_intercept_before(struct kprobe *p, struct pt_regs *regs) { 
    int ret = 0;
    char entry[200];

    if (!on_or_off)
    {
        return ret;
    }
    //if(current->pid != THE_UID) return ret;
    switch (regs->ax) {
        case __NR_access:
            break;

        case __NR_brk:
            break;

        case __NR_chdir:
            sprintf(entry, "my sysmon_intercept_before: regs->ax = %lu, "
                "current->pid = %d, current->tgid = %d, regs->di = %lu, "
                "__NR_chdir: %lu\n", regs->ax, current->pid, current->tgid, 
                (uintptr_t) regs->di, (long unsigned int) __NR_chdir);
            add_entry_to_log(entry);
            break;

        case __NR_chmod:
            break;

        case __NR_clone:
            break;

        case __NR_close:
            break;

        case __NR_dup:
            break;

        case __NR_dup2:
            break;

        case __NR_execve:
            break;

        case __NR_exit_group:
            break;

        case __NR_fcntl:
            break;

        case __NR_fork:
            break;

        case __NR_getdents:
            break;

        case __NR_getpid:
            break;

        case __NR_gettid:
            break;

        case __NR_ioctl:
            break;

        case __NR_lseek:
            /*sprintf(entry, "my sysmon_intercept_before: regs->ax = %lu, "
                "current->pid = %d, current->tgid = %d, regs->di = %lu, "
                "__NR_lseek: %lu\n", regs->ax, current->pid, current->tgid, 
                (uintptr_t) regs->di, (long unsigned int) __NR_lseek);
            add_entry_to_log(entry);*/
            break;

        case __NR_mkdir:
            sprintf(entry, "my sysmon_intercept_before: regs->ax = %lu, "
                "current->pid = %d, current->tgid = %d, regs->di = %lu, "
                "__NR_mkdir: %lu\n", regs->ax, current->pid, current->tgid, 
                (uintptr_t) regs->di, (long unsigned int) __NR_mkdir);
            add_entry_to_log(entry);
            break;

        case __NR_mmap:
            break;

        case __NR_munmap:
            break;

        case __NR_open:
            break;

        case __NR_pipe:
            break;

        case __NR_read:
            break;

        case __NR_rmdir:
            break;

        case __NR_select:
            break;

        case __NR_stat:
            break;

        case __NR_fstat:
            break;

        case __NR_lstat:
            break;

        case __NR_wait4:
            break;

        case __NR_write:
            break;

        default:
            ret = -1;
    } 

    return ret;
} 
 
void sysmon_intercept_after(struct kprobe *p, struct pt_regs *regs, 
    unsigned long flags) { 
    /* Here you could capture the return code if you wanted. */
} 
 
int my_init_module(void) {
    int ret, i = 0;
    char *entry;
    entry = "The log is empty.\n";
    

    for ( ; i < NUM_KPROBES; i++) {
        kprobes[i].symbol_name = symbol_names[i];
        kprobes[i].pre_handler = sysmon_intercept_before; 
        kprobes[i].post_handler = sysmon_intercept_after; 
        ret = register_kprobe(&kprobes[i]);
        if (ret < 0) {
            printk(KERN_INFO "register_kprobe failed, symbol_name = %s, "
                "returned = %d\n", symbol_names[i], ret);
            return ret;
        }
    }
    
    printk(KERN_INFO "Sysmon log module successfully initialized.\n"); 

    log = (char *) kmalloc(sizeof(char) * LOG_MAX_LENGTH, __GFP_REPEAT);
    add_entry_to_log(entry);
    
    proc_create(SYSMON_TOGGLE, 0600, NULL, &sysmon_toggle_fops);
    proc_create(SYSMON_UID, 0600, NULL, &sysmon_uid_fops);
    proc_create(SYSMON_LOG, 0400, NULL, &sysmon_log_fops);

    return 0; 
} 
 
void my_cleanup_module(void) { 
    int i = 0;

    for ( ; i < NUM_KPROBES; i++) {
        unregister_kprobe(&kprobes[i]); 
    }

    kfree(log);
    remove_proc_entry(SYSMON_TOGGLE, NULL);
    remove_proc_entry(SYSMON_LOG, NULL);

    printk(KERN_INFO "Sysmon log module removed.\n"); 
} 

module_init(my_init_module); 
module_exit(my_cleanup_module); 

