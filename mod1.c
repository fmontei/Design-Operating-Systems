#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

MODULE_AUTHOR("Felipe"); 
MODULE_DESCRIPTION("KPROBE MODULE"); 
MODULE_LICENSE("GPL");

#define NUM_KPROBES 30

#define MODULE_NAME "[sysmon"
#define SYSMON_LOG "sysmon_log"

#define SYS_ACCESS " - access] "
#define SYS_BRK " - brk] "
#define SYS_CHDIR " - chdir] "
#define SYS_CHMOD " - chmod] "
#define SYS_CLONE " - clone] "
#define SYS_CLOSE " - close] "
#define SYS_DUP " - dup] "
#define SYS_DUP2 " - dup2] "
#define SYS_EXECVE " - execve] "
#define SYS_EXIT_GROUP " - exit_group] "
#define SYS_FCNTL " - fcntl] "
#define SYS_FORK " - fork] "
#define SYS_GETDENTS " - getdents] "
#define SYS_GETPID " - getpid] "
#define SYS_GETTID " - gettid] "
#define SYS_IOCTL " - ioctl] "
#define SYS_LSEEK " - lseek] "
#define SYS_MKDIR " - mkdir] "
#define SYS_MMAP " - mmap] "
#define SYS_MUNMAP " - munmap] "
#define SYS_OPEN " - open] "
#define SYS_PIPE " - pipe] "
#define SYS_READ " - read] "
#define SYS_RMDIR " - rmdir] "
#define SYS_SELECT " - select] "
#define SYS_STAT " - stat] "
#define SYS_FSTAT " - fstat] "
#define SYS_LSTAT " - lstat] "
#define SYS_WAIT4 " - wait4] "
#define SYS_WRITE " - write] "

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

#define MAX_LENGTH 1024

static char *log;
static char *temp;

static void substring(char s[], char sub[], int p, int l);

static void add_entry_to_log(char *entry)
{
    if (strlen(log) + strlen(entry) < MAX_LENGTH)
    {
        strcat(log, entry);
    }
    else if (strlen(log) + strlen(entry) >= MAX_LENGTH)
    {
        char token = '\n';
        int i = 0; 
        while (token != log[i])
        {
            i++;
        }
        i += 2; // Include \n character
        
        temp = (char *) kmalloc(sizeof(char) * MAX_LENGTH, __GFP_REPEAT);
        substring(log, temp, i, MAX_LENGTH - i);
        
        kfree(log);
        log = temp;
        
        if (strlen(log) + strlen(entry) < MAX_LENGTH)
            strcat(log, entry);
        else if (strlen(log) + strlen(entry) >= MAX_LENGTH)
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
	
static int show(struct seq_file *m, void *v)
{
    seq_printf(m, "%s\n", log);
    return 0;
}

static int open(struct inode *inode, struct file *file)
{
    return single_open(file, show, NULL);
}

static ssize_t
write(struct file *file, const char *buf, size_t size, loff_t * ppos)
{
    return 0;
}

static const struct file_operations my_file_ops = {
    .owner   = THIS_MODULE,
    .open    = open,
    .write   = write,
    .read    = seq_read,
    .llseek = seq_lseek,
    .release = seq_release,
};

/* The example on T-Square uses regs->rax, but I had to use regs->ax
 * See documentation for struct pt_regs here:
 * http://lxr.free-electrons.com/source/arch/x86/include/asm/ptrace.h 
 */
int sysmon_intercept_before(struct kprobe *p, struct pt_regs *regs) { 
    int ret = 0;
    char entry[200];
   
    switch (regs->ax) {
        case __NR_access:
            break;

        case __NR_brk:
            break;

        case __NR_chdir:
            sprintf(entry, "my sysmon_intercept_before: regs->ax = %lu, "
                "current->pid = %d, current->tgid = %d, regs->di = %lu, __NR_mkdir: %lu\n",
                regs->ax, current->pid, current->tgid, 
                (uintptr_t) regs->di, (long unsigned int) __NR_mkdir);
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
            break;

        case __NR_mkdir:
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
            printk(KERN_INFO "register_kprobe failed, symbol_name = %s, " \
                "returned = %d\n", symbol_names[i], ret);
            return ret;
        }
    }
    
    printk(KERN_INFO "register_kprobe successfully initialized\n"); 

    log = (char *) kmalloc(sizeof(char) * MAX_LENGTH, __GFP_REPEAT);
    add_entry_to_log(entry);
    proc_create(SYSMON_LOG, 0600, NULL, &my_file_ops);

    return 0; 
} 
 
void my_cleanup_module(void) { 
    int i = 0;

    for ( ; i < NUM_KPROBES; i++) {
        unregister_kprobe(&kprobes[i]); 
    }

    kfree(log);
    remove_proc_entry(SYSMON_LOG, NULL);

    printk(KERN_INFO "module removed\n"); 
} 

module_init(my_init_module); 
module_exit(my_cleanup_module); 

