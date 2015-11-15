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

static struct kprobe nodeadlock_kprobe;

int sysmon_intercept_before(struct kprobe *p, struct pt_regs *regs) { 
    const int cur_uid = get_current_user()->uid.val;
    int ret = 0;
    return ret;
} 
 
void sysmon_intercept_after(struct kprobe *p, struct pt_regs *regs, 
    unsigned long flags) { 
    /* Here you could capture the return code if you wanted. */
} 

int my_init_module(void) {
    int retval = -1;
    nodeadlock_kprobe.symbol_name = "sys_nodeadlock";
    nodeadlock_kprobe.pre_handler = sysmon_intercept_before; 
    nodeadlock_kprobe.post_handler = sysmon_intercept_after; 
    retval = register_kprobe(nodeadlock_kprobe);
    if (retval < 0) {
        printk(KERN_INFO "register_kprobe failed.\n");
        return ret;
    }
    
    printk(KERN_INFO "Nodeadlock module successfully initialized.\n"); 
    
    return 0;
} 
 
void my_cleanup_module(void) { 
    unregister_kprobe(nodeadlock_kprobe);
    
    printk(KERN_INFO "Nodeadlock module uninitialized.\n");
} 

module_init(my_init_module); 
module_exit(my_cleanup_module); 
MODULE_AUTHOR("Team 23"); 
MODULE_DESCRIPTION("NODEADLOCK MODULE"); 
MODULE_LICENSE("GPL");
