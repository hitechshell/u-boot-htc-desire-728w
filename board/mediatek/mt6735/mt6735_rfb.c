// SPDX-License-Identifier: GPL-2.0

#include <dm.h>
#include <mmc.h>
#include <init.h>
#include <wdt.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/delay.h>
#include <configs/mt6735.h>
#include <pwrap/pwrap.h>
#include "mt6735_pwrap_hal.h"

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	gd->bd->bi_boot_params = CFG_SYS_SDRAM_BASE + 0x100;
	return 0;
}

#ifdef CONFIG_MMC
int mmc_get_boot_dev(void)
{
	int g_mmc_devid = -1;
	char *uflag = (char *)0x41DFFFF0;

	if (!find_mmc_device(1))
		return 0;

	if (strncmp(uflag,"eMMC",4)==0) {
		g_mmc_devid = 0;
		printf("Boot From Emmc(id:%d)\n\n", g_mmc_devid);
	} else {
		g_mmc_devid = 1;
		printf("Boot From SD(id:%d)\n\n", g_mmc_devid);
	}
	return g_mmc_devid;
}

int mmc_get_env_dev(void)
{
	return mmc_get_boot_dev();
}
#endif

#ifdef CONFIG_USB_MUSB_GADGET
int board_late_init(void)
{
	struct udevice *dev;
	int ret;

	if (CONFIG_IS_ENABLED(USB_GADGET)) {
		ret = uclass_get_device(UCLASS_USB_GADGET_GENERIC, 0, &dev);
		if (ret) {
			pr_err("%s: Cannot find USB device\n", __func__);
			return ret;
		}
	}

	return 0;
}
#endif


