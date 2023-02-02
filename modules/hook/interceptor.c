#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/current.h>
#include <asm/ptrace.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <asm/unistd.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/syscalls.h>
#include "interceptor.h"

MODULE_DESCRIPTION("My kernel module");
MODULE_AUTHOR("Harshul Goel");
MODULE_LICENSE("GPL");

//----- System Call Table Stuff ------------------------------------
/* Symbol that allows access to the kernel system call table */
extern void *sys_call_table[];

/* The sys_call_table is read-only => must make it RW before replacing a syscall */
void set_addr_rw(unsigned long addr)
{

    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);

    if (pte->pte & ~_PAGE_RW)
        pte->pte |= _PAGE_RW;
}

/* Restores the sys_call_table as read-only */
void set_addr_ro(unsigned long addr)
{

    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);

    pte->pte = pte->pte & ~_PAGE_RW;
}
//-------------------------------------------------------------

//----- Data structures and bookkeeping -----------------------
/**
 * This block contains the data structures needed for keeping track of
 * intercepted system calls (including their original calls), pid monitoring
 * synchronization on shared data, etc.
 * It's highly unlikely that you will need any globals other than these.
 */

/* List structure - each intercepted syscall may have a list of monitored pids */
struct pid_list
{
    pid_t pid;
    struct list_head list;
};

/* Store info about intercepted/replaced system calls */
typedef struct
{

    /* Original system call */
    asmlinkage long (*f)(struct pt_regs);

    /* Status: 1=intercepted, 0=not intercepted */
    int intercepted;

    /* Are any PIDs being monitored for this syscall? */
    int monitored;
    /* List of monitored PIDs */
    int listcount;
    struct list_head my_list;
} mytable;

/* An entry for each system call in this "metadata" table */
mytable table[NR_syscalls];

/* Access to the system call table and your metadata table must be synchronized */
spinlock_t my_table_lock = SPIN_LOCK_UNLOCKED;
spinlock_t sys_call_table_lock = SPIN_LOCK_UNLOCKED;
//-------------------------------------------------------------

//----------LIST OPERATIONS------------------------------------
/**
 * These operations are meant for manipulating the list of pids
 * Nothing to do here, but please make sure to read over these functions
 * to understand their purpose, as you will need to use them!
 */

/**
 * Add a pid to a syscall's list of monitored pids.
 * Returns -ENOMEM if the operation is unsuccessful.
 */
static int add_pid_sysc(pid_t pid, int sysc)
{
    struct pid_list *ple = (struct pid_list *)kmalloc(sizeof(struct pid_list), GFP_KERNEL);

    if (!ple)
        return -ENOMEM;

    INIT_LIST_HEAD(&ple->list);
    ple->pid = pid;

    list_add(&ple->list, &(table[sysc].my_list));
    table[sysc].listcount++;

    return 0;
}

/**
 * Remove a pid from a system call's list of monitored pids.
 * Returns -EINVAL if no such pid was found in the list.
 */
static int del_pid_sysc(pid_t pid, int sysc)
{
    struct list_head *i;
    struct pid_list *ple;

    list_for_each(i, &(table[sysc].my_list))
    {

        ple = list_entry(i, struct pid_list, list);
        if (ple->pid == pid)
        {

            list_del(i);
            kfree(ple);

            table[sysc].listcount--;
            /* If there are no more pids in sysc's list of pids, then
             * stop the monitoring only if it's not for all pids (monitored=2) */
            if (table[sysc].listcount == 0 && table[sysc].monitored == 1)
            {
                table[sysc].monitored = 0;
            }

            return 0;
        }
    }

    return -EINVAL;
}

/**
 * Remove a pid from all the lists of monitored pids (for all intercepted syscalls).
 * Returns -1 if this process is not being monitored in any list.
 */
