static const char drv_ver[]="v6.6.1-backport-5.15-7-gf2b0a729d";
#include <linux/module.h>
module_param_string(drv_ver, drv_ver, sizeof(drv_ver), 0444);
