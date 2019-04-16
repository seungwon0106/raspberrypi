#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
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
__used
__attribute__((section("__versions"))) = {
	{ 0x367398b6, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xfe990052, __VMLINUX_SYMBOL_STR(gpio_free) },
	{ 0xc1514a3b, __VMLINUX_SYMBOL_STR(free_irq) },
	{ 0x5eaebe1a, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0xd6b8e852, __VMLINUX_SYMBOL_STR(request_threaded_irq) },
	{ 0xc5d3d598, __VMLINUX_SYMBOL_STR(gpiod_to_irq) },
	{ 0x3ee15874, __VMLINUX_SYMBOL_STR(gpio_to_desc) },
	{ 0x47229b5c, __VMLINUX_SYMBOL_STR(gpio_request) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x1a96fe1c, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xdb7c2b58, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0xcbbf3ca9, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x7e20ba1d, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x12d86225, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xede71573, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xd8e484f0, __VMLINUX_SYMBOL_STR(register_chrdev_region) },
	{ 0x1e047854, __VMLINUX_SYMBOL_STR(warn_slowpath_fmt) },
	{ 0x2121b914, __VMLINUX_SYMBOL_STR(pid_task) },
	{ 0xe9340513, __VMLINUX_SYMBOL_STR(find_vpid) },
	{ 0xb742fd7, __VMLINUX_SYMBOL_STR(simple_strtol) },
	{ 0x28cc25db, __VMLINUX_SYMBOL_STR(arm_copy_from_user) },
	{ 0x4a74c285, __VMLINUX_SYMBOL_STR(try_module_get) },
	{ 0xa61e767e, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0xbb08e14e, __VMLINUX_SYMBOL_STR(send_sig_info) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0xe6932195, __VMLINUX_SYMBOL_STR(hrtimer_start_range_ns) },
	{ 0xce58dc69, __VMLINUX_SYMBOL_STR(hrtimer_init) },
	{ 0x2e5810c6, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr1) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xb1ad28e0, __VMLINUX_SYMBOL_STR(__gnu_mcount_nc) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "255EC3C21F8658C3A182B0A");
