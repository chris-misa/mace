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

#ifdef RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xe8aad16f, "module_layout" },
	{ 0x317eed9c, "param_ops_int" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x2b24963c, "tracepoint_probe_unregister" },
	{ 0xaa2862b9, "for_each_kernel_tracepoint" },
	{ 0xf4cf7ac1, "__register_chrdev" },
	{ 0x37a0cba, "kfree" },
	{ 0x8c7d81de, "kmem_cache_alloc_trace" },
	{ 0xbc5cde86, "kmalloc_caches" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0xb18fc00, "current_task" },
	{ 0xdb7305a1, "__stack_chk_fail" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x1000e51, "schedule" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xc3aaf0a9, "__put_user_1" },
	{ 0x91715312, "sprintf" },
	{ 0x50026d26, "try_module_get" },
	{ 0x56f28f13, "module_put" },
	{ 0x5d6dbea7, "tracepoint_probe_register" },
	{ 0x27e1a049, "printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "A34920F065D61E684B3D78D");
