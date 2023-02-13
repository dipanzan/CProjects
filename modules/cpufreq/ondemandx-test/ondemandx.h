/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Header file for CPUFreq ondemand governor and related code.
 *
 * Copyright (C) 2016, Intel Corporation
 * Author: Rafael J. Wysocki <rafael.j.wysocki@intel.com>
 */

#ifndef _ONDEMANDX_H
#define _ONDEMANDX_H

#include <linux/ioctl.h>
#include <linux/uaccess.h>

#include "cpufreq_governor.h"

static ssize_t device_read(struct file *file, char __user *buf, size_t len, loff_t *off);
static ssize_t device_write(struct file *file, const char *buf, size_t len, loff_t *off);
static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#define ONDEMANDX "ondemandx"
#define IOCTL "[IOCTL]"

// #define "ioctl name" __IOX("magic number","command number","argument type")
#define WR_VALUE _IOW('a', 'a', int *)
#define RD_VALUE _IOR('a', 'b', int *)

struct od_policy_dbs_info
{
	struct policy_dbs_info policy_dbs;
	unsigned int freq_lo;
	unsigned int freq_lo_delay_us;
	unsigned int freq_hi_delay_us;
	unsigned int sample_type : 1;
};

static inline struct od_policy_dbs_info *to_dbs_info(struct policy_dbs_info *policy_dbs)
{
	return container_of(policy_dbs, struct od_policy_dbs_info, policy_dbs);
}

struct od_dbs_tuners
{
	unsigned int powersave_bias;
};

static void print_freq_table(struct cpufreq_policy *policy)
{
	static unsigned int print_once = 0;
	struct cpufreq_frequency_table *freq_table = policy->freq_table;

	if (!freq_table)
	{
		printk(KERN_ALERT "freq_table not available!");
	}
	else
	{
		if (print_once != 1)
		{
			unsigned int n_frequencies = 0;
			while (freq_table[n_frequencies].frequency != CPUFREQ_TABLE_END)
				printk(KERN_INFO "freq_table[%d] = %dKHz",
					   n_frequencies, freq_table[n_frequencies].frequency);
			n_frequencies++;
		}
		print_once = 1;
	}
}

static void print_freq_min_max(struct cpufreq_policy *policy)
{
	unsigned int min_f, max_f;
	min_f = policy->cpuinfo.min_freq;
	max_f = policy->cpuinfo.max_freq;

	printk(KERN_INFO "min_f: %d, max_f: %d", min_f, max_f);
}

#endif /* _ONDEMANDX_H */