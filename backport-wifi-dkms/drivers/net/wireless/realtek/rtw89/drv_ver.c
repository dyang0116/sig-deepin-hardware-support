static const char drv_ver[]="v6.6.1-backport-5.15-9-gfcb4f961c";
#include <linux/module.h>
module_param_string(drv_ver, drv_ver, sizeof(drv_ver), 0444);
