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
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xf9a482f9, "msleep" },
	{ 0xf1c7f7dc, "i2c_smbus_read_byte_data" },
	{ 0xb78a941, "i2c_smbus_read_i2c_block_data" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x5c3c7387, "kstrtoull" },
	{ 0xac1a55be, "unregister_reboot_notifier" },
	{ 0x547e9aed, "i2c_del_driver" },
	{ 0xea3f1479, "i2c_unregister_device" },
	{ 0xbc013b6d, "i2c_get_adapter" },
	{ 0x5a921311, "strncmp" },
	{ 0x27771e9, "i2c_put_adapter" },
	{ 0xfb6a8e4a, "i2c_new_client_device" },
	{ 0xf675ab32, "i2c_smbus_read_word_data" },
	{ 0xaf75bf13, "i2c_register_driver" },
	{ 0x81784bdb, "_dev_info" },
	{ 0x9166fada, "strncpy" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0x81e6b37f, "dmi_get_system_info" },
	{ 0x3517383e, "register_reboot_notifier" },
	{ 0xa05da9e1, "devm_kmalloc" },
	{ 0x70b1c405, "led_classdev_register_ext" },
	{ 0x4d085fe6, "_dev_err" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x3c12dfe, "cancel_work_sync" },
	{ 0x2b94567f, "led_classdev_unregister" },
	{ 0xad97334b, "devm_kfree" },
	{ 0x92997ed8, "_printk" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x7794c430, "i2c_smbus_write_block_data" },
	{ 0x5002fa32, "module_layout" },
};

MODULE_INFO(depends, "");

