#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>

static struct jprobe nodeadlock_probe;

static long j_nodeadlock(const char *action, int thread_id, int index)
{
    int retval = 0;

    if (action == NULL || strcmp(action, "init") != 0
        || strcmp(action, "lock") != 0 || strcmp(action, "unlock") != 0) {
        retval = -1;
    }

    printk(KERN_INFO "action = %s, thread_id = %d, index = %d\n",
        action, thread_id, index);

    /* Always end with a call to jprobe_return(). */
    jprobe_return();
    return retval;
}

static struct jprobe nodeadlock_probe = {
    .entry = j_nodeadlock,
    .kp = {
        .symbol_name = "sys_nodeadlock",
    },
};

static int __init my_init_module(void) {
    int retval = -1;

    retval = register_jprobe(&nodeadlock_probe);
    if (retval < 0) {
        printk(KERN_INFO "register_kprobe failed.\n");
        return -1;
    }

    printk(KERN_INFO "Nodeadlock module successfully initialized.\n");

    return 0;
}

static __exit void my_cleanup_module(void) {
    unregister_jprobe(&nodeadlock_probe);

    printk(KERN_INFO "Nodeadlock module uninitialized.\n");
}

module_init(my_init_module);
module_exit(my_cleanup_module);
MODULE_AUTHOR("Team 23");
MODULE_DESCRIPTION("NODEADLOCK MODULE");
MODULE_LICENSE("GPL");

