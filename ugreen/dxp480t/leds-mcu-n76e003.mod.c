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
	{ 0xf1c7f7dc, "i2c_smbus_read_byte_data" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x81784bdb, "_dev_info" },
	{ 0x58b250e6, "i2c_smbus_write_byte_data" },
	{ 0x4d085fe6, "_dev_err" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xa05da9e1, "devm_kmalloc" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0x70b1c405, "led_classdev_register_ext" },
	{ 0x5c3c7387, "kstrtoull" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x5a921311, "strncmp" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xaf75bf13, "i2c_register_driver" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x3c12dfe, "cancel_work_sync" },
	{ 0x2b94567f, "led_classdev_unregister" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x547e9aed, "i2c_del_driver" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x5002fa32, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("i2c:mcu_led");
