#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/sched.h>

static char *target_process_name = NULL;
static int target_process_pid = 0;

module_param(target_process_name, charp, 0000);
module_param(target_process_pid, int, 0000);

MODULE_PARM_DESC(target_process_name, "target_process_name is the process name arg from command line");
MODULE_PARM_DESC(target_process_pid, "target_process_pid is the process PID arg from command line");

static int process_found = 0;

static void print_all_threads_from_process(struct task_struct *_process)
{
    struct task_struct *_thread;
    char _comm[TASK_COMM_LEN];

    for_each_thread(_process, _thread)
    {
        pr_info("thread_name: %s\tPID:[%d]\tCPU:[%d]\n",
                get_task_comm(_comm, _thread),
                task_pid_nr(_thread),
                _thread->on_cpu);
    }
}

static void find_process_with_name(struct task_struct *_process)
{
    if (!strcmp(target_process_name, _process->comm))
    {
        process_found = 1;
        print_all_threads_from_process(_process);
    }
}

static void find_process_with_pid(struct task_struct *_process)
{
    if (_process->pid == target_process_pid)
    {
        process_found = 1;
        print_all_threads_from_process(_process);
    }
}

typedef void (*name_or_pid_func)(struct task_struct *);

static name_or_pid_func find_process_with_name_or_pid(void)
{
    if (target_process_name != NULL && target_process_pid == 0)
    {
        return find_process_with_name;
    }
    else if (target_process_pid != 0)
    {
        return find_process_with_pid;
    }
    else
    {
        pr_info("FAILED: target process/pid not provided.\n");
    }
    return NULL;
}

static void report_process_found(void)
{
    if (!process_found)
    {
        pr_alert("FAILED: target process [%s] | pid [%d] not running.\n",
                 target_process_name, target_process_pid);
    }
}

static void find_process(void)
{
    pr_alert("Hooking with target process: %s\n", target_process_name);
    struct task_struct *_process, *_thread;
    name_or_pid_func func = find_process_with_name_or_pid();
    rcu_read_lock();
    for_each_process(_process)
    {
        func(_process);
    }
    rcu_read_unlock();
}

static int __init test_init(void)
{
    pr_info("detect-process installed\n");
    find_process();
    report_process_found();
    return 0;
}

static void __exit test_exit(void)
{
    pr_info("detect-process uninstalled\n");
    
}

MODULE_AUTHOR("Dipanzan Islam <dipanzan@live.com>");
MODULE_DESCRIPTION("Kernel module for intercepting process/thread");
MODULE_LICENSE("GPL");

module_init(test_init);
module_exit(test_exit);
