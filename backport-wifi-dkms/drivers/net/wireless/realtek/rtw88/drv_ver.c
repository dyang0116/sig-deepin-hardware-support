#define VERSTR "v6.8-backport-5.15-9-gd91b9d2d"

static char *drv_ver = VERSTR;
#include <linux/module.h>
module_param(drv_ver, charp, 0444);
MODULE_PARM_DESC(drv_ver, VERSTR);

static int __init rtw88_drv_init(void)
{
	printk("rtw88 drv init version: %s\n", VERSTR);

	return 0;
}

static void __exit rtw88_drv_exit(void)
{
	printk("rtw88 drv exit\n");
}

module_init(rtw88_drv_init);
module_exit(rtw88_drv_exit);
