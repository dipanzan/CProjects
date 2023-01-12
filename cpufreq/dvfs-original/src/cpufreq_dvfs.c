/*
 *  Kernel version : 4.13 (ex: Ubuntu 16.04)
 *  drivers/cpufreq/cpufreq_dvfs.c
 *  drivers/cpufreq/cpufreq_ondemand.c
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/types.h>
#include <linux/cpu.h>
#include <linux/percpu-defs.h>
#include <linux/slab.h>
#include <linux/tick.h>
#include <linux/sched/cpufreq.h>
#include <linux/cpufreq.h>
#include <linux/timer.h>
#include <linux/netdevice.h>
#include <linux/mm.h>
#include "cpufreq_dvfs.h"

/* Used for computation of the download speed
 * DownloadSpeed is an estimation based of the average volume of data (in Byte) received between
 * ten execution of the governor. Ondemand governor is called every 10 000 uS (10 ms -> 100 times
 * in 1 seconds). The value is in KB and store in global variable "download_speed"
 *
 * Formula: mean / NB_VALUE_USED / 1024 * 100
 */
#define NB_VALUE_FOR_MEAN   10
static unsigned int download_speed = 0;
static __u64 old_tr_bytes = 0;
static struct timer_list network_timer;

/* Memory load variables */
static unsigned int memory_load;

void update_network_metrics(void)
{
    struct net_device *dev;
    struct rtnl_link_stats64 temp;
    struct rtnl_link_stats64 *net_stats;
    __u64 tr_bytes, diffByte;

    printk(KERN_INFO, "updating download speed");

    //char* interface = "eth0";
    read_lock(&dev_base_lock);

    tr_bytes = 0;
    dev = first_net_device(&init_net);
    while (dev) {
        net_stats = dev_get_stats(dev, &temp);
        tr_bytes += net_stats->tx_bytes + net_stats->rx_bytes;


        // next network interface
        dev = next_net_device(dev);
    }

    read_unlock(&dev_base_lock);

    // compute the number of packet received since the last call of the governor
    diffByte = tr_bytes - old_tr_bytes;
    old_tr_bytes = tr_bytes;

    // Add this value to the mean and compute the average download speed every NB_VALUE_FOR_MEAN
    download_speed = diffByte / 1024 ;
    printk(KERN_INFO "download speed: %u", download_speed);
    mod_timer(&network_timer, jiffies + msecs_to_jiffies(1000));
}

void update_memory_metrics(void)
{
	struct sysinfo ram_info;
	si_meminfo(&ram_info);
	__kernel_long_t total = ram_info.totalram * ram_info.mem_unit;
	__kernel_long_t free = ram_info.freeram * ram_info.mem_unit;
	memory_load = 100 - (100 * free) / total;

	if (memory_load > 100)
		memory_load = 100;
	if (memory_load < 0)
		memory_load = 0;
}

void update_load_metrics(unsigned long data)
{
    update_network_metrics();
    update_memory_metrics();
    return;
}

/* DVFS governor macros */
#define DEF_FREQUENCY_UP_THRESHOLD		(80)
#define DEF_SAMPLING_DOWN_FACTOR		(1)
#define MAX_SAMPLING_DOWN_FACTOR		(100000)
#define MICRO_FREQUENCY_UP_THRESHOLD		(95)
#define MICRO_FREQUENCY_MIN_SAMPLE_RATE		(10000)
#define MIN_FREQUENCY_UP_THRESHOLD		(1)
#define MAX_FREQUENCY_UP_THRESHOLD		(100)

static struct dvfs_ops dvfs_ops;

static unsigned int default_powersave_bias;

/*
 * Not all CPUs want IO time to be accounted as busy; this depends on how
 * efficient idling at a higher frequency/voltage is.
 * Pavel Machek says this is not so for various generations of AMD and old
 * Intel systems.
 * Mike Chan (android.com) claims this is also not true for ARM.
 * Because of this, whitelist specific known (series) of CPUs by default, and
 * leave all others up to the user.
 */
static int should_io_be_busy(void)
{
#if defined(CONFIG_X86)
	/*
	 * For Intel, Core 2 (model 15) and later have an efficient idle.
	 */
	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
			boot_cpu_data.x86 == 6 &&
			boot_cpu_data.x86_model >= 15)
		return 1;
#endif
	return 0;
}

/*
 * Find right freq to be set now with powersave_bias on.
 * Returns the freq_hi to be used right now and will set freq_hi_delay_us,
 * freq_lo, and freq_lo_delay_us in percpu area for averaging freqs.
 */
