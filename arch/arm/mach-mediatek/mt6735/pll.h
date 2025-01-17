// SPDX-License-Identifier: GPL-2.0-only

#ifndef PLL_H
#define PLL_H

#define APMIXED_BASE      (0x10209000)
#define CKSYS_BASE        (0x10210000)
#define INFRACFG_AO_BASE  (0x10000000)
#define PERICFG_BASE      (0x10002000)
#define AUDIO_BASE        (0x11220000)
#define MFGCFG_BASE       (0x13000000)
#define MMSYS_CONFIG_BASE (0x14000000)
#define IMGSYS_BASE       (0x15000000)
#define VDEC_GCON_BASE    (0x16000000)
#define VENC_GCON_BASE    (0x17000000)

/* MCUSS Register */
#define ACLKEN_DIV              (0x10200640)

/* APMIXEDSYS Register */
#define AP_PLL_CON0             (APMIXED_BASE + 0x00)
#define AP_PLL_CON1             (APMIXED_BASE + 0x04)
#define AP_PLL_CON2             (APMIXED_BASE + 0x08)
#define AP_PLL_CON3             (APMIXED_BASE + 0x0C)
#define AP_PLL_CON4             (APMIXED_BASE + 0x10)
#define AP_PLL_CON5             (APMIXED_BASE + 0x14)
#define AP_PLL_CON6             (APMIXED_BASE + 0x18)
#define AP_PLL_CON7             (APMIXED_BASE + 0x1C)
#define CLKSQ_STB_CON0          (APMIXED_BASE + 0x20)
#define PLL_PWR_CON0            (APMIXED_BASE + 0x24)
#define PLL_PWR_CON1            (APMIXED_BASE + 0x28)
#define PLL_ISO_CON0            (APMIXED_BASE + 0x2C)
#define PLL_ISO_CON1            (APMIXED_BASE + 0x30)
#define PLL_STB_CON0            (APMIXED_BASE + 0x34)
#define DIV_STB_CON0            (APMIXED_BASE + 0x38)
#define PLL_CHG_CON0            (APMIXED_BASE + 0x3C)
#define PLL_TEST_CON0           (APMIXED_BASE + 0x40)

#define ARMPLL_CON0             (APMIXED_BASE + 0x200)
#define ARMPLL_CON1             (APMIXED_BASE + 0x204)
#define ARMPLL_CON2             (APMIXED_BASE + 0x208)
#define ARMPLL_PWR_CON0         (APMIXED_BASE + 0x20C)

#define MAINPLL_CON0            (APMIXED_BASE + 0x210)
#define MAINPLL_CON1            (APMIXED_BASE + 0x214)
#define MAINPLL_PWR_CON0        (APMIXED_BASE + 0x21C)

#define UNIVPLL_CON0            (APMIXED_BASE + 0x220)
#define UNIVPLL_CON1            (APMIXED_BASE + 0x224)
#define UNIVPLL_PWR_CON0        (APMIXED_BASE + 0x22C)

#define MMPLL_CON0              (APMIXED_BASE + 0x230)
#define MMPLL_CON1              (APMIXED_BASE + 0x234)
#define MMPLL_CON2              (APMIXED_BASE + 0x238)
#define MMPLL_PWR_CON0          (APMIXED_BASE + 0x23C)

#define MSDCPLL_CON0            (APMIXED_BASE + 0x240)
#define MSDCPLL_CON1            (APMIXED_BASE + 0x244)
#define MSDCPLL_PWR_CON0        (APMIXED_BASE + 0x24C)

#define VENCPLL_CON0            (APMIXED_BASE + 0x250)
#define VENCPLL_CON1            (APMIXED_BASE + 0x254)
#define VENCPLL_PWR_CON0        (APMIXED_BASE + 0x25C)

#define TVDPLL_CON0             (APMIXED_BASE + 0x260)
#define TVDPLL_CON1             (APMIXED_BASE + 0x264)
#define TVDPLL_PWR_CON0         (APMIXED_BASE + 0x26C)
#define APLL1_CON0              (APMIXED_BASE + 0x270)
#define APLL1_CON1              (APMIXED_BASE + 0x274)
#define APLL1_CON2              (APMIXED_BASE + 0x278)
#define APLL1_CON3              (APMIXED_BASE + 0x27C)
#define APLL1_PWR_CON0          (APMIXED_BASE + 0x280)

