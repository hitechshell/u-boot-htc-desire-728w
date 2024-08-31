/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MUSB OTG driver register I/O
 *
 * Copyright 2005 Mentor Graphics Corporation
 * Copyright (C) 2005-2006 by Texas Instruments
 * Copyright (C) 2006-2007 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef __MUSB_LINUX_PLATFORM_ARCH_H__
#define __MUSB_LINUX_PLATFORM_ARCH_H__

#include <linux/io.h>

#define musb_ep_select(_mbase, _epnum)	musb->io.ep_select((_mbase), (_epnum))
/* NOTE:  these offsets are all in bytes */


/**
 * struct musb_io - IO functions for MUSB
 * @ep_offset:	platform specific function to get end point offset
 * @ep_select:	platform specific function to select end point
 * @fifo_offset: platform specific function to get fifo offset
 * @read_fifo:	platform specific function to read fifo
 * @write_fifo:	platform specific function to write fifo
 * @busctl_offset: platform specific function to get busctl offset
 * @get_toggle: platform specific function to get toggle
 * @set_toggle: platform specific function to set toggle
 */
struct musb_io {
	u32	(*ep_offset)(u8 epnum, u16 offset);
	void	(*ep_select)(void __iomem *mbase, u8 epnum);
	u32	(*fifo_offset)(u8 epnum);
	void	(*read_fifo)(struct musb_hw_ep *hw_ep, u16 len, u8 *buf);
	void	(*write_fifo)(struct musb_hw_ep *hw_ep, u16 len, const u8 *buf);
	u32	(*busctl_offset)(u8 epnum, u16 offset);
	u16	(*get_toggle)(struct musb_qh *qh, int is_out);
	u16	(*set_toggle)(struct musb_qh *qh, int is_out, struct urb *urb);
};

static inline u16 musb_readw(const void __iomem *addr, unsigned offset)
	{ return __raw_readw(addr + offset); }

static inline u32 musb_readl(const void __iomem *addr, unsigned offset)
	{ return __raw_readl(addr + offset); }

static inline void musb_writew(void __iomem *addr, unsigned offset, u16 data)
	{ __raw_writew(data, addr + offset); }

static inline void musb_writel(void __iomem *addr, unsigned offset, u32 data)
	{ __raw_writel(data, addr + offset); }

#if defined(CONFIG_USB_MUSB_TUSB6010) || defined (CONFIG_USB_MUSB_TUSB6010_MODULE)

/*
 * TUSB6010 doesn't allow 8-bit access; 16-bit access is the minimum.
 */
static inline u8 musb_readb(const void __iomem *addr, unsigned offset)
{
	u16 tmp;
	u8 val;

	tmp = __raw_readw(addr + (offset & ~1));
	if (offset & 1)
		val = (tmp >> 8);
	else
		val = tmp & 0xff;

	return val;
}

static inline void musb_writeb(void __iomem *addr, unsigned offset, u8 data)
{
	u16 tmp;

	tmp = __raw_readw(addr + (offset & ~1));
	if (offset & 1)
		tmp = (data << 8) | (tmp & 0xff);
	else
		tmp = (tmp & 0xff00) | data;

	__raw_writew(tmp, addr + (offset & ~1));
}

#else

static inline u8 musb_readb(const void __iomem *addr, unsigned offset)
	{ return __raw_readb(addr + offset); }

static inline void musb_writeb(void __iomem *addr, unsigned offset, u8 data)
	{ __raw_writeb(data, addr + offset); }

#endif	/* CONFIG_USB_MUSB_TUSB6010 */

#endif