static unsigned int generic_powersave_bias_target(struct cpufreq_policy *policy,
		unsigned int freq_next, unsigned int relation)
{
	unsigned int freq_req, freq_reduc, freq_avg;
	unsigned int freq_hi, freq_lo;
	unsigned int index;
	unsigned int delay_hi_us;
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct dvfs_policy_dbs_info *dbs_info = to_dbs_info(policy_dbs);
	struct dbs_data *dbs_data = policy_dbs->dbs_data;
	struct dvfs_dbs_tuners *dvfs_tuners = dbs_data->tuners;
	struct cpufreq_frequency_table *freq_table = policy->freq_table;

	if (!freq_table) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_delay_us = 0;
		return freq_next;
	}

	index = cpufreq_frequency_table_target(policy, freq_next, relation);
	freq_req = freq_table[index].frequency;
	freq_reduc = freq_req * dvfs_tuners->powersave_bias / 1000;
	freq_avg = freq_req - freq_reduc;

	/* Find freq bounds for freq_avg in freq_table */
	index = cpufreq_table_find_index_h(policy, freq_avg);
	freq_lo = freq_table[index].frequency;
	index = cpufreq_table_find_index_l(policy, freq_avg);
	freq_hi = freq_table[index].frequency;

	/* Find out how long we have to be in hi and lo freqs */
	if (freq_hi == freq_lo) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_delay_us = 0;
		return freq_lo;
	}
	delay_hi_us = (freq_avg - freq_lo) * dbs_data->sampling_rate;
	delay_hi_us += (freq_hi - freq_lo) / 2;
	delay_hi_us /= freq_hi - freq_lo;
	dbs_info->freq_hi_delay_us = delay_hi_us;
	dbs_info->freq_lo = freq_lo;
	dbs_info->freq_lo_delay_us = dbs_data->sampling_rate - delay_hi_us;
	return freq_hi;
}

static void dvfs_powersave_bias_init(struct cpufreq_policy *policy)
{
	struct dvfs_policy_dbs_info *dbs_info = to_dbs_info(policy->governor_data);

	dbs_info->freq_lo = 0;
}

/*
 * Every sampling_rate, we check, if current idle time is less than 20%
 * (default), then we try to increase frequency. Else, we adjust the frequency
 * proportional to load.
 */
static void dvfs_update(struct cpufreq_policy *policy)
{
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct dbs_data *dbs_data = policy_dbs->dbs_data;
	struct dvfs_dbs_tuners *dvfs_tuners = dbs_data->tuners;
	struct cpufreq_frequency_table *freq_table = policy->freq_table;
	
	unsigned int freq_next, min_f, max_f;
	unsigned int n_frequencies = 0;
	unsigned int network_load = 0;
	unsigned int cpu_load = dbs_update(policy);
	unsigned int target_freq_percent = 0;

	/* Network load decision rules */
	if (download_speed < 50) { // under 50 Kbps
		network_load = 0; // 0% "load"
	} else if (download_speed < 500) {
		network_load = 10;
	} else if (download_speed < 2048) {
		network_load = 30;
	} else if (download_speed < 5120) {
		network_load = 70;
	} else {
		network_load = 100;
	}

	/* For printing purpose, get the number of frequencies */
	while (policy->freq_table[n_frequencies].frequency != CPUFREQ_TABLE_END) {
		n_frequencies++;
	}

	/* Calculate the next frequency proportional to resources load */
	min_f = policy->cpuinfo.min_freq;
	max_f = policy->cpuinfo.max_freq;

	// TODO: Make smarter decision here
	target_freq_percent = 1 * cpu_load + -1 * network_load;

	// boundaries safecheck for target_freq_percent
	if (target_freq_percent < 0)
		target_freq_percent = 0;
	if (target_freq_percent > 100)
		target_freq_percent = 100;

	target_freq_percent = 100 - target_freq_percent; // first array item = highest frequency
	freq_next = min_f + target_freq_percent * (max_f - min_f) / 100;

	/* Set CPU frequency target */
	if (dvfs_tuners->powersave_bias) {
		freq_next = dvfs_ops.powersave_bias_target(policy, freq_next, CPUFREQ_RELATION_L);
	}
	__cpufreq_driver_target(policy, freq_next, CPUFREQ_RELATION_C);

	/* Do some prints */
	printk(KERN_INFO "dvfs_update: set frequency to %u MHz - network load ~= %u Kb/s (~%u percent). cpu_load=%u. memory_load=%u", freq_next / 1024, download_speed, network_load, cpu_load, memory_load);
}

