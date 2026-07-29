#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x2d3385d3, "system_wq" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0xf2d03b12, "dev_get_stats" },
	{ 0xdbdcb3c1, "led_set_brightness" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x5c3c7387, "kstrtoull" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x69acdf38, "memcpy" },
	{ 0x5a921311, "strncmp" },
	{ 0xe177515f, "init_net" },
	{ 0xd70c3649, "dev_get_by_name" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x319bae02, "led_trigger_register" },
	{ 0xc3690fc, "_raw_spin_lock_bh" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xe46021ca, "_raw_spin_unlock_bh" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x37befc70, "jiffies_to_msecs" },
	{ 0x9d0d6206, "unregister_netdevice_notifier" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x37a0cba, "kfree" },
	{ 0x92997ed8, "_printk" },
	{ 0xff2e7805, "led_trigger_unregister" },
	{ 0xa12d2260, "kmalloc_caches" },
	{ 0xc25f21b, "__kmalloc_cache_noprof" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xd2da1048, "register_netdevice_notifier" },
	{ 0xaf21be06, "led_blink_set_oneshot" },
	{ 0x5002fa32, "module_layout" },
};

MODULE_INFO(depends, "");

