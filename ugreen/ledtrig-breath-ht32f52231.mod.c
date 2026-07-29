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
	{ 0x37a0cba, "kfree" },
	{ 0xdbdcb3c1, "led_set_brightness" },
	{ 0xff2e7805, "led_trigger_unregister" },
	{ 0xa12d2260, "kmalloc_caches" },
	{ 0xc25f21b, "__kmalloc_cache_noprof" },
	{ 0x2f2c95c4, "flush_work" },
	{ 0xf9a482f9, "msleep" },
	{ 0xc784ca2a, "led_get_default_pattern" },
	{ 0x29a5f62d, "_dev_warn" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x319bae02, "led_trigger_register" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x5c3c7387, "kstrtoull" },
	{ 0x9510b0c0, "led_blink_set" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x5002fa32, "module_layout" },
};

MODULE_INFO(depends, "");

