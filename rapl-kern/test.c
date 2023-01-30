#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/string.h>
#include <linux/sched.h>

#define PROC_NAME "lock"

static char process_name[TASK_COMM_LEN];
static struct task_struct *__process_struct;

static void print_task_info(unsigned int tgid)
{
    struct task_struct *process;
    struct task_struct *thread;
    char comm[TASK_COMM_LEN];

    rcu_read_lock();
    for_each_process(process)
    {
        for_each_thread(process, thread)
        {
            pr_info("thread node: %p\t Name: %s\t PID:[%d]\t TGID:[%d]\n",
                    thread, get_task_comm(comm, thread),
                    task_pid_nr(thread), task_tgid_nr(thread));

            if (!strcmp(PROC_NAME, comm))
            {
                __process_struct = process;
                break;
            }
        }
    }
    rcu_read_unlock();
}

static void print_task(void)
{
    if (__process_struct)
    {
        char *comm = __process_struct->comm;
        unsigned int on_cpu = __process_struct->on_cpu;
        pr_alert("PROCESS DETECTED: %s on CPU: %d", comm, on_cpu);
        pr_alert("YAAAY");
    }
}

static int __init test_init(void)
{
    pr_info("test-sys installed");
    print_task_info(0);
    print_task();
    return 0;
}

static void __exit test_exit(void)
{
    pr_info("test-sys uninstalled");
}

MODULE_AUTHOR("Dipanzan Islam <dipanzan@live.com>");
MODULE_DESCRIPTION("Kernel module for intercepting process/thread");
MODULE_LICENSE("GPL");

module_init(test_init);
module_exit(test_exit);