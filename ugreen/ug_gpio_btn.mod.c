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
	{ 0x92997ed8, "_printk" },
	{ 0xedc03953, "iounmap" },
	{ 0xdc0e4855, "timer_delete" },
	{ 0x97701647, "input_unregister_device" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x4fa9c394, "input_event" },
	{ 0xde80cd09, "ioremap" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x24d273d1, "add_timer" },
	{ 0xdb30cb5e, "devm_input_allocate_device" },
	{ 0xd0e64cfe, "input_set_capability" },
	{ 0x59cdcb49, "input_register_device" },
	{ 0x4d085fe6, "_dev_err" },
	{ 0xa313ff90, "platform_device_unregister" },
	{ 0x81e6b37f, "dmi_get_system_info" },
	{ 0x5a921311, "strncmp" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x7c983a5d, "dmi_walk" },
	{ 0x416133c6, "__platform_driver_register" },
	{ 0xe1adf503, "platform_device_register" },
	{ 0xfa066018, "platform_driver_unregister" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x5002fa32, "module_layout" },
};

MODULE_INFO(depends, "");

