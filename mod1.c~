#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>

static struct kprobe kp;

static unsigned int counter = 0;

int Pre_Handler(struct kprobe *p, struct pt_regs *regs) { 
    int ret = 0;
   
    /* The example on T-Square uses regs->rax, but I had to use regs->ax
     * See documentation for struct pt_regs here:
     * http://lxr.free-electrons.com/source/arch/x86/include/asm/ptrace.h 
     */
    switch (regs->ax) {
        case __NR_mkdir:
            printk(KERN_INFO "Felipe's Pre_Handler: counter = %u\n", counter++);
            break;

        default:
            ret = -1;
    } 

    return ret;
} 
 
void Post_Handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags) { 
    switch (regs->ax) {
        case __NR_mkdir:
            printk(KERN_INFO "Felipe's Post_Handler: counter = %u\n", counter++);
            break;
    }
} 
 
int myinit(void) 
{ 
    int ret;

    kp.symbol_name = "sys_mkdir";
    kp.pre_handler = Pre_Handler; 
    kp.post_handler = Post_Handler; 
    ret = register_kprobe(&kp); 

    if (ret < 0) {
		printk(KERN_INFO "register_kprobe failed, returned %d\n", ret);
		return ret;
	}

    
    printk(KERN_INFO "Probe inserted by Felipe\n"); 

    return 0; 
} 
 
void myexit(void) 
{ 
    unregister_kprobe(&kp); 
    printk(KERN_INFO "module removed\n "); 
} 
 
module_init(myinit); 
module_exit(myexit); 
MODULE_AUTHOR("Felipe"); 
MODULE_DESCRIPTION("KPROBE MODULE"); 
MODULE_LICENSE("GPL");