static unsigned int dvfs_dbs_update(struct cpufreq_policy *policy)
{
	struct policy_dbs_info *policy_dbs = policy->governor_data;
	struct dbs_data *dbs_data = policy_dbs->dbs_data;
	struct dvfs_policy_dbs_info *dbs_info = to_dbs_info(policy_dbs);
	int sample_type = dbs_info->sample_type;

	/* Common NORMAL_SAMPLE setup */
	dbs_info->sample_type = DVFS_NORMAL_SAMPLE;
	/*
	 * DVFS_SUB_SAMPLE doesn't make sense if sample_delay_ns is 0, so ignore
	 * it then.
	 */
	if (sample_type == DVFS_SUB_SAMPLE && policy_dbs->sample_delay_ns > 0) {
		__cpufreq_driver_target(policy, dbs_info->freq_lo,
					CPUFREQ_RELATION_H);
		return dbs_info->freq_lo_delay_us;
	}

	dvfs_update(policy);

	if (dbs_info->freq_lo) {
		/* Setup SUB_SAMPLE */
		dbs_info->sample_type = DVFS_SUB_SAMPLE;
		return dbs_info->freq_hi_delay_us;
	}

	return dbs_data->sampling_rate * policy_dbs->rate_mult;
}

/************************** sysfs interface ************************/
static struct dbs_governor dvfs_dbs_gov;

static ssize_t store_io_is_busy(struct gov_attr_set *attr_set, const char *buf,
				size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_data->io_is_busy = !!input;

	/* we need to re-evaluate prev_cpu_idle */
	gov_update_cpu_data(dbs_data);

	return count;
}

static ssize_t store_up_threshold(struct gov_attr_set *attr_set,
				  const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}

	dbs_data->up_threshold = input;
	return count;
}

static ssize_t store_sampling_down_factor(struct gov_attr_set *attr_set,
					  const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct policy_dbs_info *policy_dbs;
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;

	dbs_data->sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	list_for_each_entry(policy_dbs, &attr_set->policy_list, list) {
		/*
		 * Doing this without locking might lead to using different
		 * rate_mult values in dvfs_update() and dvfs_dbs_update().
		 */
		mutex_lock(&policy_dbs->update_mutex);
		policy_dbs->rate_mult = 1;
		mutex_unlock(&policy_dbs->update_mutex);
	}

	return count;
}

static ssize_t store_ignore_nice_load(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_data->ignore_nice_load) { /* nothing to do */
		return count;
	}
	dbs_data->ignore_nice_load = input;

	/* we need to re-evaluate prev_cpu_idle */
	gov_update_cpu_data(dbs_data);

	return count;
}

static ssize_t store_powersave_bias(struct gov_attr_set *attr_set,
				    const char *buf, size_t count)
{
	struct dbs_data *dbs_data = to_dbs_data(attr_set);
	struct dvfs_dbs_tuners *dvfs_tuners = dbs_data->tuners;
	struct policy_dbs_info *policy_dbs;
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 1000)
		input = 1000;

	dvfs_tuners->powersave_bias = input;

	list_for_each_entry(policy_dbs, &attr_set->policy_list, list)
		dvfs_powersave_bias_init(policy_dbs->policy);

	return count;
}

gov_show_one_common(sampling_rate);
gov_show_one_common(up_threshold);
gov_show_one_common(sampling_down_factor);
gov_show_one_common(ignore_nice_load);
gov_show_one_common(min_sampling_rate);
gov_show_one_common(io_is_busy);
gov_show_one(dvfs, powersave_bias);

gov_attr_rw(sampling_rate);
gov_attr_rw(io_is_busy);
gov_attr_rw(up_threshold);
gov_attr_rw(sampling_down_factor);
gov_attr_rw(ignore_nice_load);
gov_attr_rw(powersave_bias);
gov_attr_ro(min_sampling_rate);

static struct attribute *dvfs_attributes[] = {
	&min_sampling_rate.attr,
	&sampling_rate.attr,
	&up_threshold.attr,
	&sampling_down_factor.attr,
	&ignore_nice_load.attr,
	&powersave_bias.attr,
	&io_is_busy.attr,
	NULL
};

/************************** sysfs end ************************/

static struct policy_dbs_info *dvfs_alloc(void)
{
	struct dvfs_policy_dbs_info *dbs_info;

	dbs_info = kzalloc(sizeof(*dbs_info), GFP_KERNEL);
	return dbs_info ? &dbs_info->policy_dbs : NULL;
}

static void dvfs_free(struct policy_dbs_info *policy_dbs)
{
	kfree(to_dbs_info(policy_dbs));
}

