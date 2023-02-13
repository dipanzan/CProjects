#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xf704969, "module_layout" },
	{ 0xc946dda0, "cdev_del" },
	{ 0xd731cdd9, "kmalloc_caches" },
	{ 0x2d725fd4, "cdev_init" },
	{ 0xe9aed6c4, "cpufreq_dbs_governor_start" },
	{ 0x3062d83b, "cpufreq_register_governor" },
	{ 0xa92ec74, "boot_cpu_data" },
	{ 0x82e7bb9c, "device_destroy" },
	{ 0x2d5f69b3, "rcu_read_unlock_strict" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x7a2af7b4, "cpu_number" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x7b9793a2, "get_cpu_idle_time_us" },
	{ 0xd7a71519, "__cpufreq_driver_target" },
	{ 0xcd2df77, "cpufreq_dbs_governor_limits" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xaa44a707, "cpumask_next" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x17de3d5, "nr_cpu_ids" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xa084749a, "__bitmap_or" },
	{ 0xfb578fc5, "memset" },
	{ 0x7f71c925, "cpufreq_table_index_unsorted" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x2c432b72, "cpufreq_unregister_governor" },
	{ 0x5a5a2271, "__cpu_online_mask" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0xefc94da8, "device_create" },
	{ 0xa04f945a, "cpus_read_lock" },
	{ 0x5d1151ba, "cpufreq_cpu_get_raw" },
	{ 0xc378cea7, "cdev_add" },
	{ 0xab0324d4, "init_task" },
	{ 0xa23650f3, "store_sampling_rate" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0xb5bf8857, "cpufreq_dbs_governor_init" },
	{ 0x92997ed8, "_printk" },
	{ 0x6dfd644f, "cpufreq_dbs_governor_exit" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x17aedcf8, "cpufreq_dbs_governor_stop" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x7c797b6, "kmem_cache_alloc_trace" },
	{ 0x18fb2caf, "cpus_read_unlock" },
	{ 0x37a0cba, "kfree" },
	{ 0x933c4a18, "class_destroy" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xfa8dc79d, "gov_update_cpu_data" },
	{ 0x325cb5cb, "__class_create" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x23b09657, "dbs_update" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "CBE9EBEDCA3DFDC78346993");
