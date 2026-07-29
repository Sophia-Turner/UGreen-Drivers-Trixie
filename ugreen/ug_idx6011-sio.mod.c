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
	{ 0x1035c7c2, "__release_region" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x92997ed8, "_printk" },
	{ 0x96b29254, "strncasecmp" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x7682ba4e, "__copy_overflow" },
	{ 0x8c8569cb, "kstrtoint" },
	{ 0xde80cd09, "ioremap" },
	{ 0x2f627328, "proc_mkdir" },
	{ 0xdbdf6c92, "ioport_resource" },
	{ 0x5c3c7387, "kstrtoull" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x2cf56265, "__dynamic_pr_debug" },
	{ 0xc4e07007, "remove_proc_entry" },
	{ 0xbbbeda21, "param_ops_int" },
	{ 0x581a7cd, "single_open" },
	{ 0x619cb7dd, "simple_read_from_buffer" },
	{ 0xf9a482f9, "msleep" },
	{ 0x85bd1608, "__request_region" },
	{ 0x81e6b37f, "dmi_get_system_info" },
	{ 0xa78af5f3, "ioread32" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x4a453f53, "iowrite32" },
	{ 0xd61ecb12, "seq_release" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xedc03953, "iounmap" },
	{ 0x41c64d51, "proc_create_data" },
	{ 0x5002fa32, "module_layout" },
};

MODULE_INFO(depends, "");