static int del_pid(pid_t pid)
{
    struct list_head *i, *n;
    struct pid_list *ple;
    int ispid = 0, s = 0;

    for (s = 1; s < NR_syscalls; s++)
    {

        list_for_each_safe(i, n, &(table[s].my_list))
        {

            ple = list_entry(i, struct pid_list, list);
            if (ple->pid == pid)
            {

                list_del(i);
                ispid = 1;
                kfree(ple);

                table[s].listcount--;
                /* If there are no more pids in sysc's list of pids, then
                 * stop the monitoring only if it's not for all pids (monitored=2) */
                if (table[s].listcount == 0 && table[s].monitored == 1)
                {
                    table[s].monitored = 0;
                }
            }
        }
    }

    if (ispid)
        return 0;
    return -1;
}

/**
 * Clear the list of monitored pids for a specific syscall.
 */
static void destroy_list(int sysc)
{

    struct list_head *i, *n;
    struct pid_list *ple;

    list_for_each_safe(i, n, &(table[sysc].my_list))
    {

        ple = list_entry(i, struct pid_list, list);
        list_del(i);
        kfree(ple);
    }

    table[sysc].listcount = 0;
    table[sysc].monitored = 0;
}

/**
 * Check if two pids have the same owner - useful for checking if a pid
 * requested to be monitored is owned by the requesting process.
 * Remember that when requesting to start monitoring for a pid, only the
 * owner of that pid is allowed to request that.
 */
static int check_pids_same_owner(pid_t pid1, pid_t pid2)
{

    struct task_struct *p1 = pid_task(find_vpid(pid1), PIDTYPE_PID);
    struct task_struct *p2 = pid_task(find_vpid(pid2), PIDTYPE_PID);
    if (p1->real_cred->uid != p2->real_cred->uid)
        return -EPERM;
    return 0;
}

/**
 * Check if a pid is already being monitored for a specific syscall.
 * Returns 1 if it already is, or 0 if pid is not in sysc's list.
 */
static int check_pid_monitored(int sysc, pid_t pid)
{

    struct list_head *i;
    struct pid_list *ple;

    list_for_each(i, &(table[sysc].my_list))
    {

        ple = list_entry(i, struct pid_list, list);
        if (ple->pid == pid)
            return 1;
    }
    return 0;
}
//----------------------------------------------------------------

//----- Intercepting exit_group ----------------------------------
/**
 * Since a process can exit without its owner specifically requesting
 * to stop monitoring it, we must intercept the exit_group system call
 * so that we can remove the exiting process's pid from *all* syscall lists.
 */

/**
 * Stores original exit_group function - after all, we must restore it
 * when our kernel module exits.
 */
asmlinkage long (*orig_exit_group)(struct pt_regs reg);

/**
 * Our custom exit_group system call.
 *
 *
 */
asmlinkage long my_exit_group(struct pt_regs reg)
{
    // delete_pid modifies the my_list values in table so lock is needed
    spin_lock(&my_table_lock);
    del_pid(current->pid);
    spin_unlock(&my_table_lock);
    return orig_exit_group(reg);
}
//----------------------------------------------------------------

/**
 * This is the generic interceptor function.
 * It should just log a message and call the original syscall.
 *
 */
asmlinkage long interceptor(struct pt_regs reg)
{
    int monitoredStatus;
    asmlinkage long (*orig_syscall)(struct pt_regs);

    spin_lock(&my_table_lock);
    monitoredStatus = table[reg.ax].monitored;
    /* If all pids are being monitored or the current pid is being monitored for syscall(reg.ax), then
     * log a message containing the values for registers */
    if (monitoredStatus > 1 || (monitoredStatus > 0 && check_pid_monitored(reg.ax, current->pid)))
    {
        log_message(current->pid, reg.ax, reg.bx, reg.cx, reg.dx, reg.si, reg.di, reg.bp)
    }
    // save the original syscall to be returned and unlock table
    orig_syscall = table[reg.ax].f;
    spin_unlock(&my_table_lock);
    return orig_syscall(reg);
}

