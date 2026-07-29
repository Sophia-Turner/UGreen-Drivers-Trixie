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
	{ 0x41c64d51, "proc_create_data" },
	{ 0xdc0e4855, "timer_delete" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x92997ed8, "_printk" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x24d273d1, "add_timer" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0x5a921311, "strncmp" },
	{ 0xde80cd09, "ioremap" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x2f627328, "proc_mkdir" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x2cf56265, "__dynamic_pr_debug" },
	{ 0xc4e07007, "remove_proc_entry" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x581a7cd, "single_open" },
	{ 0x619cb7dd, "simple_read_from_buffer" },
	{ 0xf9a482f9, "msleep" },
	{ 0x2d3385d3, "system_wq" },
	{ 0x81e6b37f, "dmi_get_system_info" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0xd61ecb12, "seq_release" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xedc03953, "iounmap" },
	{ 0x5002fa32, "module_layout" },
};

MODULE_INFO(depends, "");