#define APLL2_CON0              (APMIXED_BASE + 0x284)
#define APLL2_CON1              (APMIXED_BASE + 0x288)
#define APLL2_CON2              (APMIXED_BASE + 0x28C)
#define APLL2_CON3              (APMIXED_BASE + 0x290)
#define APLL2_PWR_CON0          (APMIXED_BASE + 0x294)
#define CLK_MODE                (CKSYS_BASE + 0x000)
#define TST_SEL_0               (CKSYS_BASE + 0x020)
#define TST_SEL_1               (CKSYS_BASE + 0x024)
#define CLK_CFG_0               (CKSYS_BASE + 0x040)
#define CLK_CFG_1               (CKSYS_BASE + 0x050)
#define CLK_CFG_2               (CKSYS_BASE + 0x060)
#define CLK_CFG_3               (CKSYS_BASE + 0x070)
#define CLK_CFG_4               (CKSYS_BASE + 0x080)
#define CLK_CFG_5               (CKSYS_BASE + 0x090)
#define CLK_CFG_6               (CKSYS_BASE + 0x0A0) 
#define CLK_CFG_7               (CKSYS_BASE + 0x0B0) 
#define CLK_CFG_8               (CKSYS_BASE + 0x100)
#define CLK_CFG_9               (CKSYS_BASE + 0x104)
#define CLK_CFG_10              (CKSYS_BASE + 0x108)
#define CLK_CFG_11              (CKSYS_BASE + 0x10C)
#define CLK_SCP_CFG_0           (CKSYS_BASE + 0x200)
#define CLK_SCP_CFG_1           (CKSYS_BASE + 0x204)
#define CLK_MISC_CFG_0          (CKSYS_BASE + 0x210)
#define CLK_MISC_CFG_1          (CKSYS_BASE + 0x214)
#define CLK_MISC_CFG_2          (CKSYS_BASE + 0x218)
#define CLK26CALI_0             (CKSYS_BASE + 0x220)
#define CLK26CALI_1             (CKSYS_BASE + 0x224)
#define CLK26CALI_2             (CKSYS_BASE + 0x228)
#define CKSTA_REG               (CKSYS_BASE + 0x22C)
#define MBIST_CFG_0             (CKSYS_BASE + 0x308)
#define MBIST_CFG_1             (CKSYS_BASE + 0x30C)

#define TOP_CKMUXSEL            (INFRACFG_AO_BASE + 0x00)
#define TOP_CKDIV1              (INFRACFG_AO_BASE + 0x08)
#define TOP_DCMCTL              (INFRACFG_AO_BASE + 0x10)

#define	INFRA_GLOBALCON_DCMCTL  (INFRACFG_AO_BASE + 0x050)
#define INFRA_GLOBALCON_DCMCTL_MASK     (0x00000303)
#define INFRA_GLOBALCON_DCMCTL_ON       (0x00000303)
#define INFRA_GLOBALCON_DCMDBC  (INFRACFG_AO_BASE + 0x054)
#define INFRA_GLOBALCON_DCMDBC_MASK  ((0x7f<<0) | (1<<8) | (0x7f<<16) | (1<<24))
#define INFRA_GLOBALCON_DCMDBC_ON      ((0<<0) | (1<<8) | (0<<16) | (1<<24))
#define INFRA_GLOBALCON_DCMFSEL (INFRACFG_AO_BASE + 0x058)
#define INFRA_GLOBALCON_DCMFSEL_MASK ((0x7<<0) | (0xf<<8) | (0x1f<<16) | (0x1f<<24))
#define INFRA_GLOBALCON_DCMFSEL_ON ((0<<0) | (0<<8) | (0x10<<16) | (0x10<<24))

#define INFRA_PDN_SET0          (INFRACFG_AO_BASE + 0x0040)
#define INFRA_PDN_CLR0          (INFRACFG_AO_BASE + 0x0044)
#define INFRA_PDN_STA0          (INFRACFG_AO_BASE + 0x0048)

#define PERI_PDN_SET0           (PERICFG_BASE + 0x0008)
#define PERI_PDN_CLR0           (PERICFG_BASE + 0x0010)
#define PERI_PDN_STA0           (PERICFG_BASE + 0x0018)

void mt_pll_init(void);

#endif