static int dvfs_init(struct dbs_data *dbs_data)
{
	struct dvfs_dbs_tuners *tuners;
	u64 idle_time;
	int cpu;

	tuners = kzalloc(sizeof(*tuners), GFP_KERNEL);
	if (!tuners)
		return -ENOMEM;

	cpu = get_cpu();
	idle_time = get_cpu_idle_time_us(cpu, NULL);
	put_cpu();
	if (idle_time != -1ULL) {
		/* Idle micro accounting is supported. Use finer thresholds */
		dbs_data->up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;
		/*
		 * In nohz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low).
		*/
		dbs_data->min_sampling_rate = MICRO_FREQUENCY_MIN_SAMPLE_RATE;
	} else {
		dbs_data->up_threshold = DEF_FREQUENCY_UP_THRESHOLD;

		/* For correct statistics, we need 10 ticks for each measure */
		dbs_data->min_sampling_rate = MIN_SAMPLING_RATE_RATIO *
			jiffies_to_usecs(10);
	}

	dbs_data->sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR;
	dbs_data->ignore_nice_load = 0;
	tuners->powersave_bias = default_powersave_bias;
	dbs_data->io_is_busy = should_io_be_busy();

	dbs_data->tuners = tuners;

        // Init timer for network monitoring
        // Call update_load_metrics function every 100 ms
        setup_timer(&network_timer, update_load_metrics, 0);
        mod_timer(&network_timer, jiffies + msecs_to_jiffies(100));
	return 0;
}

static void dvfs_exit(struct dbs_data *dbs_data)
{
	kfree(dbs_data->tuners);
    del_timer(&network_timer);
}

static void dvfs_start(struct cpufreq_policy *policy)
{
	struct dvfs_policy_dbs_info *dbs_info = to_dbs_info(policy->governor_data);

	dbs_info->sample_type = DVFS_NORMAL_SAMPLE;
	dvfs_powersave_bias_init(policy);
}

static struct dvfs_ops dvfs_ops = {
	.powersave_bias_target = generic_powersave_bias_target,
};

static struct dbs_governor dvfs_dbs_gov = {
	.gov = CPUFREQ_DBS_GOVERNOR_INITIALIZER("dvfs"),
	.kobj_type = { .default_attrs = dvfs_attributes },
	.gov_dbs_update = dvfs_dbs_update,
	.alloc = dvfs_alloc,
	.free = dvfs_free,
	.init = dvfs_init,
	.exit = dvfs_exit,
	.start = dvfs_start,
};

#define CPU_FREQ_GOV_DVFS	(&dvfs_dbs_gov.gov)

static void dvfs_set_powersave_bias(unsigned int powersave_bias)
{
	unsigned int cpu;
	cpumask_t done;

	default_powersave_bias = powersave_bias;
	cpumask_clear(&done);

	get_online_cpus();
	for_each_online_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct policy_dbs_info *policy_dbs;
		struct dbs_data *dbs_data;
		struct dvfs_dbs_tuners *dvfs_tuners;

		if (cpumask_test_cpu(cpu, &done))
			continue;

		policy = cpufreq_cpu_get_raw(cpu);
		if (!policy || policy->governor != CPU_FREQ_GOV_DVFS)
			continue;

		policy_dbs = policy->governor_data;
		if (!policy_dbs)
			continue;

		cpumask_or(&done, &done, policy->cpus);

		dbs_data = policy_dbs->dbs_data;
		dvfs_tuners = dbs_data->tuners;
		dvfs_tuners->powersave_bias = default_powersave_bias;
	}
	put_online_cpus();
}

void dvfs_register_powersave_bias_handler(unsigned int (*f)
		(struct cpufreq_policy *, unsigned int, unsigned int),
		unsigned int powersave_bias)
{
	dvfs_ops.powersave_bias_target = f;
	dvfs_set_powersave_bias(powersave_bias);
}
EXPORT_SYMBOL_GPL(dvfs_register_powersave_bias_handler);

void dvfs_unregister_powersave_bias_handler(void)
{
	dvfs_ops.powersave_bias_target = generic_powersave_bias_target;
	dvfs_set_powersave_bias(0);
}
EXPORT_SYMBOL_GPL(dvfs_unregister_powersave_bias_handler);

static int __init cpufreq_gov_dbs_init(void)
{
	unsigned int n_cpu, i;
	n_cpu = 0;
	for_each_online_cpu(i) {
		n_cpu++;
	}
	printk(KERN_INFO "DVFS governor __init - online CPUs : %u", n_cpu);

	return cpufreq_register_governor(CPU_FREQ_GOV_DVFS);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	cpufreq_unregister_governor(CPU_FREQ_GOV_DVFS);
}

MODULE_AUTHOR("Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>");
MODULE_AUTHOR("Alexey Starikovskiy <alexey.y.starikovskiy@intel.com>");
MODULE_AUTHOR("Erwan Beauvois <erwan@allmyt.eu>");
MODULE_DESCRIPTION("'cpufreq_dvfs' - A dynamic cpufreq governor that "
	"adapt cpu frequency to other resource's load (network, disk, memory)");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_DVFS
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return CPU_FREQ_GOV_DVFS;
}

fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
