// SPDX-License-Identifier: GPL-2.0
/*
 * Mediatek "glue layer"
 *
 * Copyright (C) 2019-2021 by Mediatek
 * Based on the AllWinner SUNXI "glue layer" code.
 * Copyright (C) 2015 Hans de Goede <hdegoede@redhat.com>
 * Copyright (C) 2013 Jussi Kivilinna <jussi.kivilinna@iki.fi>
 *
 * This file is part of the Inventra Controller Driver for Linux.
 */
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <dm/pinctrl.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/usb/musb.h>
#include <generic-phy.h>
#include <power/regulator.h>
#include <usb.h>
#include "linux-compat.h"
#include "musb_core.h"
#include "musb_uboot.h"

#define PWR_ENABLE_SUSPENDM  (1<<0)
#define PWR_SOFT_CONN        (1<<6)
#define PWR_HS_ENAB          (1<<5)

#define USB_L1INTS			0x00a0
#define USB_L1INTM			0x00a4

#define TX_INT_STATUS		BIT(0)
#define RX_INT_STATUS		BIT(1)
#define USBCOM_INT_STATUS	BIT(2)

struct mtk_musb_config {
	struct musb_hdrc_config *config;
};

struct mtk_musb_glue {
	struct musb_host_data mdata;
	struct clk usbpllclk;
	struct clk usbmcuclk;
	struct clk usbclk;
	struct mtk_musb_config *cfg;
	struct device dev;
	struct udevice *vusb33_supply;
	struct phy phy;
};

#define to_mtk_musb_glue(d)	container_of(d, struct mtk_musb_glue, dev)

/******************************************************************************
 * MUSB Glue code
 ******************************************************************************/

static irqreturn_t generic_interrupt(int irq, void *__hci)
{
	unsigned long flags;
	irqreturn_t retval = IRQ_NONE;
	struct musb *musb = __hci;

	spin_lock_irqsave(&musb->lock, flags);
	musb->int_usb = musb_readb(musb->mregs, MUSB_INTRUSB);
	musb->int_rx = musb_readw(musb->mregs, MUSB_INTRRX);
	musb->int_tx = musb_readw(musb->mregs, MUSB_INTRTX);

	if ((musb->int_usb & MUSB_INTR_RESET) && !is_host_active(musb)) {
		/* ep0 FADDR must be 0 when (re)entering peripheral mode */
		musb_ep_select(musb->mregs, 0);
		musb_writeb(musb->mregs, MUSB_FADDR, 0);
	}

	if (musb->int_usb || musb->int_tx || musb->int_rx)
		retval = musb_interrupt(musb);

	spin_unlock_irqrestore(&musb->lock, flags);

	return retval;
}

static irqreturn_t mtk_musb_interrupt(int irq, void *dev_id)
{
	irqreturn_t retval = IRQ_NONE;
	struct musb *musb = (struct musb *)dev_id;
	u32 l1_ints;

	l1_ints = musb_readl(musb->mregs, USB_L1INTS) &
	musb_readl(musb->mregs, USB_L1INTM);

	if (l1_ints & (TX_INT_STATUS | RX_INT_STATUS | USBCOM_INT_STATUS))
		retval = generic_interrupt(irq, musb);

	#if defined(CONFIG_USB_INVENTRA_DMA)
	if (l1_ints & DMA_INT_STATUS)
		retval = dma_controller_irq(irq, musb->dma_controller);
	#endif
	return retval;
}


/* musb_core does not call enable / disable in a balanced manner <sigh> */
static bool enabled;

static int mtk_musb_enable(struct musb *musb)
{
	struct mtk_musb_glue *glue = to_mtk_musb_glue(musb->controller);
	int ret;

	printf("%s():\n", __func__);

	musb_ep_select(musb->mregs, 0);
	musb_writeb(musb->mregs, MUSB_FADDR, 0);

	if (enabled)
		return 0;

	ret = generic_phy_power_on(&glue->phy);
	if (ret) {
		pr_debug("failed to power on USB PHY\n");
		return ret;
	}
	regulator_set_enable(glue->vusb33_supply, true);
	enabled = true;

	return 0;
}