/**
 * Takes in an integer which is a valid syscall that was intercepted
 * Sets systemcall table to the value from table[syscall].f and releases the value from the table
 * */
static void clean_deintercept(int syscall)
{
    spin_lock(&sys_call_table_lock);
    set_addr_rw((unsigned long)sys_call_table);
    sys_call_table[syscall] = table[syscall].f;
    set_addr_ro((unsigned long)sys_call_table);
    spin_unlock(&sys_call_table_lock);
    // release the systemcall and clean the pid list
    table[syscall].f = NULL;
    table[syscall].intercepted = 0;
    destroy_list(syscall);
}

/**
 * Takes in a long pointer argument which is then assigned the error code based on the intercepted status
 * Returns 1 if there is an error 0 when there is none
 * */
int check_syscall_error(long *errortype, int intercepted, int value)
{
    if (current_uid()) // check if the caller is root
    {
        *errortype = -EPERM;
    }
    // determine if the call has been intercepted or not and return the error accordingly
    else if (intercepted == value)
    {
        *errortype = (value) ? -EBUSY : -EINVAL;
    }
    else
    {
        return 0; // no errors found
    }
    return 1;
}

/**
 * Takes in a long pointer argument which is then assigned the
 * error code based on the monitered and intercepted status
 *
 * Returns 1 if there is an error 0 when there is none
 * */
int check_monitoring_error(long *errorType, int syscall, int pid, int intercepted, int monitored, int value)
{
    /* if the current user id is non-root and (pid is non 0 and current pid doesnt have share the same owner)
    then set errorType to permission error */
    if (current_uid() && (!pid || check_pids_same_owner(current->pid, pid)))
    {
        *errorType = -EPERM;
    } // intercepted == 0 (not intercepted )or pid is negative or (pid is 0 AND pid isnt that process's)
    else if (!intercepted || pid < 0 || (pid && !pid_task(find_vpid(pid), PIDTYPE_PID)))
    {
        *errorType = -EINVAL;
    } // check if the system call is already being monitored and return the error based on the value
    else if (!(monitored || value) || (monitored == 1 && check_pid_monitored(syscall, pid) == value))
    {
        *errorType = (value) ? -EBUSY : -EINVAL;
    }
    else
    {
        return 0;
    }
    return 1;
}
/**
 * My system call - this function is called whenever a user issues a MY_CUSTOM_SYSCALL system call.
 * When that happens, the parameters for this system call indicate one of 4 actions/commands:
 *      - REQUEST_SYSCALL_INTERCEPT to intercept the 'syscall' argument
 *      - REQUEST_SYSCALL_RELEASE to de-intercept the 'syscall' argument
 *      - REQUEST_START_MONITORING to start monitoring for 'pid' whenever it issues 'syscall'
 *      - REQUEST_STOP_MONITORING to stop monitoring for 'pid'
 *      For the last two, if pid=0, that translates to "all pids".
 *
 *
 */
