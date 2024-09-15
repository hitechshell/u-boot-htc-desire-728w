// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <usb.h>
#include <linux/stat.h>
#include <linux/usb/composite.h>

#include "musb_core.h"
#include "musb_host.h"
#include "musb_qmu.h"

static unsigned int host_qmu_tx_max_active_isoc_gpd[MAX_QMU_EP + 1];
static unsigned int host_qmu_tx_max_number_of_pkts[MAX_QMU_EP + 1];
static unsigned int host_qmu_rx_max_active_isoc_gpd[MAX_QMU_EP + 1];
static unsigned int host_qmu_rx_max_number_of_pkts[MAX_QMU_EP + 1];
#define MTK_HOST_ACTIVE_DEV_TABLE_SZ 128
static u8 mtk_host_active_dev_table[MTK_HOST_ACTIVE_DEV_TABLE_SZ];

void mtk_host_active_dev_resource_reset(void)
{
	memset(mtk_host_active_dev_table, 0, sizeof(mtk_host_active_dev_table));
	mtk_host_active_dev_cnt = 0;
}

void musb_host_active_dev_add(unsigned int addr)
{
	pr_debug("devnum:%d\n", addr);

	if (addr < MTK_HOST_ACTIVE_DEV_TABLE_SZ &&
		!mtk_host_active_dev_table[addr]) {
		mtk_host_active_dev_table[addr] = 1;
	mtk_host_active_dev_cnt++;
		}
}

void __iomem *qmu_base;
int mtk_host_active_dev_cnt;
int musb_qmu_init(struct musb *musb)
{
	/* set DMA channel 0 burst mode to boost QMU speed */
	musb_writel(musb->mregs, 0x204, musb_readl(musb->mregs, 0x204) | 0x600);
	/* eanble DMA channel 0 36-BIT support */
	musb_writel(musb->mregs, 0x204,
				musb_readl(musb->mregs, 0x204) | 0x4000);

	/* make IOC field in GPD workable */
	musb_writel((musb->mregs + MUSB_QISAR), 0x30, 0);

	qmu_base = (void __iomem *)(musb->mregs + MUSB_QMUBASE);

	/* finish all hw op before init qmu */
	mb();

	if (qmu_init_gpd_pool(musb->controller)) {
		QMU_ERR("[QMU]qmu_init_gpd_pool fail\n");
		return -1;
	}

	return 0;
}

void musb_qmu_exit(struct musb *musb)
{
	/* need to find a proper way to dma_free_coherent */
}

void musb_disable_q_all(struct musb *musb)
{
	u32 ep_num;

	QMU_WARN("disable_q_all\n");
	mtk_host_active_dev_resource_reset();

	for (ep_num = 1; ep_num <= RXQ_NUM; ep_num++) {
		if (mtk_is_qmu_enabled(ep_num, RXQ))
			mtk_disable_q(musb, ep_num, 1);

		/* reset statistics */
		host_qmu_rx_max_active_isoc_gpd[ep_num] = 0;
		host_qmu_rx_max_number_of_pkts[ep_num] = 0;
	}
	for (ep_num = 1; ep_num <= TXQ_NUM; ep_num++) {
		if (mtk_is_qmu_enabled(ep_num, TXQ))
			mtk_disable_q(musb, ep_num, 0);

		/* reset statistics */
		host_qmu_tx_max_active_isoc_gpd[ep_num] = 0;
		host_qmu_tx_max_number_of_pkts[ep_num] = 0;
	}
}

void musb_kick_D_CmdQ(struct musb *musb, struct musb_request *request)
{
	int isRx;

	isRx = request->tx ? 0 : 1;
	/* enable qmu at musb_gadget_eanble */
	if (!mtk_is_qmu_enabled(request->epnum, isRx)) {
		/* enable qmu */
		mtk_qmu_enable(musb, request->epnum, isRx);
	}
	/* note tx needed additional zlp field */
	mtk_qmu_insert_task(request->epnum,
						isRx,
					 (dma_addr_t *)request->request.dma,
					 request->request.length,
					 ((request->request.zero == 1) ? 1 : 0), 1);

	mtk_qmu_resume(request->epnum, isRx);
}