static void mtk_musb_disable(struct musb *musb)
{
	struct mtk_musb_glue *glue = to_mtk_musb_glue(musb->controller);
	int ret;

	printf("%s():\n", __func__);

	if (!enabled)
		return;

	if (is_host_enabled(musb)) {
		ret = generic_phy_power_off(&glue->phy);
		if (ret) {
			pr_debug("failed to power off USB PHY\n");
			return;
		}
	}

	enabled = false;
}

static int mtk_musb_init(struct musb *musb)
{
	struct mtk_musb_glue *glue = to_mtk_musb_glue(musb->controller);
	int ret;
	u8 tmp;

	printf("%s():\n", __func__);

	ret = clk_enable(&glue->usbpllclk);
	if (ret) {
		dev_err(musb->controller, "failed to enable usbpll clock\n");
		return ret;
	}
	ret = clk_enable(&glue->usbmcuclk);
	if (ret) {
		dev_err(musb->controller, "failed to enable usbmcu clock\n");
		return ret;
	}
	ret = clk_enable(&glue->usbclk);
	if (ret) {
		dev_err(musb->controller, "failed to enable usb clock\n");
		return ret;
	}

	ret = generic_phy_init(&glue->phy);
	if (ret) {
		dev_dbg(musb->controller, "failed to init USB PHY\n");
		return ret;
	}

	musb->isr = mtk_musb_interrupt;

	/* turning on the USB core cuz musb_core wouldn't */
	tmp = musb_readb(musb->mregs, 0x1);
	tmp |= PWR_SOFT_CONN;
	tmp |= PWR_ENABLE_SUSPENDM;
	tmp |= PWR_HS_ENAB;
	musb_writeb(musb->mregs, 0x1, tmp);

	return 0;
}

static int mtk_musb_exit(struct musb *musb)
{
	struct mtk_musb_glue *glue = to_mtk_musb_glue(musb->controller);
	int ret;
	u8 tmp;

	/* turning off the USB core cuz the musb_core wouldn't */
	tmp = musb_readb(musb->mregs, 0x1);
	tmp &= ~PWR_SOFT_CONN;
	musb_writeb(musb->mregs, 0x1, tmp);

	if (generic_phy_valid(&glue->phy)) {
		ret = generic_phy_exit(&glue->phy);
		if (ret) {
			dev_dbg(musb->controller,
					"failed to power off usb phy\n");
			return ret;
		}
	}

	clk_disable(&glue->usbclk);
	clk_disable(&glue->usbmcuclk);
	clk_disable(&glue->usbpllclk);
	regulator_set_enable(glue->vusb33_supply, false);
	return 0;
}

static const struct musb_platform_ops mtk_musb_ops = {
	.init		= mtk_musb_init,
	.exit		= mtk_musb_exit,
	.enable		= mtk_musb_enable,
	.disable	= mtk_musb_disable,
};

/* MTK OTG supports up to 7 endpoints */
#define MTK_MUSB_MAX_EP_NUM		8
#define MTK_MUSB_RAM_BITS		16

static struct musb_fifo_cfg mtk_musb_mode_cfg[] = {
	{ .hw_ep_num = 1, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 1, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 2, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 2, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 3, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 3, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 4, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 4, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 5, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 5, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 6, .style = FIFO_TX, .maxpacket = 1024, },
	{ .hw_ep_num = 6, .style = FIFO_RX, .maxpacket = 1024, },
	{ .hw_ep_num = 7, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 7, .style = FIFO_RX, .maxpacket = 64, },

};

static struct musb_hdrc_config musb_config = {
	.fifo_cfg       = mtk_musb_mode_cfg,
	.fifo_cfg_size  = ARRAY_SIZE(mtk_musb_mode_cfg),
	.multipoint	= true,
	.dyn_fifo	= true,
	.num_eps	= MTK_MUSB_MAX_EP_NUM,
	.ram_bits	= MTK_MUSB_RAM_BITS,
};