asmlinkage long my_syscall(int cmd, int syscall, int pid)
{
    long errorType;
    // if syscall argument is invalid then return invalid code
    if (syscall > NR_syscalls - 1 || syscall < 0 || syscall == MY_CUSTOM_SYSCALL)
    {
        return -EINVAL;
    }

    spin_lock(&my_table_lock);
    switch (cmd)
    {
    case REQUEST_SYSCALL_INTERCEPT:
        // checking the caller has root acces or if the call has already been intercepted
        if (check_syscall_error(&errorType, table[syscall].intercepted, 1))
        {
            spin_unlock(&my_table_lock);
            return errorType;
        }
        table[syscall].intercepted = 1;
        // save the orignal systemcall and replace it with interceptor
        spin_lock(&sys_call_table_lock);
        table[syscall].f = sys_call_table[syscall];
        set_addr_rw((unsigned long)sys_call_table);
        sys_call_table[syscall] = interceptor;
        set_addr_ro((unsigned long)sys_call_table);
        spin_unlock(&sys_call_table_lock);
        break;

    case REQUEST_SYSCALL_RELEASE:
        if (check_syscall_error(&errorType, table[syscall].intercepted, 0))
        {
            spin_unlock(&my_table_lock);
            return errorType;
        }
        clean_deintercept(syscall);
        break;

    case REQUEST_START_MONITORING:
        /*if check_monitoring_error finds an error (e.g. monitoring the current pid already)
         * then return the respective errorType*/
        if (check_monitoring_error(&errorType, syscall, pid, table[syscall].intercepted, table[syscall].monitored, 1))
        {
            spin_unlock(&my_table_lock);
            return errorType;
        }
        if (!pid) // remove any pids from list to set monitor all
        {
            destroy_list(syscall);
        }
        else if (add_pid_sysc(pid, syscall))
        { // return memory error if failed to add pid
            spin_unlock(&my_table_lock);
            return -ENOMEM;
        }
        table[syscall].monitored = (pid) ? 1 : 2; // if pid is non zero, only some pids are monitored
        break;

    case REQUEST_STOP_MONITORING:
        /* if check_monitoring_error finds an error (e.g. monitoring is set to 0)
         * then return the respective errorType */
        if (check_monitoring_error(&errorType, syscall, pid, table[syscall].intercepted, table[syscall].monitored, 0))
        {
            spin_unlock(&my_table_lock);
            return errorType;
        }
        /* if pid is 0 then remove all pids for syscall
         * else delete pid from list and return -EINVAL upon failure */
        if (!pid)
        {
            destroy_list(syscall);
        }
        else if (del_pid_sysc(pid, syscall))
        {
            spin_unlock(&my_table_lock);
            return -EINVAL;
        }
        break;

    default: // invalid cmd provided
        spin_unlock(&my_table_lock);
        return -EINVAL;
    }
    spin_unlock(&my_table_lock);
    return 0;
}

/**
 *
 */
long (*orig_custom_syscall)(void);

/**
 * Module initialization.
 *
 */
static int init_function(void)
{
    int a = 0;

    spin_lock(&sys_call_table_lock);
    // save our orignal systemcall
    orig_custom_syscall = sys_call_table[MY_CUSTOM_SYSCALL];
    orig_exit_group = sys_call_table[__NR_exit_group];

    set_addr_rw((unsigned long)sys_call_table);
    sys_call_table[MY_CUSTOM_SYSCALL] = my_syscall;
    sys_call_table[__NR_exit_group] = my_exit_group;
    set_addr_ro((unsigned long)sys_call_table);
    spin_unlock(&sys_call_table_lock);

    spin_lock(&my_table_lock);
    while (a < NR_syscalls)
    {
        table[a].f = NULL;
        table[a].intercepted = table[a].monitored = table[a].listcount = 0;
        INIT_LIST_HEAD(&table[a].my_list);
        a++;
    }
    spin_unlock(&my_table_lock);
    return 0;
}

/**
 * Module exits.
 *
 */
static void exit_function(void)
{
    int syscall;
    spin_lock(&sys_call_table_lock);
    set_addr_rw((unsigned long)sys_call_table);
    sys_call_table[MY_CUSTOM_SYSCALL] = orig_custom_syscall;
    sys_call_table[__NR_exit_group] = orig_exit_group;
    set_addr_ro((unsigned long)sys_call_table);
    spin_unlock(&sys_call_table_lock);

    spin_lock(&my_table_lock);
    for (syscall = 0; syscall < NR_syscalls; syscall++)
    {
        /* if syscall was intercepted and not released then
         * run clean_deintercept on syscall */
        if (table[syscall].intercepted)
            clean_deintercept(syscall);
    }
    spin_unlock(&my_table_lock);
}

module_init(init_function);
module_exit(exit_function);