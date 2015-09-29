#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <asm-generic/uaccess.h>
#include <linux/cred.h>
#include <linux/time.h>

MODULE_AUTHOR("Felipe"); 
MODULE_DESCRIPTION("KPROBE MODULE"); 
MODULE_LICENSE("GPL");

#define NUM_KPROBES 30
#define SYSMON_LOG "sysmon_log"
#define SYSMON_UID "sysmon_uid"
#define SYSMON_TOGGLE "sysmon_toggle"
#define LOG_MAX_LENGTH 262144 // Max length of Log

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
static char *log;  // Log character array
static int lines_omitted = 0;
static int sysmon_uid = -1;
static unsigned int is_logging_toggled = 1;

char *get_timestamp(void)
{   
    static char ret[400];
    struct timeval t;
    struct tm broken;
    do_gettimeofday(&t);
    time_to_tm(t.tv_sec, 0, &broken);

    if (lines_omitted == 0)
        sprintf(ret, "Log contents for time = %2d:%2d:%2d.%ld\n", 
            broken.tm_hour, broken.tm_min, broken.tm_sec, t.tv_usec);
    else
        sprintf(ret, "Log was full. %d lines were omitted.\n"
            "Log has been flushed to terminal. Beginning new log " 
            "for time = %2d:%2d:%2d.%ld\n", 
            lines_omitted, broken.tm_hour, broken.tm_min, 
            broken.tm_sec, t.tv_usec);

    return ret;
}

static void add_entry_to_log(char *entry)
{
    if (strlen(log) + strlen(entry) < LOG_MAX_LENGTH - 1)
    {
        strcat(log, entry);
        log[strlen(log)] = '\0';
    }
    else
    {
        lines_omitted++;
    }
}

static void flush_log(void) {
    log[0] = '\0';
    add_entry_to_log(get_timestamp());
    if (!is_logging_toggled)
        add_entry_to_log("*** Logging has been disabled. ***\n");
    lines_omitted = 0;
}

/* Sysmon UID Code */
static int sysmon_uid_open(struct inode * sp_inode, struct file *sp_file)
{
    return 0;
}

static int sysmon_uid_release(struct inode *sp_indoe, struct file *sp_file)
{
    return 0;
}

static int sysmon_uid_read(struct file *sp_file, char __user *buf, size_t size, 
                           loff_t *offset)
{
    static int finished = 0;
    char data[10];
    
    if (finished)
    {
        finished = 0;
        return 0;
    }
    
    finished = 1;
    
    sprintf(data, "%d\n", sysmon_uid);
    if (copy_to_user(buf, data, strlen(data)))
        return -EFAULT;
        
    return strlen(data);
}

static int sysmon_uid_write(struct file *sp_file, const char __user *buf, 
                            size_t size, loff_t *offset)
{
    if (size >= 2) 
    {
	    char *str = kmalloc(size, GFP_KERNEL);
        int uid;

	    /* Copy the string from user-space to kernel-space */
	    if (copy_from_user(str, buf, size)) {
		    kfree(str);
		    return -EFAULT; // if copying not successful
	    }

	    /* Convert the string into a long */
	    sscanf(str, "%d", &uid);

	    if (uid >= 0) 
        {
            sysmon_uid = uid;
            printk(KERN_INFO "Set current uid to %d\n", sysmon_uid);
            kfree(str);
            return size;
        }
	    else 
            kfree(str);
    }
    return -EINVAL;
}

static const struct file_operations sysmon_uid_fops = {
    .open = sysmon_uid_open,
    .read = sysmon_uid_read,
    .write = sysmon_uid_write,
    .release = sysmon_uid_release
};

/* Sysmon TOGGLE Code: reference is www.pageframes.com/?p=89 */
static int sysmon_toggle_open(struct inode * sp_inode, struct file *sp_file)
{
    return 0;
}

static int sysmon_toggle_release(struct inode *sp_indoe, struct file *sp_file)
{
    return 0;
}

static int sysmon_toggle_read(struct file *sp_file, char __user *buf, size_t size, 
                              loff_t *offset)
{
    static int finished = 0;
    char data[10];
    
    if (finished)
    {
        finished = 0;
        return 0;
    }
    
    finished = 1;
    
    sprintf(data, "%d\n", is_logging_toggled);
    if (copy_to_user(buf, data, strlen(data)))
        return -EFAULT;
        
    return strlen(data);
}

