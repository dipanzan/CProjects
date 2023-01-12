// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/drivers/cpufreq/cpufreq_performance.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "userspacex.h"

#define FREQ_SCALING_MIN 400000
#define FREQ_SCALING_MAX 4463000

#define USERSPACEX "userspacex"

static DEFINE_PER_CPU(unsigned int, cpu_is_managed);
static DEFINE_MUTEX(userspace_mutex);

static struct task_struct kthread_arr;

int poller(void* idx)
{
    return 0;
}


/*  cpufreq_set - set the CPU frequency
    @policy: pointer to policy struct where freq is being set
    @freq: target frequency in kHz
    Sets the CPU frequency to freq.
*/
static int cpufreq_set(struct cpufreq_policy *policy, unsigned int freq)
{
    int ret = -EINVAL;
    unsigned int *setspeed = policy->governor_data;

    pr_debug("cpufreq_set for cpu %u, freq %u kHz\n", policy->cpu, freq);

    mutex_lock(&userspace_mutex);
    if (!per_cpu(cpu_is_managed, policy->cpu))
        goto err;

    // 400Hz
    freq = FREQ_SCALING_MIN;

    *setspeed = freq;

    printk(KERN_ALERT "cpufreq_set for cpu %u, freq %u kHz\n", policy->cpu, freq);

    ret = __cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);
err:
    mutex_unlock(&userspace_mutex);
    return ret;
}

static ssize_t show_speed(struct cpufreq_policy *policy, char *buf)
{
    return sprintf(buf, "%u\n", policy->cur);
}

static int cpufreq_userspace_policy_init(struct cpufreq_policy *policy)
{
    unsigned int *setspeed;

    setspeed = kzalloc(sizeof(*setspeed), GFP_KERNEL);
    if (!setspeed)
        return -ENOMEM;

    policy->governor_data = setspeed;
    return 0;
}

static void cpufreq_userspace_policy_exit(struct cpufreq_policy *policy)
{
    mutex_lock(&userspace_mutex);
    kfree(policy->governor_data);
    policy->governor_data = NULL;
    mutex_unlock(&userspace_mutex);
}

static int cpufreq_userspace_policy_start(struct cpufreq_policy *policy)
{
    unsigned int *setspeed = policy->governor_data;

    BUG_ON(!policy->cur);
    pr_debug("started managing cpu %u\n", policy->cpu);

    mutex_lock(&userspace_mutex);
    per_cpu(cpu_is_managed, policy->cpu) = 1;
    *setspeed = policy->cur;
    mutex_unlock(&userspace_mutex);
    return 0;
}

static void cpufreq_userspace_policy_stop(struct cpufreq_policy *policy)
{
    unsigned int *setspeed = policy->governor_data;

    pr_debug("managing cpu %u stopped\n", policy->cpu);

    mutex_lock(&userspace_mutex);
    per_cpu(cpu_is_managed, policy->cpu) = 0;
    *setspeed = 0;
    mutex_unlock(&userspace_mutex);
}

static void cpufreq_userspace_policy_limits(struct cpufreq_policy *policy)
{
    unsigned int *setspeed = policy->governor_data;

    mutex_lock(&userspace_mutex);

    pr_debug("limit event for cpu %u: %u - %u kHz, currently %u kHz, last set to %u kHz\n",
             policy->cpu, policy->min, policy->max, policy->cur, *setspeed);

    if (policy->max < *setspeed)
        __cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
    else if (policy->min > *setspeed)
        __cpufreq_driver_target(policy, policy->min, CPUFREQ_RELATION_L);
    else
        __cpufreq_driver_target(policy, *setspeed, CPUFREQ_RELATION_L);

    mutex_unlock(&userspace_mutex);
}

static struct cpufreq_governor cpufreq_gov_userspacex = {
    .name = USERSPACEX,
    .owner = THIS_MODULE,
    .init = cpufreq_userspace_policy_init,
    .exit = cpufreq_userspace_policy_exit,
    .start = cpufreq_userspace_policy_start,
    .stop = cpufreq_userspace_policy_stop,
    .limits = cpufreq_userspace_policy_limits,
    .store_setspeed = cpufreq_set,
    .show_setspeed = show_speed
};

#define CPUFREQ_GOV_USERSPACEX (cpufreq_gov_userspacex)

static int __init cpufreq_userspacex_dbs_init(void)
{
    unsigned int n_cpu, i;
    n_cpu = 0;
    for_each_online_cpu(i)
    {
        n_cpu++;
    }
    printk(KERN_INFO "%s governor __init - online CPUs : %u", USERSPACEX, n_cpu);
    printk(KERN_INFO "%s governor INSTALLED successfully!\n", USERSPACEX);

    return cpufreq_register_governor(&CPUFREQ_GOV_USERSPACEX);
}

static void __exit cpufreq_userspacex_dbs_exit(void)
{
    printk(KERN_INFO "userspacex governor UNINSTALLED successfully!\n");
    cpufreq_unregister_governor(&CPUFREQ_GOV_USERSPACEX);
}

MODULE_AUTHOR("Dipanzan Islam <dipanzan@live.com>");
MODULE_DESCRIPTION("CPUfreq policy governor 'userspacex'");
MODULE_LICENSE("GPL");

module_init(cpufreq_userspacex_dbs_init);
module_exit(cpufreq_userspacex_dbs_exit);