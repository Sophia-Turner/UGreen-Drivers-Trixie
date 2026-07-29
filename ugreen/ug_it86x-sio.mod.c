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
	{ 0xdbdf6c92, "ioport_resource" },
	{ 0x85bd1608, "__request_region" },
	{ 0x1035c7c2, "__release_region" },
	{ 0x2f627328, "proc_mkdir" },
	{ 0x41c64d51, "proc_create_data" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x5a921311, "strncmp" },
	{ 0x5c3c7387, "kstrtoull" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x89940875, "mutex_lock_interruptible" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x619cb7dd, "simple_read_from_buffer" },
	{ 0xd61ecb12, "seq_release" },
	{ 0xbbbeda21, "param_ops_int" },
	{ 0xc4e07007, "remove_proc_entry" },
	{ 0x92997ed8, "_printk" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x581a7cd, "single_open" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0x5002fa32, "module_layout" },
};

MODULE_INFO(depends, "");

