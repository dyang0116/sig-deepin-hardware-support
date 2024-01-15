static const char drv_ver[]="v6.6-backport-5.15-4-g9405c0925";
#include <linux/module.h>
module_param_string(drv_ver, drv_ver, sizeof(drv_ver), 0444);