irqreturn_t musb_q_irq(struct musb *musb)
{

	irqreturn_t retval = IRQ_NONE;
	u32 wQmuVal = musb->int_queue;
	int i;

	QMU_INFO("wQmuVal:%d\n", wQmuVal);
	for (i = 1; i <= MAX_QMU_EP; i++) {
		if (wQmuVal & DQMU_M_RX_DONE(i)) {
			if (!musb->is_host)
				qmu_done_rx(musb, i);
			else
				h_qmu_done_rx(musb, i);
		}
		if (wQmuVal & DQMU_M_TX_DONE(i)) {
			if (!musb->is_host)
				qmu_done_tx(musb, i);
			else
				h_qmu_done_tx(musb, i);
		}
	}

	mtk_qmu_irq_err(musb, wQmuVal);

	return retval;
}

void musb_flush_qmu(u32 ep_num, u8 isRx)
{
	QMU_DBG("flush %s(%d)\n", isRx ? "RQ" : "TQ", ep_num);
	mtk_qmu_stop(ep_num, isRx);
	qmu_reset_gpd_pool(ep_num, isRx);
}

void musb_restart_qmu(struct musb *musb, u32 ep_num, u8 isRx)
{
	QMU_DBG("restart %s(%d)\n", isRx ? "RQ" : "TQ", ep_num);
	flush_ep_csr(musb, ep_num, isRx);
	mtk_qmu_enable(musb, ep_num, isRx);
}

bool musb_is_qmu_stop(u32 ep_num, u8 isRx)
{
	void __iomem *base = qmu_base;

	if (!isRx) {
		if (MGC_ReadQMU16(base, MGC_O_QMU_TQCSR(ep_num))
			& DQMU_QUE_ACTIVE)
			return false;
		else
			return true;
	} else {
		if (MGC_ReadQMU16(base, MGC_O_QMU_RQCSR(ep_num))
			& DQMU_QUE_ACTIVE)
			return false;
		else
			return true;
	}
}

void musb_tx_zlp_qmu(struct musb *musb, u32 ep_num)
{
	/* sent ZLP through PIO */
	void __iomem *epio = musb->endpoints[ep_num].regs;
	void __iomem *mbase = musb->mregs;
	int cnt = 50; /* 50*200us, total 10 ms */
	int is_timeout = 1;
	u16 csr;

	QMU_WARN("TX ZLP direct sent\n");
	musb_ep_select(mbase, ep_num);

	/* disable dma for pio */
	csr = musb_readw(epio, MUSB_TXCSR);
	csr &= ~MUSB_TXCSR_DMAENAB;
	musb_writew(epio, MUSB_TXCSR, csr);

	/* TXPKTRDY */
	csr = musb_readw(epio, MUSB_TXCSR);
	csr |= MUSB_TXCSR_TXPKTRDY;
	musb_writew(epio, MUSB_TXCSR, csr);

	/* wait ZLP sent */
	while (cnt--) {
		csr = musb_readw(epio, MUSB_TXCSR);
		if (!(csr & MUSB_TXCSR_TXPKTRDY)) {
			is_timeout = 0;
			break;
		}
		udelay(200);
	}

	/* re-enable dma for qmu */
	csr = musb_readw(epio, MUSB_TXCSR);
	csr |= MUSB_TXCSR_DMAENAB;
	musb_writew(epio, MUSB_TXCSR, csr);

	if (is_timeout)
		QMU_ERR("TX ZLP sent fail???\n");
	QMU_WARN("TX ZLP sent done\n");
}