static int musb_usb_probe(struct udevice *dev)
{
	struct mtk_musb_glue *glue = dev_get_priv(dev);
	struct musb_host_data *host = &glue->mdata;
	struct musb_hdrc_platform_data pdata;
	void *base = dev_read_addr_ptr(dev);
	int ret;

	printf("%s():\n", __func__);

#ifdef CONFIG_USB_MUSB_HOST
	struct usb_bus_priv *priv = dev_get_uclass_priv(dev);
#endif

	if (!base)
		return -EINVAL;

	glue->cfg = (struct mtk_musb_config *)dev_get_driver_data(dev);
	if (!glue->cfg)
		return -EINVAL;

	ret = clk_get_by_name(dev, "usbpll", &glue->usbpllclk);
	if (ret) {
		dev_err(dev, "failed to get usbpll clock\n");
		return ret;
	}
	ret = clk_get_by_name(dev, "usbmcu", &glue->usbmcuclk);
	if (ret) {
		dev_err(dev, "failed to get usbmcu clock\n");
		return ret;
	}
	ret = clk_get_by_name(dev, "usb", &glue->usbclk);
	if (ret) {
		dev_err(dev, "failed to get usb clock\n");
		return ret;
	}
	ret = generic_phy_get_by_name(dev, "usb", &glue->phy);
	if (ret) {
		pr_err("failed to get usb PHY\n");
		return ret;
	}

	ret = device_get_supply_regulator(dev, "vusb33-supply",
									  &glue->vusb33_supply);
	if (ret) {
		debug("can't get vusb33 regulator %d!\n", ret);
	}

	ret = generic_phy_power_on(&glue->phy);
	if (ret) {
		pr_debug("failed to power on USB PHY\n");
		return ret;
	}

	memset(&pdata, 0, sizeof(pdata));
	pdata.power = (u8)400;
	pdata.platform_ops = &mtk_musb_ops;
	pdata.config = glue->cfg->config;

#ifdef CONFIG_USB_MUSB_HOST
	priv->desc_before_addr = true;
	pinctrl_select_state(dev, "host");
	pdata.mode = MUSB_HOST;
	host->host = musb_init_controller(&pdata, &glue->dev, base);
	if (!host->host)
		return -EIO;

	ret = musb_lowlevel_init(host);
	if (!ret)
		printf("MTK MUSB OTG (Host)\n");

	ret = generic_phy_set_mode(&glue->phy, PHY_MODE_USB_HOST, 0);
	if (ret) {
		printf("Setting USB PHY Host mode failed with error %d\n", ret);
		return ret;
	}
	MUSB_HST_MODE(musb);
#else

	pinctrl_select_state(dev, "peripheral");
	pdata.mode = MUSB_PERIPHERAL;
	host->host = musb_init_controller(&pdata, &glue->dev, base);
	if (!host->host)
		return -EIO;

	ret = generic_phy_set_mode(&glue->phy, PHY_MODE_USB_DEVICE, 0);
	if (ret) {
		printf("Setting USB PHY Peripheral mode failed with error %d\n", ret);
		return ret;
	}
	printf("MTK MUSB OTG (Peripheral)\n");
	return usb_add_gadget_udc(&glue->dev, &host->host->g);
#endif

	return ret;
}

static int musb_usb_remove(struct udevice *dev)
{
	struct mtk_musb_glue *glue = dev_get_priv(dev);
	struct musb_host_data *host = &glue->mdata;

	musb_stop(host->host);
	free(host->host);
	host->host = NULL;

	return 0;
}

static const struct mtk_musb_config mtk_cfg = {
	.config = &musb_config,
};

static const struct udevice_id mtk_musb_ids[] = {
	{ .compatible = "mediatek,mtk-musb",
	  .data = (ulong)&mtk_cfg },
	{ }
};

U_BOOT_DRIVER(mtk_musb) = {
	.name		= "mtk_musb",
#ifdef CONFIG_USB_MUSB_HOST
	.id		= UCLASS_USB,
#else
	.id		= UCLASS_USB_GADGET_GENERIC,
#endif
	.of_match	= mtk_musb_ids,
	.probe		= musb_usb_probe,
	.remove		= musb_usb_remove,
#ifdef CONFIG_USB_MUSB_HOST
	.ops		= &musb_usb_ops,
#endif
	.plat_auto	= sizeof(struct usb_plat),
	.priv_auto	= sizeof(struct mtk_musb_glue),
};