static int sysmon_toggle_write(struct file *sp_file, const char __user *buf, 
                               size_t size, loff_t *offset)
{
    if (size == 2) // 2 includes the number and \0
    {
        const char number = buf[0];
        if (number == '0') 
        {
            is_logging_toggled = 0;
            printk(KERN_INFO "Sysmon log disabled.\n");
            return size;
        }
        else if (number == '1') 
        {
            is_logging_toggled = 1;
            printk(KERN_INFO "Sysmon log enabled.\n");
            return size;
        }
    }
    if (size == 6) 
    {
        char *str = kmalloc(size, GFP_KERNEL);

	    if (copy_from_user(str, buf, size)) {
		    kfree(str);
		    return -EFAULT; 
	    }

        sscanf(buf, "%6s", str);
        if (strcmp(str, "flush") == 0) 
        {
            printk(KERN_INFO "Flushing log\n");
            flush_log();
        }
        kfree(str);
        return size;
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

/* The example on T-Square uses regs->rax, but I had to use regs->ax
 * See documentation for struct pt_regs here:
 * http://lxr.free-electrons.com/source/arch/x86/include/asm/ptrace.h 
 */
int sysmon_intercept_before(struct kprobe *p, struct pt_regs *regs) { 
    const int cur_uid = get_current_user()->uid.val;
    int ret = 0;
    char entry[200];
    
    if (!is_logging_toggled)
    {
        return ret;
    }

    if (cur_uid != sysmon_uid)
    {
        return ret;
    }   

    switch (regs->ax) {
        case __NR_access:
            sprintf(entry, "RAX: %lu (__NR_access), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_brk:
            sprintf(entry, "RAX: %lu (__NR_brk), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_chdir:
            sprintf(entry, "RAX: %lu (__NR_chdir), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_chmod:
            sprintf(entry, "RAX: %lu (__NR_chmod), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_clone:
            sprintf(entry, "RAX: %lu (__NR_clone), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_close:
            sprintf(entry, "RAX: %lu (__NR_close), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_dup:
            sprintf(entry, "RAX: %lu (__NR_dup), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_dup2:
            sprintf(entry, "RAX: %lu (__NR_dup2), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_execve:  
            sprintf(entry, "RAX: %lu (__NR_execve), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_exit_group:
            sprintf(entry, "RAX: %lu (__NR_exit_group), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_fcntl:
            sprintf(entry, "RAX: %lu (__NR_fcntl), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_fork:
            sprintf(entry, "RAX: %lu (__NR_fork), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_getdents:
            sprintf(entry, "RAX: %lu (__NR_getdents), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_getpid:
            sprintf(entry, "RAX: %lu (__NR_getpid), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_gettid:
            sprintf(entry, "RAX: %lu (__NR_gettid), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_ioctl:
            sprintf(entry, "RAX: %lu (__NR_ioctl), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_lseek:
            sprintf(entry, "RAX: %lu (__NR_lseek), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_mkdir:
            sprintf(entry, "RAX: %lu (__NR_mkdir), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_mmap:
            sprintf(entry, "RAX: %lu (__NR_mmap), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_munmap:
            sprintf(entry, "RAX: %lu (__NR_munmap), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_open:
            sprintf(entry, "RAX: %lu (__NR_open), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_pipe:
            sprintf(entry, "RAX: %lu (__NR_pipe), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_read:
            sprintf(entry, "RAX: %lu (__NR_read), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_rmdir:
            sprintf(entry, "RAX: %lu (__NR_rmdir), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_select:
            sprintf(entry, "RAX: %lu (__NR_select), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_stat:
            sprintf(entry, "RAX: %lu (__NR_stat), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_fstat:
            sprintf(entry, "RAX: %lu (__NR_fstat), "
                "RDI (Arg0): %lu, Arg1: %lu, UID: %d, PID: %lu, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_lstat:
            sprintf(entry, "RAX: %lu (__NR_lstat), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_wait4:
            sprintf(entry, "RAX: %lu (__NR_wait4), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
            break;

        case __NR_write:
            sprintf(entry, "RAX: %lu (__NR_write), "
                "Arg0: %lu, Arg1: %lu, UID: %d, PID: %d, TGID: %d\n",
                regs->ax, (uintptr_t) regs->di, (uintptr_t) regs->si, sysmon_uid, 
                current->pid, current->tgid);
            add_entry_to_log(entry);
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

    log = (char *) kmalloc(sizeof(char) * LOG_MAX_LENGTH, GFP_KERNEL);
    log[0] = '\0';
    add_entry_to_log(get_timestamp());
    
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
    remove_proc_entry(SYSMON_UID, NULL);
    remove_proc_entry(SYSMON_LOG, NULL);    

    printk(KERN_INFO "Sysmon log module removed.\n"); 
} 

module_init(my_init_module); 
module_exit(my_cleanup_module); 

