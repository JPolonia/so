#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const struct modversion_info ____versions[]
__attribute_used__
__attribute__((section("__versions"))) = {
	{ 0x8d0ae4f, "struct_module" },
	{ 0x133d4e85, "no_llseek" },
	{ 0x89b301d4, "param_get_int" },
	{ 0x98bd6f46, "param_set_int" },
	{ 0x2323fd40, "nonseekable_open" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xd85a396a, "cdev_add" },
	{ 0xbb37bf2e, "cdev_init" },
	{ 0x8523fd53, "kmem_cache_alloc" },
	{ 0xb13d73c9, "malloc_sizes" },
	{ 0xd8e484f0, "register_chrdev_region" },
	{ 0x1b7d4074, "printk" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x37a0cba, "kfree" },
	{ 0x4bd6e07e, "cdev_del" },
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";