int mtk_kick_CmdQ(struct musb *musb,
				  int isRx, struct musb_qh *qh, struct urb *urb)
{
	void __iomem        *mbase = musb->mregs;
	u16 intr_e = 0;
	struct musb_hw_ep	*hw_ep = qh->hw_ep;
	void __iomem		*epio = hw_ep->regs;
	unsigned int offset = 0;
	u8 bIsIoc;
	dma_addr_t pBuffer;
	u32 dwLength;
	u16 i;
	u32 gpd_free_count = 0;

	if (!urb) {
		QMU_WARN("!urb\n");
		return -1; /*KOBE : should we return a value */
	}

	if (!mtk_is_qmu_enabled(hw_ep->epnum, isRx)) {
		pr_debug("! mtk_is_qmu_enabled<%d,%s>\n",
			hw_ep->epnum, isRx?"RXQ":"TXQ");

		musb_ep_select(mbase, hw_ep->epnum);
		flush_ep_csr(musb, hw_ep->epnum,  isRx);

		if (isRx) {
			pr_debug("isRX = 1\n");
			if (qh->type == USB_ENDPOINT_XFER_ISOC) {
				pr_debug("USB_ENDPOINT_XFER_ISOC\n");
				if (qh->hb_mult == 3)
					musb_writew(epio, MUSB_RXMAXP,
								qh->maxpacket|0x1000);
					else if (qh->hb_mult == 2)
						musb_writew(epio, MUSB_RXMAXP,
									qh->maxpacket|0x800);
						else
							musb_writew(epio, MUSB_RXMAXP,
										qh->maxpacket);
			} else {
				pr_debug("!! USB_ENDPOINT_XFER_ISOC\n");
				musb_writew(epio, MUSB_RXMAXP, qh->maxpacket);
			}

			musb_writew(epio, MUSB_RXCSR, MUSB_RXCSR_DMAENAB);
			/*CC: speed */
			musb_writeb(epio, MUSB_RXTYPE,
						(qh->type_reg | usb_pipeendpoint(urb->pipe)));
			musb_writeb(epio, MUSB_RXINTERVAL, qh->intv_reg);
			if (musb->is_multipoint) {
				pr_debug("is_multipoint\n");
				musb_write_rxfunaddr(musb->mregs, hw_ep->epnum,
									 qh->addr_reg);
				musb_write_rxhubaddr(musb->mregs, hw_ep->epnum,
									 qh->h_addr_reg);
				musb_write_rxhubport(musb->mregs, hw_ep->epnum,
									 qh->h_port_reg);
			} else {
				pr_debug("!! is_multipoint\n");
				musb_writeb(musb->mregs, MUSB_FADDR,
							qh->addr_reg);
			}
			/*turn off intrRx*/
			intr_e = musb_readw(musb->mregs, MUSB_INTRRXE);
			intr_e = intr_e & (~(1<<(hw_ep->epnum)));
			musb_writew(musb->mregs, MUSB_INTRRXE, intr_e);
		} else {
			musb_writew(epio, MUSB_TXMAXP, qh->maxpacket);
			musb_writew(epio, MUSB_TXCSR, MUSB_TXCSR_DMAENAB);
			/*CC: speed?*/
			musb_writeb(epio, MUSB_TXTYPE,
						(qh->type_reg | usb_pipeendpoint(urb->pipe)));
			musb_writeb(epio, MUSB_TXINTERVAL,
						qh->intv_reg);
			if (musb->is_multipoint) {
				pr_debug("is_multipoint\n");
				musb_write_txfunaddr(mbase, hw_ep->epnum,
									 qh->addr_reg);
				musb_write_txhubaddr(mbase, hw_ep->epnum,
									 qh->h_addr_reg);
				musb_write_txhubport(mbase, hw_ep->epnum,
									 qh->h_port_reg);
				/* FIXME if !epnum, do the same for RX ... */
			} else {
				pr_debug("!! is_multipoint\n");
				musb_writeb(mbase, MUSB_FADDR, qh->addr_reg);
			}
			/*
			 * Turn off intrTx ,
			 * but this will be revert
			 * by musb_ep_program
			 */
			intr_e = musb_readw(musb->mregs, MUSB_INTRTXE);
			intr_e = intr_e & (~(1<<hw_ep->epnum));
			musb_writew(musb->mregs, MUSB_INTRTXE, intr_e);
		}

		mtk_qmu_enable(musb, hw_ep->epnum, isRx);
	}

	gpd_free_count = qmu_free_gpd_count(isRx, hw_ep->epnum);
	if (qh->type == USB_ENDPOINT_XFER_ISOC) {
		u32 gpd_used_count;

		pr_debug("USB_ENDPOINT_XFER_ISOC\n");
		pBuffer = urb->transfer_dma;

		if (gpd_free_count < urb->number_of_packets) {
			pr_debug("gpd_free_count:%d, number_of_packets:%d\n",
				gpd_free_count, urb->number_of_packets);
			pr_debug("%s:%d Error Here\n", __func__, __LINE__);
			return -ENOSPC;
		}
		for (i = 0; i < urb->number_of_packets; i++) {
			urb->iso_frame_desc[i].status = 0;
			offset = urb->iso_frame_desc[i].offset;
			dwLength = urb->iso_frame_desc[i].length;
			/* If interrupt on complete ? */
			bIsIoc = (i == (urb->number_of_packets-1)) ? 1 : 0;
			pr_debug("mtk_qmu_insert_task\n");
			mtk_qmu_insert_task(hw_ep->epnum, isRx,
								((dma_addr_t *)pBuffer + offset), dwLength, 0, bIsIoc);

			mtk_qmu_resume(hw_ep->epnum, isRx);
		}

		gpd_used_count = qmu_used_gpd_count(isRx, hw_ep->epnum);

		if (!isRx) {
			if (host_qmu_tx_max_active_isoc_gpd[hw_ep->epnum]
				< gpd_used_count)

				host_qmu_tx_max_active_isoc_gpd[hw_ep->epnum]
				= gpd_used_count;

			if (host_qmu_tx_max_number_of_pkts[hw_ep->epnum]
				< urb->number_of_packets)

				host_qmu_tx_max_number_of_pkts[hw_ep->epnum]
				= urb->number_of_packets;

			pr_debug("TXQ[%d], max_isoc gpd:%d, max_pkts:%d, active_dev:%d\n",
			 hw_ep->epnum,
			 host_qmu_tx_max_active_isoc_gpd
			 [hw_ep->epnum],
			 host_qmu_tx_max_number_of_pkts
			 [hw_ep->epnum],
			 mtk_host_active_dev_cnt);

		} else {
			if (host_qmu_rx_max_active_isoc_gpd[hw_ep->epnum]
				< gpd_used_count)

				host_qmu_rx_max_active_isoc_gpd[hw_ep->epnum]
				= gpd_used_count;

			if (host_qmu_rx_max_number_of_pkts[hw_ep->epnum]
				< urb->number_of_packets)

				host_qmu_rx_max_number_of_pkts[hw_ep->epnum]
				= urb->number_of_packets;

			pr_debug("RXQ[%d], max_isoc gpd:%d, max_pkts:%d, active_dev:%d\n",
			 hw_ep->epnum,
			 host_qmu_rx_max_active_isoc_gpd
			 [hw_ep->epnum],
			 host_qmu_rx_max_number_of_pkts
			 [hw_ep->epnum],
			 mtk_host_active_dev_cnt);
		}
	} else {
		/* Must be the bulk transfer type */
		QMU_WARN("non isoc\n");
		pBuffer = urb->transfer_dma;
		if (urb->transfer_buffer_length < QMU_RX_SPLIT_THRE) {
			if (gpd_free_count < 1) {
				pr_debug("gpd_free_count:%d, number_of_packets:%d\n",
		gpd_free_count, urb->number_of_packets);
				pr_debug("%s:%d Error Here\n",
					__func__, __LINE__);
				return -ENOSPC;
			}
			pr_debug("urb->transfer_buffer_length : %d\n",
				urb->transfer_buffer_length);

			dwLength = urb->transfer_buffer_length;
			bIsIoc = 1;

			mtk_qmu_insert_task(hw_ep->epnum, isRx,
								((dma_addr_t *)pBuffer + offset), dwLength, 0, bIsIoc);
			mtk_qmu_resume(hw_ep->epnum, isRx);
		} else {
			/*reuse isoc urb->unmber_of_packets*/
			urb->number_of_packets =
			((urb->transfer_buffer_length)
			+ QMU_RX_SPLIT_BLOCK_SIZE-1)
			/(QMU_RX_SPLIT_BLOCK_SIZE);
			if (gpd_free_count < urb->number_of_packets) {
				pr_debug("gpd_free_count:%d, number_of_packets:%d\n",
		gpd_free_count, urb->number_of_packets);
				pr_debug("%s:%d Error Here\n"
				, __func__, __LINE__);
				return -ENOSPC;
			}
			for (i = 0; i < urb->number_of_packets; i++) {
				offset = QMU_RX_SPLIT_BLOCK_SIZE*i;
				dwLength = QMU_RX_SPLIT_BLOCK_SIZE;

				/* If interrupt on complete ? */
				bIsIoc = (i == (urb->number_of_packets-1)) ?
				1 : 0;
				dwLength = (i == (urb->number_of_packets-1)) ?
				((urb->transfer_buffer_length)
				% QMU_RX_SPLIT_BLOCK_SIZE) : dwLength;
				if (dwLength == 0)
					dwLength = QMU_RX_SPLIT_BLOCK_SIZE;

				mtk_qmu_insert_task(hw_ep->epnum, isRx,
									((dma_addr_t *)pBuffer + offset), dwLength, 0, bIsIoc);
				mtk_qmu_resume(hw_ep->epnum, isRx);
			}
		}
	}
	/*sync register*/
	mb();

	pr_debug("\n");
	return 0;
}
