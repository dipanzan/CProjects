#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/init.h>

static struct kprobe kp = {
    .symbol_name = "do_futex"
};

static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    pr_alert("futex() detected");
    pr_info("%lx", regs->ax);
    return 0;
}

static int attach_kprobe(void)
{
    kp.pre_handler = handler_pre;
    int ret = register_kprobe(&kp);

    ret < 0 ? 
        pr_info("register_kprobe() failed, err: %d", ret) :
        pr_info("kprobe attached at %p", kp.addr);
    return ret;
}

static void detach_kprobe(void)
{
    unregister_kprobe(&kp);
    pr_info("kprobe at %p unregistered", kp.addr);
    
}

static int __init kprobe_init(void)
{
    pr_info("hook-sys installed");
    int ret;
    ret = attach_kprobe();
    return ret;
}

static void __exit kprobe_exit(void)
{
    pr_info("hook-sys uninstalled");
    detach_kprobe();
}

MODULE_AUTHOR("Dipanzan Islam <dipanzan@live.com>");
MODULE_DESCRIPTION("hook-syscall futex()");
MODULE_LICENSE("GPL");

module_init(kprobe_init);
module_exit(kprobe_exit);