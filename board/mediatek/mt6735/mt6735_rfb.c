// SPDX-License-Identifier: GPL-2.0

#include <dm.h>
#include <mmc.h>
#include <init.h>
#include <wdt.h>
#include <generic-phy.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/delay.h>
#include <configs/mt6735.h>
#include <power/pmic.h>

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
int g_dnl_board_usb_cable_connected(void)
{
	struct udevice *dev;
	int ret, val;

	ret = uclass_get_device(UCLASS_PWRAP, 0, &dev);
	if (ret) {
		pr_err("%s: Cannot find PWRAP device\n", __func__);
		return ret;
	}

	ret = uclass_get_device(UCLASS_PMIC, 0, &dev);
	if (ret) {
		pr_err("%s: Cannot find pmic device\n", __func__);
		return ret;
	}

	pmic_read_u32(dev, 0x0f48, &val);
	printf("PMIC CHR REG read value 0x%x\n", val);
	ret = (((val) & (0x1 << 5)) >> 5);
	if (ret == 1) {
		printf("pmic usb detected, %d\n", ret);
		return ret;
	} else {
		printf("pmic failed to detect USB\n");
	}

	return ret; /* Should return 1 on normal cases */
}

#endif


