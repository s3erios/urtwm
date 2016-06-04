/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2016 Andriy Voskoboinyk <avos@FreeBSD.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * $OpenBSD: if_urtwnreg.h,v 1.3 2010/11/16 18:02:59 damien Exp $
 * $FreeBSD$
 */

#define URTWM_CONFIG_INDEX	0
#define URTWM_IFACE_INDEX	0

#define	URTWM_NOISE_FLOOR	-95

#define R92C_MAX_CHAINS	2

/* Maximum number of output pipes is 4. */
#define R88A_MAX_EPOUT	4

#define R92C_MAX_TX_PWR	0x3f

#define R88A_PUBQ_NPAGES	219
#define R88A_TXPKTBUF_COUNT	256
#define R88A_TX_PAGE_COUNT	244
#define R88A_TX_PAGE_BOUNDARY	(R88A_TX_PAGE_COUNT + 1)
#define R88A_RX_DMA_BUFFER_SIZE	0x3e80

#define R92C_H2C_NBOX		4

/* USB Requests. */
#define R92C_REQ_REGS		0x05
#define R92C_REG_DOWN_SIG	0xeaeaeaea

/*
 * MAC registers.
 */
/* System Configuration. */
#define R92C_SYS_ISO_CTRL		0x000
#define R92C_SYS_FUNC_EN		0x002
#define R92C_APS_FSMCO			0x004
#define R92C_SYS_CLKR			0x008
#define R92C_AFE_MISC			0x010
#define R92C_SPS0_CTRL			0x011
#define R92C_SPS_OCP_CFG		0x018
#define R92C_RSV_CTRL			0x01c
#define R92C_RF_CTRL			0x01f
#define R92C_LDOA15_CTRL		0x020
#define R92C_LDOV12D_CTRL		0x021
#define R92C_LDOHCI12_CTRL		0x022
#define R92C_LPLDO_CTRL			0x023
#define R92C_AFE_XTAL_CTRL		0x024
#define R92C_AFE_PLL_CTRL		0x028
#define R92C_APE_PLL_CTRL_EXT		0x02c
#define R92C_EFUSE_CTRL			0x030
#define R92C_EFUSE_TEST			0x034
#define R92C_PWR_DATA			0x038
#define R92C_CAL_TIMER			0x03c
#define R92C_ACLK_MON			0x03e
#define R92C_GPIO_MUXCFG		0x040
#define R92C_GPIO_IO_SEL		0x042
#define R92C_MAC_PINMUX_CFG		0x043
#define R92C_GPIO_PIN_CTRL		0x044
#define R92C_GPIO_IN			0x044
#define R92C_GPIO_OUT			0x045
#define R92C_GPIO_IOSEL			0x046
#define R92C_GPIO_MOD			0x047
#define R92C_GPIO_INTM			0x048
#define R92C_LEDCFG0			0x04c
#define R92C_LEDCFG1			0x04d
#define R92C_LEDCFG2			0x04e
#define R92C_LEDCFG3			0x04f
#define R92C_FSIMR			0x050
#define R92C_FSISR			0x054
#define R92C_HSIMR			0x058
#define R92C_HSISR			0x05c
#define R88E_BB_PAD_CTRL		0x064
#define R88A_SDIO_CTRL			0x070
#define R88A_RF_B_CTRL			0x076
#define R92C_MCUFWDL			0x080
#define R92C_HMEBOX_EXT(idx)		(0x088 + (idx) * 2)
#define R88E_HIMR			0x0b0
#define R88E_HISR			0x0b4
#define R88E_HIMRE			0x0b8
#define R88E_HISRE			0x0bc
#define R92C_EFUSE_ACCESS               0x0cf
#define R92C_BIST_SCAN			0x0d0
#define R92C_BIST_RPT			0x0d4
#define R92C_BIST_ROM_RPT		0x0d8
#define R92C_USB_SIE_INTF		0x0e0
#define R92C_PCIE_MIO_INTF		0x0e4
#define R92C_PCIE_MIO_INTD		0x0e8
#define R92C_HPON_FSM			0x0ec
#define R92C_SYS_CFG			0x0f0
/* MAC General Configuration. */
#define R92C_CR				0x100
#define R92C_MSR			0x102
#define R92C_PBP			0x104
#define R92C_TRXDMA_CTRL		0x10c
#define R92C_TRXFF_BNDY			0x114
#define R92C_TRXFF_STATUS		0x118
#define R92C_RXFF_PTR			0x11c
#define R92C_HIMR			0x120
#define R92C_HISR			0x124
#define R92C_HIMRE			0x128
#define R92C_HISRE			0x12c
#define R92C_CPWM			0x12f
#define R92C_FWIMR			0x130
#define R92C_FWISR			0x134
#define R92C_PKTBUF_DBG_CTRL		0x140
#define R92C_PKTBUF_DBG_DATA_L		0x144
#define R92C_PKTBUF_DBG_DATA_H		0x148
#define R92C_TC0_CTRL(i)		(0x150 + (i) * 4)
#define R92C_TCUNIT_BASE		0x164
#define R92C_MBIST_START		0x174
#define R92C_MBIST_DONE			0x178
#define R92C_MBIST_FAIL			0x17c
#define R88E_32K_CTRL			0x194
#define R92C_C2HEVT_MSG_NORMAL		0x1a0
#define R92C_C2HEVT_MSG_TEST		0x1b8
#define R92C_C2HEVT_CLEAR		0x1bf
#define R92C_MCUTST_1			0x1c0
#define R92C_FMETHR			0x1c8
#define R92C_HMETFR			0x1cc
#define R92C_HMEBOX(idx)		(0x1d0 + (idx) * 4)
#define R92C_LLT_INIT			0x1e0
#define R92C_BB_ACCESS_CTRL		0x1e8
#define R92C_BB_ACCESS_DATA		0x1ec
#define R88E_HMEBOX_EXT(idx)            (0x1f0 + (idx) * 4)
/* Tx DMA Configuration. */
#define R92C_RQPN			0x200
#define R92C_FIFOPAGE			0x204
#define R92C_TDECTRL			0x208
#define R88A_DWBCN0_CTRL		R92C_TDECTRL
#define R92C_TXDMA_OFFSET_CHK		0x20c
#define R92C_TXDMA_STATUS		0x210
#define R92C_RQPN_NPQ			0x214
#define R88A_DWBCN1_CTRL		0x228
/* Rx DMA Configuration. */
#define R92C_RXDMA_AGG_PG_TH		0x280
#define R92C_RXPKT_NUM			0x284
#define R92C_RXDMA_STATUS		0x288
#define R88A_RXDMA_PRO			0x290
#define R88A_EARLY_MODE_CONTROL		0x2bc
/* Protocol Configuration. */
#define R88A_TXPKT_EMPTY		0x41a
#define R92C_FWHW_TXQ_CTRL		0x420
#define R92C_HWSEQ_CTRL			0x423
#define R92C_TXPKTBUF_BCNQ_BDNY		0x424
#define R92C_TXPKTBUF_MGQ_BDNY		0x425
#define R92C_SPEC_SIFS			0x428
#define R92C_RL				0x42a
#define R92C_DARFRC			0x430
#define R92C_RARFRC			0x438
#define R92C_RRSR			0x440
#define R92C_ARFR(i)			(0x444 + (i) * 4)
#define R88A_ARFR_5G(i)			(0x444 + (i) * 8)
#define R88A_CCK_CHECK			0x454
#define R88A_AMPDU_MAX_TIME		0x456
#define R88E_TXPKTBUF_BCNQ1_BDNY	0x457
#define R92C_AGGLEN_LMT			0x458
#define R88A_AMPDU_MAX_LENGTH		R92C_AGGLEN_LMT
#define R92C_AMPDU_MIN_SPACE		0x45c
#define R92C_TXPKTBUF_WMAC_LBK_BF_HD	0x45d
#define R92C_FAST_EDCA_CTRL		0x460
#define R92C_RD_RESP_PKT_TH		0x463
#define R92C_INIRTS_RATE_SEL		0x480
#define R88A_DATA_SEC			0x483
#define R92C_INIDATA_RATE_SEL(macid)	(0x484 + (macid))
#define R88A_ARFR_2G(i)			(0x48c + (i) * 8)
#define R92C_QUEUE_CTRL			0x4c6
#define R88A_HT_SINGLE_AMPDU		0x4c7
#define R92C_MAX_AGGR_NUM		0x4ca
#define R92C_BAR_MODE_CTRL		0x4cc
#define R88E_TX_RPT_CTRL		0x4ec
#define R88E_TX_RPT_MACID_MAX		0x4ed
#define R88E_TX_RPT_TIME		0x4f0
/* EDCA Configuration. */
#define R92C_EDCA_VO_PARAM		0x500
#define R92C_EDCA_VI_PARAM		0x504
#define R92C_EDCA_BE_PARAM		0x508
#define R92C_EDCA_BK_PARAM		0x50c
#define R92C_BCNTCFG			0x510
#define R92C_PIFS			0x512
#define R92C_RDG_PIFS			0x513
#define R92C_SIFS_CCK			0x514
#define R92C_SIFS_OFDM			0x516
#define R92C_AGGR_BREAK_TIME		0x51a
#define R92C_SLOT			0x51b
#define R92C_TX_PTCL_CTRL		0x520
#define R92C_TXPAUSE			0x522
#define R92C_DIS_TXREQ_CLR		0x523
#define R92C_RD_CTRL			0x524
#define R92C_TBTT_PROHIBIT		0x540
#define R92C_RD_NAV_NXT			0x544
#define R92C_NAV_PROT_LEN		0x546
#define R92C_BCN_CTRL			0x550
#define R92C_BCN_CTRL1			0x551
#define R92C_MBID_NUM			0x552
#define R92C_DUAL_TSF_RST		0x553
#define R92C_BCN_INTERVAL		0x554
#define R92C_BCN_INTERVAL1		0x556
#define R92C_DRVERLYINT			0x558
#define R92C_BCNDMATIM			0x559
#define R92C_ATIMWND			0x55a
#define R92C_USTIME_TSF			0x55c
#define R92C_BCN_MAX_ERR		0x55d
#define R92C_RXTSF_OFFSET_CCK		0x55e
#define R92C_RXTSF_OFFSET_OFDM		0x55f
#define R92C_TSFTR(i)			(0x560 + (i) * 8)
#define R92C_PSTIMER			0x580
#define R92C_TIMER0			0x584
#define R92C_TIMER1			0x588
#define R92C_ACMHWCTRL			0x5c0
#define R92C_ACMRSTCTRL			0x5c1
#define R92C_ACMAVG			0x5c2
#define R92C_VO_ADMTIME			0x5c4
#define R92C_VI_ADMTIME			0x5c6
#define R92C_BE_ADMTIME			0x5c8
#define R92C_EDCA_RANDOM_GEN		0x5cc
#define R92C_SCH_TXCMD			0x5d0
#define R88E_SCH_TXCMD			0x5f8
/* WMAC Configuration. */
#define R92C_APSD_CTRL			0x600
#define R92C_BWOPMODE			0x603
#define R92C_RCR			0x608
#define R92C_RX_PKT_LIMIT		0x60c
#define R92C_RX_DRVINFO_SZ		0x60f
#define R92C_MACID			0x610
#define R92C_BSSID			0x618
#define R92C_MAR			0x620
#define R92C_USTIME_EDCA		0x638
#define R92C_MAC_SPEC_SIFS		0x63a
#define R92C_R2T_SIFS			0x63c
#define R92C_T2T_SIFS			0x63e
#define R92C_ACKTO			0x640
#define R92C_NAV_UPPER			0x652
#define R88A_WMAC_TRXPTCL_CTL		0x668
#define R92C_CAMCMD			0x670
#define R92C_CAMWRITE			0x674
#define R92C_CAMREAD			0x678
#define R92C_CAMDBG			0x67c
#define R92C_SECCFG			0x680
#define R92C_RXFLTMAP0			0x6a0
#define R92C_RXFLTMAP1			0x6a2
#define R92C_RXFLTMAP2			0x6a4
#define R92C_BCN_PSR_RPT		0x6a8

/* Bits for R92C_SYS_ISO_CTRL. */
#define R92C_SYS_ISO_CTRL_MD2PP		0x0001
#define R92C_SYS_ISO_CTRL_UA2USB	0x0002
#define R92C_SYS_ISO_CTRL_UD2CORE	0x0004
#define R92C_SYS_ISO_CTRL_PA2PCIE	0x0008
#define R92C_SYS_ISO_CTRL_PD2CORE	0x0010
#define R92C_SYS_ISO_CTRL_IP2MAC	0x0020
#define R92C_SYS_ISO_CTRL_DIOP		0x0040
#define R92C_SYS_ISO_CTRL_DIOE		0x0080
#define R92C_SYS_ISO_CTRL_EB2CORE	0x0100
#define R92C_SYS_ISO_CTRL_DIOR		0x0200
#define R92C_SYS_ISO_CTRL_PWC_EV25V	0x4000
#define R92C_SYS_ISO_CTRL_PWC_EV12V	0x8000

/* Bits for R92C_SYS_FUNC_EN. */
#define R92C_SYS_FUNC_EN_BBRSTB		0x0001
#define R92C_SYS_FUNC_EN_BB_GLB_RST	0x0002
#define R92C_SYS_FUNC_EN_USBA		0x0004
#define R92C_SYS_FUNC_EN_UPLL		0x0008
#define R92C_SYS_FUNC_EN_USBD		0x0010
#define R92C_SYS_FUNC_EN_DIO_PCIE	0x0020
#define R92C_SYS_FUNC_EN_PCIEA		0x0040
#define R92C_SYS_FUNC_EN_PPLL		0x0080
#define R92C_SYS_FUNC_EN_PCIED		0x0100
#define R92C_SYS_FUNC_EN_DIOE		0x0200
#define R92C_SYS_FUNC_EN_CPUEN		0x0400
#define R92C_SYS_FUNC_EN_DCORE		0x0800
#define R92C_SYS_FUNC_EN_ELDR		0x1000
#define R92C_SYS_FUNC_EN_DIO_RF		0x2000
#define R92C_SYS_FUNC_EN_HWPDN		0x4000
#define R92C_SYS_FUNC_EN_MREGEN		0x8000

/* Bits for R92C_APS_FSMCO. */
#define R92C_APS_FSMCO_PFM_LDALL	0x00000001
#define R92C_APS_FSMCO_PFM_ALDN		0x00000002
#define R92C_APS_FSMCO_PFM_LDKP		0x00000004
#define R92C_APS_FSMCO_PFM_WOWL		0x00000008
#define R92C_APS_FSMCO_PDN_EN		0x00000010
#define R92C_APS_FSMCO_PDN_PL		0x00000020
#define R92C_APS_FSMCO_APFM_ONMAC	0x00000100
#define R92C_APS_FSMCO_APFM_OFF		0x00000200
#define R92C_APS_FSMCO_APFM_RSM		0x00000400
#define R92C_APS_FSMCO_AFSM_HSUS	0x00000800
#define R92C_APS_FSMCO_AFSM_PCIE	0x00001000
#define R92C_APS_FSMCO_APDM_MAC		0x00002000
#define R92C_APS_FSMCO_APDM_HOST	0x00004000
#define R92C_APS_FSMCO_APDM_HPDN	0x00008000
#define R92C_APS_FSMCO_RDY_MACON	0x00010000
#define R92C_APS_FSMCO_SUS_HOST		0x00020000
#define R92C_APS_FSMCO_ROP_ALD		0x00100000
#define R92C_APS_FSMCO_ROP_PWR		0x00200000
#define R92C_APS_FSMCO_ROP_SPS		0x00400000
#define R92C_APS_FSMCO_SOP_MRST		0x02000000
#define R92C_APS_FSMCO_SOP_FUSE		0x04000000
#define R92C_APS_FSMCO_SOP_ABG		0x08000000
#define R92C_APS_FSMCO_SOP_AMB		0x10000000
#define R92C_APS_FSMCO_SOP_RCK		0x20000000
#define R92C_APS_FSMCO_SOP_A8M		0x40000000
#define R92C_APS_FSMCO_XOP_BTCK		0x80000000

/* Bits for R92C_SYS_CLKR. */
#define R92C_SYS_CLKR_ANAD16V_EN	0x00000001
#define R92C_SYS_CLKR_ANA8M		0x00000002
#define R92C_SYS_CLKR_MACSLP		0x00000010
#define R92C_SYS_CLKR_LOADER_EN		0x00000020
#define R92C_SYS_CLKR_80M_SSC_DIS	0x00000080
#define R92C_SYS_CLKR_80M_SSC_EN_HO	0x00000100
#define R92C_SYS_CLKR_PHY_SSC_RSTB	0x00000200
#define R92C_SYS_CLKR_SEC_EN		0x00000400
#define R92C_SYS_CLKR_MAC_EN		0x00000800
#define R92C_SYS_CLKR_SYS_EN		0x00001000
#define R92C_SYS_CLKR_RING_EN		0x00002000

/* Bits for R92C_RF_CTRL. */
#define R92C_RF_CTRL_EN		0x01
#define R92C_RF_CTRL_RSTB	0x02
#define R92C_RF_CTRL_SDMRSTB	0x04

/* Bits for R92C_LDOA15_CTRL. */
#define R92C_LDOA15_CTRL_EN		0x01
#define R92C_LDOA15_CTRL_STBY		0x02
#define R92C_LDOA15_CTRL_OBUF		0x04
#define R92C_LDOA15_CTRL_REG_VOS	0x08

/* Bits for R92C_LDOV12D_CTRL. */
#define R92C_LDOV12D_CTRL_LDV12_EN	0x01

/* Bits for R92C_LPLDO_CTRL. */
#define R92C_LPLDO_CTRL_SLEEP		0x10

/* Bits for R92C_AFE_XTAL_CTRL. */
#define R92C_AFE_XTAL_CTRL_ADDR_M	0x007ff800
#define R92C_AFE_XTAL_CTRL_ADDR_S	11

/* Bits for R92C_AFE_PLL_CTRL. */
#define R92C_AFE_PLL_CTRL_EN		0x0001
#define R92C_AFE_PLL_CTRL_320_EN	0x0002
#define R92C_AFE_PLL_CTRL_FREF_SEL	0x0004
#define R92C_AFE_PLL_CTRL_EDGE_SEL	0x0008
#define R92C_AFE_PLL_CTRL_WDOGB		0x0010
#define R92C_AFE_PLL_CTRL_LPFEN		0x0020

/* Bits for R92C_EFUSE_CTRL. */
#define R92C_EFUSE_CTRL_DATA_M	0x000000ff
#define R92C_EFUSE_CTRL_DATA_S	0
#define R92C_EFUSE_CTRL_ADDR_M	0x0003ff00
#define R92C_EFUSE_CTRL_ADDR_S	8
#define R92C_EFUSE_CTRL_VALID	0x80000000

/* Bits for R92C_GPIO_MUXCFG. */
#define R92C_GPIO_MUXCFG_ENBT	0x0020

/* Bits for R92C_LEDCFG0. */
#define R92C_LEDCFG0_DIS	0x08

/* Bits for R92C_LEDCFG2. */
#define R88A_LEDCFG2_ENA	0x20

/* Bits for R92C_MCUFWDL. */
#define R92C_MCUFWDL_EN			0x00000001
#define R92C_MCUFWDL_RDY		0x00000002
#define R92C_MCUFWDL_CHKSUM_RPT		0x00000004
#define R92C_MCUFWDL_MACINI_RDY		0x00000008
#define R92C_MCUFWDL_BBINI_RDY		0x00000010
#define R92C_MCUFWDL_RFINI_RDY		0x00000020
#define R92C_MCUFWDL_WINTINI_RDY	0x00000040
#define R92C_MCUFWDL_RAM_DL_SEL		0x00000080
#define R92C_MCUFWDL_PAGE_M		0x00070000
#define R92C_MCUFWDL_PAGE_S		16
#define R92C_MCUFWDL_CPRST		0x00800000

/* Bits for R88E_HIMR. */
#define R88E_HIMR_CPWM			0x00000100
#define R88E_HIMR_CPWM2			0x00000200
#define R88E_HIMR_TBDER			0x04000000
#define R88E_HIMR_PSTIMEOUT		0x20000000

/* Bits for R88E_HIMRE.*/
#define R88E_HIMRE_RXFOVW		0x00000100
#define R88E_HIMRE_TXFOVW		0x00000200
#define R88E_HIMRE_RXERR		0x00000400
#define R88E_HIMRE_TXERR		0x00000800

/* Bits for R92C_EFUSE_ACCESS. */
#define R92C_EFUSE_ACCESS_OFF		0x00
#define R92C_EFUSE_ACCESS_ON		0x69

/* Bits for R92C_HPON_FSM. */
#define R92C_HPON_FSM_CHIP_BONDING_ID_S		22
#define R92C_HPON_FSM_CHIP_BONDING_ID_M		0x00c00000
#define R92C_HPON_FSM_CHIP_BONDING_ID_92C_1T2R	1

/* Bits for R92C_SYS_CFG. */
#define R92C_SYS_CFG_XCLK_VLD		0x00000001
#define R92C_SYS_CFG_ACLK_VLD		0x00000002
#define R92C_SYS_CFG_UCLK_VLD		0x00000004
#define R92C_SYS_CFG_PCLK_VLD		0x00000008
#define R92C_SYS_CFG_PCIRSTB		0x00000010
#define R92C_SYS_CFG_V15_VLD		0x00000020
#define R92C_SYS_CFG_TRP_B15V_EN	0x00000080
#define R92C_SYS_CFG_SIC_IDLE		0x00000100
#define R92C_SYS_CFG_BD_MAC2		0x00000200
#define R92C_SYS_CFG_BD_MAC1		0x00000400
#define R92C_SYS_CFG_IC_MACPHY_MODE	0x00000800
#define R92C_SYS_CFG_CHIP_VER_RTL_M	0x0000f000
#define R92C_SYS_CFG_CHIP_VER_RTL_S	12
#define R92C_SYS_CFG_BT_FUNC		0x00010000
#define R92C_SYS_CFG_VENDOR_UMC		0x00080000
#define R92C_SYS_CFG_PAD_HWPD_IDN	0x00400000
#define R92C_SYS_CFG_TRP_VAUX_EN	0x00800000
#define R92C_SYS_CFG_TRP_BT_EN		0x01000000
#define R92C_SYS_CFG_BD_PKG_SEL		0x02000000
#define R92C_SYS_CFG_BD_HCI_SEL		0x04000000
#define R92C_SYS_CFG_TYPE_92C		0x08000000

/* Bits for R92C_CR. */
#define R92C_CR_HCI_TXDMA_EN	0x0001
#define R92C_CR_HCI_RXDMA_EN	0x0002
#define R92C_CR_TXDMA_EN	0x0004
#define R92C_CR_RXDMA_EN	0x0008
#define R92C_CR_PROTOCOL_EN	0x0010
#define R92C_CR_SCHEDULE_EN	0x0020
#define R92C_CR_MACTXEN		0x0040
#define R92C_CR_MACRXEN		0x0080
#define R92C_CR_ENSWBCN		0x0100
#define R92C_CR_ENSEC		0x0200
#define R92C_CR_CALTMR_EN	0x0400

/* Bits for R92C_MSR. */
#define R92C_MSR_NOLINK		0x00
#define R92C_MSR_ADHOC		0x01
#define R92C_MSR_INFRA		0x02
#define R92C_MSR_AP		0x03
#define R92C_MSR_MASK		(R92C_MSR_AP)

/* Bits for R92C_PBP. */
#define R92C_PBP_PSRX_M		0x0f
#define R92C_PBP_PSRX_S		0
#define R92C_PBP_PSTX_M		0xf0
#define R92C_PBP_PSTX_S		4
#define R92C_PBP_64		0
#define R92C_PBP_128		1
#define R92C_PBP_256		2
#define R92C_PBP_512		3
#define R92C_PBP_1024		4

/* Bits for R92C_TRXDMA_CTRL. */
#define R92C_TRXDMA_CTRL_RXDMA_AGG_EN		0x0004
#define R92C_TRXDMA_CTRL_TXDMA_VOQ_MAP_M	0x0030
#define R92C_TRXDMA_CTRL_TXDMA_VOQ_MAP_S	4
#define R92C_TRXDMA_CTRL_TXDMA_VIQ_MAP_M	0x00c0
#define R92C_TRXDMA_CTRL_TXDMA_VIQ_MAP_S	6
#define R92C_TRXDMA_CTRL_TXDMA_BEQ_MAP_M	0x0300
#define R92C_TRXDMA_CTRL_TXDMA_BEQ_MAP_S	8
#define R92C_TRXDMA_CTRL_TXDMA_BKQ_MAP_M	0x0c00
#define R92C_TRXDMA_CTRL_TXDMA_BKQ_MAP_S	10
#define R92C_TRXDMA_CTRL_TXDMA_MGQ_MAP_M	0x3000
#define R92C_TRXDMA_CTRL_TXDMA_MGQ_MAP_S	12
#define R92C_TRXDMA_CTRL_TXDMA_HIQ_MAP_M	0xc000
#define R92C_TRXDMA_CTRL_TXDMA_HIQ_MAP_S	14
#define R92C_TRXDMA_CTRL_QUEUE_LOW		1
#define R92C_TRXDMA_CTRL_QUEUE_NORMAL		2
#define R92C_TRXDMA_CTRL_QUEUE_HIGH		3
#define R92C_TRXDMA_CTRL_QMAP_M			0xfff0
/* Shortcuts. */
#define R92C_TRXDMA_CTRL_QMAP_3EP		0xf5b0
#define R92C_TRXDMA_CTRL_QMAP_HQ_LQ		0xf5f0
#define R92C_TRXDMA_CTRL_QMAP_HQ_NQ		0xfaf0
#define R92C_TRXDMA_CTRL_QMAP_LQ		0x5550
#define R92C_TRXDMA_CTRL_QMAP_NQ		0xaaa0
#define R92C_TRXDMA_CTRL_QMAP_HQ		0xfff0

/* Bits for R92C_LLT_INIT. */
#define R92C_LLT_INIT_DATA_M		0x000000ff
#define R92C_LLT_INIT_DATA_S		0
#define R92C_LLT_INIT_ADDR_M		0x0000ff00
#define R92C_LLT_INIT_ADDR_S		8
#define R92C_LLT_INIT_OP_M		0xc0000000
#define R92C_LLT_INIT_OP_S		30
#define R92C_LLT_INIT_OP_NO_ACTIVE	0
#define R92C_LLT_INIT_OP_WRITE		1

/* Bits for R92C_RQPN. */
#define R92C_RQPN_HPQ_M		0x000000ff
#define R92C_RQPN_HPQ_S		0
#define R92C_RQPN_LPQ_M		0x0000ff00
#define R92C_RQPN_LPQ_S		8
#define R92C_RQPN_PUBQ_M	0x00ff0000
#define R92C_RQPN_PUBQ_S	16
#define R92C_RQPN_LD		0x80000000

/* Bits for R88A_DWBCN1_CTRL. */
#define R88A_DWBCN1_CTRL_SEL_EN		0x02

/* Bits for R92C_TDECTRL. */
#define R92C_TDECTRL_BLK_DESC_NUM_M	0x000000f0
#define R92C_TDECTRL_BLK_DESC_NUM_S	4
#define R92C_TDECTRL_BCN_VALID		0x00010000

/* Bits for R92C_TXDMA_OFFSET_CHK. */
#define R92C_TXDMA_OFFSET_DROP_DATA_EN	0x00000200

/* Bits for R92C_FWHW_TXQ_CTRL. */
#define R92C_FWHW_TXQ_CTRL_AMPDU_RTY_NEW	0x80
#define R92C_FWHW_TXQ_CTRL_REAL_BEACON	0x400000

/* Bits for R92C_SPEC_SIFS. */
#define R92C_SPEC_SIFS_CCK_M	0x00ff
#define R92C_SPEC_SIFS_CCK_S	0
#define R92C_SPEC_SIFS_OFDM_M	0xff00
#define R92C_SPEC_SIFS_OFDM_S	8

/* Bits for R92C_RL. */
#define R92C_RL_LRL_M		0x003f
#define R92C_RL_LRL_S		0
#define R92C_RL_SRL_M		0x3f00
#define R92C_RL_SRL_S		8

/* Bits for R92C_RRSR. */
#define R92C_RRSR_RATE_BITMAP_M		0x000fffff
#define R92C_RRSR_RATE_BITMAP_S		0
#define R92C_RRSR_RATE_CCK_ONLY_1M	0xffff1
#define R92C_RRSR_RSC_LOWSUBCHNL	0x00200000
#define R92C_RRSR_RSC_UPSUBCHNL		0x00400000
#define R92C_RRSR_SHORT			0x00800000

/* Bits for R88A_CCK_CHECK. */
#define R88A_CCK_CHECK_5GHZ		0x80

/* Bits for R88A_DATA_SEC. */
#define R88A_DATA_SEC_NO_EXT		0x00
#define R88A_DATA_SEC_PRIM_UP_20	0x01
#define R88A_DATA_SEC_PRIM_DOWN_20	0x02
#define R88A_DATA_SEC_PRIM_UPPER_20	0x03
#define R88A_DATA_SEC_PRIM_LOWER_20	0x04
#define R88A_DATA_SEC_PRIM_UP_40	0x90
#define R88A_DATA_SEC_PRIM_DOWN_40	0xa0

/* Bits for R88A_HT_SINGLE_AMPDU. */
#define R88A_HT_SINGLE_AMPDU_PKT_ENA	0x80

/* Bits for R88E_TX_RPT_CTRL. */
#define R88E_TX_RPT1_ENA		0x01
#define R88E_TX_RPT2_ENA		0x02

/* Bits for R92C_EDCA_XX_PARAM. */
#define R92C_EDCA_PARAM_AIFS_M		0x000000ff
#define R92C_EDCA_PARAM_AIFS_S		0
#define R92C_EDCA_PARAM_ECWMIN_M	0x00000f00
#define R92C_EDCA_PARAM_ECWMIN_S	8
#define R92C_EDCA_PARAM_ECWMAX_M	0x0000f000
#define R92C_EDCA_PARAM_ECWMAX_S	12
#define R92C_EDCA_PARAM_TXOP_M		0xffff0000
#define R92C_EDCA_PARAM_TXOP_S		16

/* Bits for R92C_HWSEQ_CTRL / R92C_TXPAUSE. */
#define R92C_TX_QUEUE_VO		0x01
#define R92C_TX_QUEUE_VI		0x02
#define R92C_TX_QUEUE_BE		0x04
#define R92C_TX_QUEUE_BK		0x08
#define R92C_TX_QUEUE_MGT		0x10
#define R92C_TX_QUEUE_HIGH		0x20
#define R92C_TX_QUEUE_BCN		0x40

/* Shortcuts. */
#define R92C_TX_QUEUE_AC			\
	(R92C_TX_QUEUE_VO | R92C_TX_QUEUE_VI |	\
	 R92C_TX_QUEUE_BE | R92C_TX_QUEUE_BK)

#define R92C_TX_QUEUE_ALL			\
	(R92C_TX_QUEUE_AC | R92C_TX_QUEUE_MGT |	\
	 R92C_TX_QUEUE_HIGH | R92C_TX_QUEUE_BCN | 0x80)	/* XXX */

/* Bits for R92C_BCN_CTRL. */
#define R92C_BCN_CTRL_EN_MBSSID		0x02
#define R92C_BCN_CTRL_TXBCN_RPT		0x04
#define R92C_BCN_CTRL_EN_BCN		0x08
#define R92C_BCN_CTRL_DIS_TSF_UDT0	0x10

/* Bits for R92C_MBID_NUM. */
#define R92C_MBID_TXBCN_RPT0		0x08
#define R92C_MBID_TXBCN_RPT1		0x10

/* Bits for R92C_DUAL_TSF_RST. */
#define R92C_DUAL_TSF_RST0		0x01
#define R92C_DUAL_TSF_RST1		0x02
#define R92C_DUAL_TSF_RST_TXOK		0x20

/* Bits for R92C_ACMHWCTRL. */
#define R92C_ACMHWCTRL_EN		0x01
#define R92C_ACMHWCTRL_BE		0x02
#define R92C_ACMHWCTRL_VI		0x04
#define R92C_ACMHWCTRL_VO		0x08
#define R92C_ACMHWCTRL_ACM_MASK		0x0f

/* Bits for R92C_APSD_CTRL. */
#define R92C_APSD_CTRL_OFF		0x40
#define R92C_APSD_CTRL_OFF_STATUS	0x80

/* Bits for R92C_BWOPMODE. */
#define R92C_BWOPMODE_11J	0x01
#define R92C_BWOPMODE_5G	0x02
#define R92C_BWOPMODE_20MHZ	0x04

/* Bits for R92C_RCR. */
#define R92C_RCR_AAP		0x00000001
#define R92C_RCR_APM		0x00000002
#define R92C_RCR_AM		0x00000004
#define R92C_RCR_AB		0x00000008
#define R92C_RCR_ADD3		0x00000010
#define R92C_RCR_APWRMGT	0x00000020
#define R92C_RCR_CBSSID_DATA	0x00000040
#define R92C_RCR_CBSSID_BCN	0x00000080
#define R92C_RCR_ACRC32		0x00000100
#define R92C_RCR_AICV		0x00000200
#define R92C_RCR_ADF		0x00000800
#define R92C_RCR_ACF		0x00001000
#define R92C_RCR_AMF		0x00002000
#define R92C_RCR_HTC_LOC_CTRL	0x00004000
#define R92C_RCR_MFBEN		0x00400000
#define R92C_RCR_LSIGEN		0x00800000
#define R92C_RCR_ENMBID		0x01000000
#define R92C_RCR_APP_BA_SSN	0x08000000
#define R92C_RCR_APP_PHYSTS	0x10000000
#define R92C_RCR_APP_ICV	0x20000000
#define R92C_RCR_APP_MIC	0x40000000
#define R92C_RCR_APPFCS		0x80000000

/* Bits for R92C_CAMCMD. */
#define R92C_CAMCMD_ADDR_M	0x0000ffff
#define R92C_CAMCMD_ADDR_S	0
#define R92C_CAMCMD_WRITE	0x00010000
#define R92C_CAMCMD_CLR		0x40000000
#define R92C_CAMCMD_POLLING	0x80000000

/* Bits for R92C_SECCFG. */
#define R92C_SECCFG_TXUCKEY_DEF	0x0001
#define R92C_SECCFG_RXUCKEY_DEF	0x0002
#define R92C_SECCFG_TXENC_ENA	0x0004
#define R92C_SECCFG_RXDEC_ENA	0x0008
#define R92C_SECCFG_CMP_A2	0x0010
#define R92C_SECCFG_TXBCKEY_DEF	0x0040
#define R92C_SECCFG_RXBCKEY_DEF	0x0080
#define R88E_SECCFG_CHK_KEYID	0x0100

/* Bits for R92C_RXFLTMAP*. */
#define R92C_RXFLTMAP_SUBTYPE(subtype)	\
	(1 << ((subtype) >> IEEE80211_FC0_SUBTYPE_SHIFT))


/*
 * Baseband registers.
 */
#define R88A_OFDMCCK_EN			0x808
#define R88A_CCA_ON_SEC			0x838
#define R88A_L1_PEAK_TH			0x848
#define R88A_FC_AREA			0x860
#define R92C_FPGA0_RFIFACEOE(chain)	(0x860 + (chain) * 4)
#define R92C_FPGA0_RFPARAM(idx)		(0x878 + (idx) * 4)
#define R88A_RFMOD			0x8ac
#define R88A_HSSI_PARAM2		0x8b0
#define R88A_ADC_BUF_CLK		0x8c4
#define R92C_CCK0_SYSTEM		0xa00
#define R88A_HSSI_PARAM1(chain)		(0xc00 + (chain) * 0x200)
#define R88A_TX_SCALE(chain)		(0xc1c + (chain) * 0x200)
#define R88A_TXAGC_CCK11_1(chain)	(0xc20 + (chain) * 0x200)
#define R88A_TXAGC_OFDM18_6(chain)	(0xc24 + (chain) * 0x200)
#define R88A_TXAGC_OFDM54_24(chain)	(0xc28 + (chain) * 0x200)
#define R88A_TXAGC_MCS3_0(chain)	(0xc2c + (chain) * 0x200)
#define R88A_TXAGC_MCS7_4(chain)	(0xc30 + (chain) * 0x200)
#define R88A_TXAGC_MCS11_8(chain)	(0xc34 + (chain) * 0x200)
#define R88A_TXAGC_MCS15_12(chain)	(0xc38 + (chain) * 0x200)
#define R88A_TXAGC_NSS1IX3_1IX0(chain)	(0xc3c + (chain) * 0x200)
#define R88A_TXAGC_NSS1IX7_1IX4(chain)	(0xc40 + (chain) * 0x200)
#define R88A_TXAGC_NSS2IX1_1IX8(chain)	(0xc44 + (chain) * 0x200)
#define R88A_TXAGC_NSS2IX5_2IX2(chain)	(0xc48 + (chain) * 0x200)
#define R88A_TXAGC_NSS2IX9_2IX6(chain)	(0xc4c + (chain) * 0x200)
#define R92C_OFDM0_AGCCORE1(chain)	(0xc50 + (chain) * 8)
#define R88A_LSSI_PARAM(chain)		(0xc90 + (chain) * 0x200)
#define R88A_RFE_PINMUX(chain)		(0xcb0 + (chain) * 0x200)
#define R88A_RFE_INV(chain)		(0xcb4 + (chain) * 0x200)
#define R88A_HSPI_READBACK(chain)	(0xd04 + (chain) * 0x40)
#define R88A_LSSI_READBACK(chain)	(0xd08 + (chain) * 0x40)

/* Bits for R88A_OFDMCCK_EN. */
#define R88A_OFDMCCK_EN_CCK	0x10000000
#define R88A_OFDMCCK_EN_OFDM	0x20000000

/* Bits for R88A_CCA_ON_SEC. */
#define R88A_CCA_ON_SEC_EXT_CHAN_M	0xf0000000
#define R88A_CCA_ON_SEC_EXT_CHAN_S	28

/* Bits for R88A_RFE_PINMUX(i). */
#define R88A_RFE_PINMUX_PA_A_MASK	0x000000f0
#define R88A_RFE_PINMUX_LNA_MASK	0x0000f000

/* Bits for R92C_FPGA0_RFIFACEOE(0). */
#define R88A_FPGA0_RFIFACEOE0_ANT_M	0x00000300
#define R88A_FPGA0_RFIFACEOE0_ANT_S	8

/* Bits for R88A_RFMOD. */
#define R88A_RFMOD_EXT_CHAN_M		0x3C
#define R88A_RFMOD_EXT_CHAN_S		2

/* Bits for R88A_HSSI_PARAM2. */
#define R88A_HSSI_PARAM2_READ_ADDR_MASK	0xff

/* Bits for R88A_HSSI_PARAM1(i). */
#define R88A_HSSI_PARAM1_PI		0x00000004

/* Bits for R88A_TX_SCALE(i). */
#define R88A_TX_SCALE_SWING_M		0xffe00000
#define R88A_TX_SCALE_SWING_S		21

/* Bits for R88A_TXAGC_CCK11_1(i). */
#define R88A_TXAGC_CCK1_M		0x000000ff
#define R88A_TXAGC_CCK1_S		0
#define R88A_TXAGC_CCK2_M		0x0000ff00
#define R88A_TXAGC_CCK2_S		8
#define R88A_TXAGC_CCK55_M		0x00ff0000
#define R88A_TXAGC_CCK55_S		16
#define R88A_TXAGC_CCK11_M		0xff000000
#define R88A_TXAGC_CCK11_S		24

/* Bits for R88A_TXAGC_OFDM18_6(i). */
#define R88A_TXAGC_OFDM06_M		0x000000ff
#define R88A_TXAGC_OFDM06_S		0
#define R88A_TXAGC_OFDM09_M		0x0000ff00
#define R88A_TXAGC_OFDM09_S		8
#define R88A_TXAGC_OFDM12_M		0x00ff0000
#define R88A_TXAGC_OFDM12_S		16
#define R88A_TXAGC_OFDM18_M		0xff000000
#define R88A_TXAGC_OFDM18_S		24

/* Bits for R88A_TXAGC_OFDM54_24(i). */
#define R88A_TXAGC_OFDM24_M		0x000000ff
#define R88A_TXAGC_OFDM24_S		0
#define R88A_TXAGC_OFDM36_M		0x0000ff00
#define R88A_TXAGC_OFDM36_S		8
#define R88A_TXAGC_OFDM48_M		0x00ff0000
#define R88A_TXAGC_OFDM48_S		16
#define R88A_TXAGC_OFDM54_M		0xff000000
#define R88A_TXAGC_OFDM54_S		24

/* Bits for R88A_TXAGC_MCS3_0(i). */
#define R88A_TXAGC_MCS0_M		0x000000ff
#define R88A_TXAGC_MCS0_S		0
#define R88A_TXAGC_MCS1_M		0x0000ff00
#define R88A_TXAGC_MCS1_S		8
#define R88A_TXAGC_MCS2_M		0x00ff0000
#define R88A_TXAGC_MCS2_S		16
#define R88A_TXAGC_MCS3_M		0xff000000
#define R88A_TXAGC_MCS3_S		24

/* Bits for R88A_TXAGC_MCS7_4(i). */
#define R88A_TXAGC_MCS4_M		0x000000ff
#define R88A_TXAGC_MCS4_S		0
#define R88A_TXAGC_MCS5_M		0x0000ff00
#define R88A_TXAGC_MCS5_S		8
#define R88A_TXAGC_MCS6_M		0x00ff0000
#define R88A_TXAGC_MCS6_S		16
#define R88A_TXAGC_MCS7_M		0xff000000
#define R88A_TXAGC_MCS7_S		24

/* Bits for R88A_TXAGC_MCS11_8(i). */
#define R88A_TXAGC_MCS8_M		0x000000ff
#define R88A_TXAGC_MCS8_S		0
#define R88A_TXAGC_MCS9_M		0x0000ff00
#define R88A_TXAGC_MCS9_S		8
#define R88A_TXAGC_MCS10_M		0x00ff0000
#define R88A_TXAGC_MCS10_S		16
#define R88A_TXAGC_MCS11_M		0xff000000
#define R88A_TXAGC_MCS11_S		24

/* Bits for R88A_TXAGC_MCS15_12(i). */
#define R88A_TXAGC_MCS12_M		0x000000ff
#define R88A_TXAGC_MCS12_S		0
#define R88A_TXAGC_MCS13_M		0x0000ff00
#define R88A_TXAGC_MCS13_S		8
#define R88A_TXAGC_MCS14_M		0x00ff0000
#define R88A_TXAGC_MCS14_S		16
#define R88A_TXAGC_MCS15_M		0xff000000
#define R88A_TXAGC_MCS15_S		24

/* Bits for R92C_OFDM0_AGCCORE1(i). */
#define R92C_OFDM0_AGCCORE1_GAIN_M	0x0000007f
#define R92C_OFDM0_AGCCORE1_GAIN_S	0

/* Bits for R88A_LSSI_PARAM(i). */
#define R92C_LSSI_PARAM_DATA_M		0x000fffff
#define R92C_LSSI_PARAM_DATA_S		0
#define R88E_LSSI_PARAM_ADDR_M		0x0ff00000
#define R88E_LSSI_PARAM_ADDR_S		20

/* Bits for R88A_LSSI_READBACK(i). */
#define R92C_LSSI_READBACK_DATA_M	0x000fffff
#define R92C_LSSI_READBACK_DATA_S	0


/*
 * USB registers.
 */
#define R92C_USB_SUSPEND		0xfe10
#define R92C_USB_INFO			0xfe17
#define R92C_USB_SPECIAL_OPTION		0xfe55
#define R92C_USB_HCPWM			0xfe57
#define R92C_USB_HRPWM			0xfe58
#define R92C_USB_DMA_AGG_TO		0xfe5b
#define R92C_USB_AGG_TO			0xfe5c
#define R92C_USB_AGG_TH			0xfe5d
#define R92C_USB_VID			0xfe60
#define R92C_USB_PID			0xfe62
#define R92C_USB_OPTIONAL		0xfe64
#define R92C_USB_EP			0xfe65
#define R92C_USB_PHY			0xfe68
#define R92C_USB_MAC_ADDR		0xfe70
#define R92C_USB_STRING			0xfe80

/* Bits for R92C_USB_SPECIAL_OPTION. */
#define R92C_USB_SPECIAL_OPTION_AGG_EN		0x08
#define R92C_USB_SPECIAL_OPTION_INT_BULK_SEL	0x10

/* Bits for R92C_USB_EP. */
#define R92C_USB_EP_HQ_M	0x000f
#define R92C_USB_EP_HQ_S	0
#define R92C_USB_EP_NQ_M	0x00f0
#define R92C_USB_EP_NQ_S	4
#define R92C_USB_EP_LQ_M	0x0f00
#define R92C_USB_EP_LQ_S	8


/*
 * Firmware base address.
 */
#define R92C_FW_START_ADDR	0x1000
#define R92C_FW_PAGE_SIZE	4096


/*
 * RF (6052) registers.
 */
#define R92C_RF_AC		0x00
#define R92C_RF_IQADJ_G(i)	(0x01 + (i))
#define R92C_RF_POW_TRSW	0x05
#define R92C_RF_GAIN_RX		0x06
#define R92C_RF_GAIN_TX		0x07
#define R92C_RF_TXM_IDAC	0x08
#define R92C_RF_BS_IQGEN	0x0f
#define R92C_RF_MODE1		0x10
#define R92C_RF_MODE2		0x11
#define R92C_RF_RX_AGC_HP	0x12
#define R92C_RF_TX_AGC		0x13
#define R92C_RF_BIAS		0x14
#define R92C_RF_IPA		0x15
#define R92C_RF_POW_ABILITY	0x17
#define R92C_RF_CHNLBW		0x18
#define R92C_RF_RX_G1		0x1a
#define R92C_RF_RX_G2		0x1b
#define R92C_RF_RX_BB2		0x1c
#define R92C_RF_RX_BB1		0x1d
#define R92C_RF_RCK1		0x1e
#define R92C_RF_RCK2		0x1f
#define R92C_RF_TX_G(i)		(0x20 + (i))
#define R92C_RF_TX_BB1		0x23
#define R92C_RF_T_METER		0x24
#define R92C_RF_SYN_G(i)	(0x25 + (i))
#define R92C_RF_RCK_OS		0x30
#define R92C_RF_TXPA_G(i)	(0x31 + (i))
#define R88E_RF_T_METER		0x42

/* Bits for R92C_RF_AC. */
#define R92C_RF_AC_MODE_M	0x70000
#define R92C_RF_AC_MODE_S	16
#define R92C_RF_AC_MODE_STANDBY	1

/* Bits for R92C_RF_CHNLBW. */
#define R92C_RF_CHNLBW_CHNL_M	0x003ff
#define R92C_RF_CHNLBW_CHNL_S	0
#define R92C_RF_CHNLBW_BW20	0x00400
#define R88E_RF_CHNLBW_BW20	0x00c00
#define R92C_RF_CHNLBW_LCSTART	0x08000

/* Bits for R92C_RF_T_METER. */
#define R92C_RF_T_METER_START	0x60
#define R92C_RF_T_METER_VAL_M	0x1f
#define R92C_RF_T_METER_VAL_S	0

/* Bits for R88E_RF_T_METER. */
#define R88E_RF_T_METER_VAL_M	0x0fc00
#define R88E_RF_T_METER_VAL_S	10
#define R88E_RF_T_METER_START	0x30000


/*
 * CAM entries.
 */
#define R92C_CAM_ENTRY_COUNT	32

#define R92C_CAM_CTL0(entry)	((entry) * 8 + 0)
#define R92C_CAM_CTL1(entry)	((entry) * 8 + 1)
#define R92C_CAM_KEY(entry, i)	((entry) * 8 + 2 + (i))

/* Bits for R92C_CAM_CTL0(i). */
#define R92C_CAM_KEYID_M	0x00000003
#define R92C_CAM_KEYID_S	0
#define R92C_CAM_ALGO_M		0x0000001c
#define R92C_CAM_ALGO_S		2
#define R92C_CAM_ALGO_NONE	0
#define R92C_CAM_ALGO_WEP40	1
#define R92C_CAM_ALGO_TKIP	2
#define R92C_CAM_ALGO_AES	4
#define R92C_CAM_ALGO_WEP104	5
#define R92C_CAM_VALID		0x00008000
#define R92C_CAM_MACLO_M	0xffff0000
#define R92C_CAM_MACLO_S	16

/* Rate adaptation modes. */
#define R88A_RAID_11BGN_2_40	0
#define R88A_RAID_11BGN_1_40	1
#define R88A_RAID_11BGN_2	2
#define R88A_RAID_11BGN_1	3
#define R88A_RAID_11GN_2	4
#define R88A_RAID_11GN_1	5
#define R88A_RAID_11BG		6
#define R88A_RAID_11G		7	/* "pure" 11g */
#define R88A_RAID_11B		8
#define R88A_RAID_11AC_2_80	9
#define R88A_RAID_11AC_1_80	10
#define R88A_RAID_11AC_1	11
#define R88A_RAID_11AC_2	12


/*
 * Macros to access subfields in registers.
 */
/* Mask and Shift (getter). */
#define MS(val, field)							\
	(((val) & field##_M) >> field##_S)

/* Shift and Mask (setter). */
#define SM(field, val)							\
	(((val) << field##_S) & field##_M)

/* Rewrite. */
#define RW(var, field, val)						\
	(((var) & ~field##_M) | SM(field, val))

/*
 * Firmware image header.
 */
struct r92c_fw_hdr {
	/* QWORD0 */
	uint16_t	signature;
	uint8_t		category;
	uint8_t		function;
	uint16_t	version;
	uint16_t	subversion;
	/* QWORD1 */
	uint8_t		month;
	uint8_t		date;
	uint8_t		hour;
	uint8_t		minute;
	uint16_t	ramcodesize;
	uint16_t	reserved2;
	/* QWORD2 */
	uint32_t	svnidx;
	uint32_t	reserved3;
	/* QWORD3 */
	uint32_t	reserved4;
	uint32_t	reserved5;
} __packed;

/*
 * Host to firmware commands.
 */
struct r92c_fw_cmd {
	uint8_t	id;
#define R92C_CMD_AP_OFFLOAD		0
#define R92C_CMD_SET_PWRMODE		1
#define R92C_CMD_JOINBSS_RPT		2
#define R92C_CMD_RSVD_PAGE		3
#define R92C_CMD_RSSI			4
#define R92C_CMD_RSSI_SETTING		5
#define R92C_CMD_MACID_CONFIG		6
#define R92C_CMD_MACID_PS_MODE		7
#define R92C_CMD_P2P_PS_OFFLOAD		8
#define R92C_CMD_SELECTIVE_SUSPEND	9
#define R92C_CMD_FLAG_EXT		0x80

	uint8_t	msg[5];
} __packed;

struct r88e_fw_cmd {
	uint8_t id;
#define	R88E_CMD_MACID_CONFIG		0x40
	uint8_t msg[7];
} __packed;

/* Structure for R92C_CMD_RSVD_PAGE. */
struct r92c_fw_cmd_rsvd_page {
	uint8_t probe_resp;
	uint8_t ps_poll;
	uint8_t null_data;
} __packed;

/* Structure for R92C_CMD_RSSI_SETTING. */
struct r92c_fw_cmd_rssi {
	uint8_t	macid;
	uint8_t	reserved;
	uint8_t	pwdb;
} __packed;

/* Structure for R92C_CMD_MACID_CONFIG. */
struct r92c_fw_cmd_macid_cfg {
	uint32_t	mask;
	uint8_t		macid;
#define URTWM_MACID_BC		0	/* Broadcast. */
#define URTWM_MACID_BSS		12
#define R88A_MACID_MAX		63
#define URTWM_MACID_MAX(sc)	R88A_MACID_MAX
#define URTWM_MACID_UNDEFINED	(uint8_t)-1
#define URTWM_MACID_VALID	0x80
} __packed;


#define URTWM_MAX_TX_COUNT	4
#define URTWM_MAX_RF_PATH	4

#define URTWM_MAX_GROUP_2G	6
#define URTWM_MAX_GROUP_5G	14

#define URTWM_DEF_TX_PWR_2G	0x2d
#define URTWM_DEF_TX_PWR_5G	0xfe

struct r88a_tx_pwr_2g {
	uint8_t		cck[URTWM_MAX_GROUP_2G];
	uint8_t		ht40[URTWM_MAX_GROUP_2G - 1];
} __packed;

struct r88a_tx_pwr_diff_2g {
	uint8_t		ht20_ofdm;
#define LOW_PART_M	0x0f
#define LOW_PART_S	0
#define HIGH_PART_M	0xf0
#define HIGH_PART_S	4

	struct {
		uint8_t	ht40_ht20;
		uint8_t	ofdm_cck;
	} __packed	diff123[URTWM_MAX_TX_COUNT - 1];
} __packed;

struct r88a_tx_pwr_5g {
	uint8_t		ht40[URTWM_MAX_GROUP_5G];
} __packed;

struct r88a_tx_pwr_diff_5g {
	uint8_t		ht20_ofdm;
	uint8_t		ht40_ht20[URTWM_MAX_TX_COUNT - 1];
	uint8_t		ofdm_ofdm[2];
	uint8_t		ht80_ht160[URTWM_MAX_TX_COUNT];
} __packed;

struct r88a_tx_pwr {
	struct r88a_tx_pwr_2g		pwr_2g;
	struct r88a_tx_pwr_diff_2g	pwr_diff_2g;
	struct r88a_tx_pwr_5g		pwr_5g;
	struct r88a_tx_pwr_diff_5g	pwr_diff_5g;
} __packed;

/*
 * RTL8821AU ROM image.
 */
struct r88a_rom {
	uint8_t			reserved1[16];
	struct r88a_tx_pwr	tx_pwr[URTWM_MAX_RF_PATH];
	uint8_t			channel_plan;
	uint8_t			xtal;
	uint8_t			thermal_meter;
	uint8_t			iqk_lck;
	uint8_t			pa_type;
#define R88A_ROM_PA_TYPE_EXTERNAL_5GHZ	0x01

	uint8_t			reserved2[2];
	uint8_t			lna_type;
#define R88A_ROM_LNA_TYPE_EXTERNAL_2GHZ	0x08
#define R88A_ROM_LNA_TYPE_EXTERNAL_5GHZ	0x08

	uint8_t			reserved3;
	uint8_t			rf_board_opt;
#define R92C_ROM_RF1_REGULATORY_M	0x07
#define R92C_ROM_RF1_REGULATORY_S	0
#define R92C_ROM_RF1_BOARD_TYPE_M	0xe0
#define R92C_ROM_RF1_BOARD_TYPE_S	5

	uint8_t			rf_feature_opt;
	uint8_t			rf_bt_opt;
	uint8_t			version;
	uint8_t			customer_id;
	uint8_t			tx_bbswing_2g;
	uint8_t			tx_bbswing_5g;
	uint8_t			reserved4[1];
        uint8_t			rf_ant_opt;
	uint8_t			reserved5[54];
	uint16_t		vid;
	uint16_t		pid;
	uint8_t			reserved6[3];
	uint8_t			macaddr[IEEE80211_ADDR_LEN];
	uint8_t			reserved7[2];
	uint8_t			string[8];	/* "Realtek " */
	uint8_t			reserved8[2];
	uint8_t			string_ven[23];	/* XXX variable length? */
	uint8_t			reserved9[208];
} __packed;

#define	URTWM_EFUSE_MAX_LEN		512

/* Rx MAC descriptor. */
struct r92c_rx_stat {
	uint32_t	rxdw0;
#define R92C_RXDW0_PKTLEN_M	0x00003fff
#define R92C_RXDW0_PKTLEN_S	0
#define R92C_RXDW0_CRCERR	0x00004000
#define R92C_RXDW0_ICVERR	0x00008000
#define R92C_RXDW0_INFOSZ_M	0x000f0000
#define R92C_RXDW0_INFOSZ_S	16
#define R92C_RXDW0_CIPHER_M	0x00700000
#define R92C_RXDW0_CIPHER_S	20
#define R92C_RXDW0_QOS		0x00800000
#define R92C_RXDW0_SHIFT_M	0x03000000
#define R92C_RXDW0_SHIFT_S	24
#define R92C_RXDW0_PHYST	0x04000000
#define R92C_RXDW0_SWDEC	0x08000000

	uint32_t	rxdw1;
#define R92C_RXDW1_MACID_M	0x0000003f
#define R92C_RXDW1_MACID_S	0
#define R92C_RXDW1_MC		0x40000000
#define R92C_RXDW1_BC		0x80000000

	uint32_t	rxdw2;
#define R92C_RXDW2_PKTCNT_M	0x00ff0000
#define R92C_RXDW2_PKTCNT_S	16

	uint32_t	rxdw3;
#define R92C_RXDW3_RATE_M	0x0000003f
#define R92C_RXDW3_RATE_S	0
#define R92C_RXDW3_HT		0x00000040
#define R92C_RXDW3_HTC		0x00000400
#define R88E_RXDW3_RPT_M	0x0000c000
#define R88E_RXDW3_RPT_S	14
#define R88E_RXDW3_RPT_RX	0
#define R88E_RXDW3_RPT_TX1	1
#define R88E_RXDW3_RPT_TX2	2

	uint32_t	rxdw4;
	uint32_t	rxdw5;
} __packed __attribute__((aligned(4)));

/* Rx PHY descriptor. */
struct r92c_rx_phystat {
	uint32_t	phydw0;
	uint32_t	phydw1;
	uint32_t	phydw2;
	uint32_t	phydw3;
	uint32_t	phydw4;
	uint32_t	phydw5;
	uint32_t	phydw6;
	uint32_t	phydw7;
} __packed __attribute__((aligned(4)));

/* Rx PHY CCK descriptor. */
struct r92c_rx_cck {
	uint8_t		adc_pwdb[4];
	uint8_t		sq_rpt;
	uint8_t		agc_rpt;
} __packed;

struct r88e_rx_cck {
	uint8_t		path_agc[2];
	uint8_t		chan;
	uint8_t		reserved1;
	uint8_t		sig_qual;
	uint8_t		agc_rpt;
	uint8_t		rpt_b;
	uint8_t 	reserved2;
	uint8_t		noise_power;
	uint8_t		path_cfotail[2];        
	uint8_t		pcts_mask[2];   
	uint8_t		stream_rxevm[2];        
	uint8_t		path_rxsnr[2];
	uint8_t		noise_power_db_lsb;
	uint8_t		reserved3[3];
	uint8_t		stream_csi[2];
	uint8_t		stream_target_csi[2];
	uint8_t		sig_evm;
} __packed;

/* Tx MAC descriptor. */
struct r88a_tx_desc {
	uint16_t	pktlen;
	uint8_t		offset;
	uint8_t		flags0;
#define R88A_FLAGS0_BMCAST	0x01
#define R88A_FLAGS0_LSG		0x04
#define R88A_FLAGS0_FSG		0x08
#define R88A_FLAGS0_OWN		0x80

	uint32_t	txdw1;
#define R88A_TXDW1_MACID_M	0x0000003f
#define R88A_TXDW1_MACID_S	0
#define R88A_TXDW1_QSEL_M	0x00001f00
#define R88A_TXDW1_QSEL_S	8

#define R88A_TXDW1_QSEL_BE	0x00	/* or 0x03 */
#define R88A_TXDW1_QSEL_BK	0x01	/* or 0x02 */
#define R88A_TXDW1_QSEL_VI	0x04	/* or 0x05 */
#define R88A_TXDW1_QSEL_VO	0x06	/* or 0x07 */
#define URTWM_MAX_TID		8

#define R88A_TXDW1_QSEL_BEACON	0x10
#define R88A_TXDW1_QSEL_MGNT	0x12

#define R88A_TXDW1_RAID_M	0x001f0000
#define R88A_TXDW1_RAID_S	16
#define R88A_TXDW1_CIPHER_M	0x00c00000
#define R88A_TXDW1_CIPHER_S	22
#define R88A_TXDW1_CIPHER_NONE	0
#define R88A_TXDW1_CIPHER_RC4	1
#define R88A_TXDW1_CIPHER_SM4	2
#define R88A_TXDW1_CIPHER_AES	3
#define R88A_TXDW1_PKTOFF_M	0x1f000000
#define R88A_TXDW1_PKTOFF_S	24

	uint32_t	txdw2;
#define R88A_TXDW2_AGGEN	0x00001000
#define R88A_TXDW2_AGGBK	0x00010000
#define R88A_TXDW2_MOREFRAG	0x00020000
#define R88A_TXDW2_CCX_RPT	0x00080000

	uint32_t	txdw3;
#define R88A_TXDW3_DRVRATE	0x00000100
#define R88A_TXDW3_DISRTSFB	0x00000200
#define R88A_TXDW3_DISDATAFB	0x00000400
#define R88A_TXDW3_CTS2SELF	0x00000800
#define R88A_TXDW3_RTSEN	0x00001000
#define R88A_TXDW3_HWRTSEN	0x00002000

	uint32_t	txdw4;
#define R88A_TXDW4_DATARATE_M		0x0000007f
#define R88A_TXDW4_DATARATE_S		0
#define R88A_TXDW4_DATARATE_FB_LMT_M	0x00001f00
#define R88A_TXDW4_DATARATE_FB_LMT_S	8
#define R88A_TXDW4_RTSRATE_FB_LMT_M	0x0001e000
#define R88A_TXDW4_RTSRATE_FB_LMT_S	13
#define R88A_TXDW4_RETRY_LMT_ENA	0x00020000
#define R88A_TXDW4_RETRY_LMT_M		0x00fc0000
#define R88A_TXDW4_RETRY_LMT_S		18
#define R88A_TXDW4_RTSRATE_M		0x1f000000
#define R88A_TXDW4_RTSRATE_S		24

	uint32_t	txdw5;
#define R88A_TXDW5_SHPRE	0x00000010
#define R88A_TXDW5_DATA_LDPC	0x00000080

	uint32_t	txdw6;

	uint16_t	txdsum;
	uint16_t	flags7;
#define R88A_FLAGS7_AGGNUM_M	0xff00
#define R88A_FLAGS7_AGGNUM_S	8

	uint32_t	txdw8;
#define R88A_TXDW8_HWSEQ_EN	0x00008000

	uint32_t	txdw9;
#define R88A_TXDW9_SEQ_M	0x00fff000
#define R88A_TXDW9_SEQ_S	12
} __packed __attribute__((aligned(4)));

struct r88e_tx_rpt_ccx {
	uint8_t		rptb0;
	uint8_t		rptb1;
#define R88E_RPTB1_MACID_M	0x3f
#define R88E_RPTB1_MACID_S	0
#define R88E_RPTB1_PKT_OK	0x40
#define R88E_RPTB1_BMC		0x80

	uint8_t		rptb2;
#define R88E_RPTB2_RETRY_CNT_M	0x3f
#define R88E_RPTB2_RETRY_CNT_S	0
#define R88E_RPTB2_LIFE_EXPIRE	0x40
#define R88E_RPTB2_RETRY_OVER	0x80

	uint8_t		rptb3;
	uint8_t		rptb4;
	uint8_t		rptb5;
	uint8_t		rptb6;
#define R88E_RPTB6_QSEL_M	0xf0
#define R88E_RPTB6_QSEL_S	4

	uint8_t		rptb7;
} __packed;

#define	R88A_INTR_MSG_LEN	60
#define R88A_MRR_SIZE		8


static const uint8_t ridx2rate[] =
	{ 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 };

/* HW rate indices. */
#define URTWM_RIDX_CCK1		0
#define URTWM_RIDX_CCK2		1
#define URTWM_RIDX_CCK55	2
#define URTWM_RIDX_CCK11	3
#define URTWM_RIDX_OFDM6	4
#define URTWM_RIDX_OFDM9	5
#define URTWM_RIDX_OFDM12	6
#define URTWM_RIDX_OFDM18	7
#define URTWM_RIDX_OFDM24	8
#define URTWM_RIDX_OFDM36	9
#define URTWM_RIDX_OFDM48	10
#define URTWM_RIDX_OFDM54	11
#define URTWM_RIDX_MCS(i)	(12 + (i))

#define URTWM_RIDX_COUNT	28
#define URTWM_RIDX_UNKNOWN	(uint8_t)-1

#define URTWM_RATE_IS_CCK(rate)  ((rate) <= URTWM_RIDX_CCK11)
#define URTWM_RATE_IS_OFDM(rate) ((rate) >= URTWM_RIDX_OFDM6)


/*
 * MAC initialization values.
 */
static const struct {
	uint16_t	reg;
	uint8_t		val;
} rtl8821au_mac[] = {
	{ 0x428, 0x0a }, { 0x429, 0x10 }, { 0x430, 0x00 }, { 0x431, 0x00 },
	{ 0x432, 0x00 }, { 0x433, 0x01 }, { 0x434, 0x04 }, { 0x435, 0x05 },
	{ 0x436, 0x07 }, { 0x437, 0x08 }, { 0x43c, 0x04 }, { 0x43d, 0x05 },
	{ 0x43e, 0x07 }, { 0x43f, 0x08 }, { 0x440, 0x5d }, { 0x441, 0x01 },
	{ 0x442, 0x00 }, { 0x444, 0x10 }, { 0x445, 0x00 }, { 0x446, 0x00 },
	{ 0x447, 0x00 }, { 0x448, 0x00 }, { 0x449, 0xf0 }, { 0x44a, 0x0f },
	{ 0x44b, 0x3e }, { 0x44c, 0x10 }, { 0x44d, 0x00 }, { 0x44e, 0x00 },
	{ 0x44f, 0x00 }, { 0x450, 0x00 }, { 0x451, 0xf0 }, { 0x452, 0x0f },
	{ 0x453, 0x00 }, { 0x456, 0x5e }, { 0x460, 0x66 }, { 0x461, 0x66 },
	{ 0x4c8, 0x3f }, { 0x4c9, 0xff }, { 0x4cc, 0xff }, { 0x4cd, 0xff },
	{ 0x4ce, 0x01 }, { 0x500, 0x26 }, { 0x501, 0xa2 }, { 0x502, 0x2f },
	{ 0x503, 0x00 }, { 0x504, 0x28 }, { 0x505, 0xa3 }, { 0x506, 0x5e },
	{ 0x507, 0x00 }, { 0x508, 0x2b }, { 0x509, 0xa4 }, { 0x50a, 0x5e },
	{ 0x50b, 0x00 }, { 0x50c, 0x4f }, { 0x50d, 0xa4 }, { 0x50e, 0x00 },
	{ 0x50f, 0x00 }, { 0x512, 0x1c }, { 0x514, 0x0a }, { 0x516, 0x0a },
	{ 0x525, 0x4f }, { 0x550, 0x10 }, { 0x551, 0x10 }, { 0x559, 0x02 },
	{ 0x55c, 0x50 }, { 0x55d, 0xff }, { 0x605, 0x30 }, { 0x607, 0x07 },
	{ 0x608, 0x0e }, { 0x609, 0x2a }, { 0x620, 0xff }, { 0x621, 0xff },
	{ 0x622, 0xff }, { 0x623, 0xff }, { 0x624, 0xff }, { 0x625, 0xff },
	{ 0x626, 0xff }, { 0x627, 0xff }, { 0x638, 0x50 }, { 0x63c, 0x0a },
	{ 0x63d, 0x0a }, { 0x63e, 0x0e }, { 0x63f, 0x0e }, { 0x640, 0x40 },
	{ 0x642, 0x40 }, { 0x643, 0x00 }, { 0x652, 0xc8 }, { 0x66e, 0x05 },
	{ 0x700, 0x21 }, { 0x701, 0x43 }, { 0x702, 0x65 }, { 0x703, 0x87 },
	{ 0x708, 0x21 }, { 0x709, 0x43 }, { 0x70a, 0x65 }, { 0x70b, 0x87 },
	{ 0x718, 0x40 }
};

/*
 * Baseband initialization values.
 */
struct urtwm_bb_prog {
	int		count;
	const uint16_t	*regs;
	const uint32_t	*vals;
	int		agccount;
	const uint32_t	*agcvals;
};

/*
 * RTL8821AU
 */
static const uint16_t rtl8821au_bb_regs[] = {
	0x800, 0x804, 0x808, 0x80c, 0x810, 0x814, 0x818, 0x820, 0x824,
	0x828, 0x82c, 0x830, 0x834, 0x838, 0x83c, 0x840, 0x844, 0x848,
	0x84c, 0x850, 0x854, 0x858, 0x85c, 0x860, 0x864, 0x868, 0x86c,
	0x870, 0x874, 0x878, 0x87c, 0x8a0, 0x8a4, 0x8a8, 0x8ac, 0x8b4,
	0x8b8, 0x8bc, 0x8c0, 0x8c4, 0x8c8, 0x8cc, 0x8d4, 0x8d8, 0x8f8,
	0x8fc, 0x900, 0x90c, 0x910, 0x914, 0x918, 0x91c, 0x920, 0x924,
	0x928, 0x92c, 0x930, 0x934, 0x960, 0x964, 0x968, 0x96c, 0x970,
	0x974, 0x978, 0x97c, 0x980, 0x984, 0x988, 0x990, 0x994, 0x998,
	0x99c, 0x9a0, 0x9a4, 0x9a8, 0x9ac, 0x9b0, 0x9b4, 0x9b8, 0x9bc,
	0x9d0, 0x9d4, 0x9d8, 0x9dc, 0x9e0, 0x9e4, 0x9e8, 0xa00, 0xa04,
	0xa08, 0xa0c, 0xa10, 0xa14, 0xa18, 0xa1c, 0xa20, 0xa24, 0xa28,
	0xa2c, 0xa70, 0xa74, 0xa78, 0xa7c, 0xa80, 0xa84, 0xb00, 0xb04,
	0xb08, 0xb0c, 0xb10, 0xb14, 0xb18, 0xb1c, 0xb20, 0xb24, 0xb28,
	0xb2c, 0xb30, 0xb34, 0xb38, 0xb3c, 0xb40, 0xb44, 0xb48, 0xb4c,
	0xb50, 0xb54, 0xb58, 0xb5c, 0xc00, 0xc04, 0xc08, 0xc0c, 0xc10,
	0xc14, 0xc1c, 0xc20, 0xc24, 0xc28, 0xc2c, 0xc30, 0xc34, 0xc38,
	0xc3c, 0xc40, 0xc44, 0xc48, 0xc4c, 0xc50, 0xc54, 0xc58, 0xc5c,
	0xc60, 0xc64, 0xc68, 0xc6c, 0xc70, 0xc74, 0xc78, 0xc7c, 0xc80,
	0xc84, 0xc94, 0xc98, 0xc9c, 0xca0, 0xca4, 0xca8, 0xcb0, 0xcb4,
	0xcb8
};

static const uint32_t rtl8821au_bb_vals[] = {
	0x0020d090, 0x080112e0, 0x0e028211, 0x92131111, 0x20101261,
	0x020c3d10, 0x03a00385, 0x00000000, 0x00030fe0, 0x00000000,
	0x002081dd, 0x2aaa8e24,	0x0037a706, 0x06489b44, 0x0000095b,
	0xc0000001, 0x40003cde, 0x62103f8b, 0x6cfdffb8, 0x28874706,
	0x0001520c, 0x8060e000, 0x74210168, 0x6929c321, 0x79727432,
	0x8ca7a314, 0x888c2878, 0x08888888, 0x31612c2e, 0x00000152,
	0x000fd000, 0x00000013, 0x7f7f7f7f, 0xa2000338, 0x0ff0fa0a,
	0x000fc080, 0x6c10d7ff, 0x0ca52090, 0x1bf00020, 0x00000000,
	0x00013169, 0x08248492,	0x940008a0, 0x290b5612, 0x400002c0,
	0x00000000, 0x00000700, 0x00000000, 0x0000fc00, 0x00000404,
	0x1c1028c0, 0x64b11a1c, 0xe0767233, 0x055aa500,	0x00000004,
	0xfffe0000, 0xfffffffe, 0x001fffff, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x801fffff, 0x000003ff, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x27100000,
	0xffff0100, 0xffffff5c, 0xffffffff, 0x000000ff, 0x00480080,
	0x00000000, 0x00000000, 0x81081008, 0x01081008, 0x01081008,
	0x01081008, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00005d00, 0x00000002, 0x00000001, 0x00d047c8, 0x01ff000c,
	0x8c8a8300, 0x2e68000f, 0x9500bb78, 0x11144028, 0x00881117,
	0x89140f00, 0x1a1b0000, 0x090e1317, 0x00000204, 0x00900000,
	0x101fff00, 0x00000008, 0x00000900, 0x225b0606, 0x21805490,
	0x001f0000, 0x03100040, 0x0000b000, 0xae0201eb, 0x01003207,
	0x00009807, 0x01000000, 0x00000002, 0x00000002, 0x0000001f,
	0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c, 0x13121110,
	0x17161514, 0x0000003a, 0x00000000, 0x00000000, 0x13000032,
	0x48080000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000007, 0x00042020, 0x80410231, 0x00000000, 0x00000100,
	0x01000000, 0x40000003, 0x2c2c2c2c, 0x30303030, 0x30303030,
	0x2c2c2c2c, 0x2c2c2c2c, 0x2c2c2c2c, 0x2c2c2c2c, 0x2a2a2a2a,
	0x2a2a2a2a, 0x2a2a2a2a, 0x2a2a2a2a, 0x2a2a2a2a, 0x00000020,
	0x001c1208, 0x30000c1c, 0x00000058, 0x34344443, 0x07003333,
	0x19791979, 0x19791979, 0x19791979, 0x19791979, 0x19791979,
	0x19791979, 0x19791979, 0x19791979, 0x0100005c, 0x00000000,
	0x00000000, 0x00000029, 0x08040201, 0x80402010, 0x77775242,
	0x10100077, 0x00508240
};

#define RTL8821AU_AGC_VALS_START	\
	0xbf000001, 0xbf020001, 0xbf040001, 0xbf060001, 0xbe080001, \
	0xbd0a0001, 0xbc0c0001, 0xba0e0001, 0xb9100001, 0xb8120001, \
	0xb7140001, 0xb6160001, 0xb5180001, 0xb41a0001, 0xb31c0001, \
	0xb21e0001, 0xb1200001, 0xb0220001, 0xaf240001, 0xae260001, \
	0xad280001, 0xac2a0001, 0xab2c0001, 0xaa2e0001, 0xa9300001, \
	0xa8320001, 0xa7340001, 0xa6360001, 0xa5380001, 0xa43a0001, \
	0x683c0001, 0x673e0001, 0x66400001, 0x65420001, 0x64440001, \
	0x63460001, 0x62480001, 0x614a0001, 0x474c0001, 0x464e0001, \
	0x45500001, 0x44520001, 0x43540001, 0x42560001, 0x41580001, \
	0x285a0001, 0x275c0001, 0x265e0001, 0x25600001, 0x24620001, \
	0x0a640001, 0x09660001, 0x08680001, 0x076a0001, 0x066c0001, \
	0x056e0001, 0x04700001, 0x03720001, 0x02740001, 0x01760001, \
	0x01780001, 0x017a0001, 0x017c0001, 0x017e0001,

#define RTL8821AU_AGC_VALS_END		\
	0x01740101, 0x01760101, 0x01780101, 0x017a0101, 0x017c0101, \
	0x017e0101

static const uint32_t rtl8821au_agc_vals[] = {
	RTL8821AU_AGC_VALS_START
	0xff000101, 0xff020101, 0xfe040101, 0xfd060101, 0xfc080101,
	0xfd0a0101, 0xfc0c0101, 0xfb0e0101, 0xfa100101, 0xf9120101,
	0xf8140101, 0xf7160101, 0xf6180101, 0xf51a0101, 0xf41c0101,
	0xf31e0101, 0xf2200101, 0xf1220101, 0xf0240101, 0xef260101,
	0xee280101, 0xed2a0101, 0xec2c0101, 0xeb2e0101, 0xea300101,
	0xe9320101, 0xe8340101, 0xe7360101, 0xe6380101, 0xe53a0101,
	0xe43c0101, 0xe33e0101, 0xa5400101, 0xa4420101, 0xa3440101,
	0x87460101, 0x86480101, 0x854a0101, 0x844c0101, 0x694e0101,
	0x68500101, 0x67520101, 0x66540101, 0x49560101, 0x48580101,
	0x475a0101, 0x2a5c0101, 0x295e0101, 0x28600101, 0x27620101,
	0x26640101, 0x25660101, 0x24680101, 0x236a0101, 0x056c0101,
	0x046e0101, 0x03700101, 0x02720101,
	RTL8821AU_AGC_VALS_END
};

/*
 * The same, but with external 5GHz PA / LNA.
 */
static const uint32_t rtl8821au_ext_5ghz_agc_vals[] = {
	RTL8821AU_AGC_VALS_START
	0xfb000101, 0xfa020101, 0xf9040101, 0xf8060101, 0xf7080101,
	0xf60a0101, 0xf50c0101, 0xf40e0101, 0xf3100101, 0xf2120101,
	0xf1140101, 0xf0160101, 0xef180101, 0xee1a0101, 0xed1c0101,
	0xec1e0101, 0xeb200101, 0xea220101, 0xe9240101, 0xe8260101,
	0xe7280101, 0xe62a0101, 0xe52c0101, 0xe42e0101, 0xe3300101,
	0xa5320101, 0xa4340101, 0xa3360101, 0x87380101, 0x863a0101,
	0x853c0101, 0x843e0101, 0x69400101, 0x68420101, 0x67440101,
	0x66460101, 0x49480101, 0x484a0101, 0x474c0101, 0x2a4e0101,
	0x29500101, 0x28520101, 0x27540101, 0x26560101, 0x25580101,
	0x245a0101, 0x235c0101, 0x055e0101, 0x04600101, 0x03620101,
	0x02640101, 0x01660101, 0x01680101, 0x016a0101, 0x016c0101,
	0x016e0101, 0x01700101, 0x01720101,
	RTL8821AU_AGC_VALS_END
};

static const struct urtwm_bb_prog rtl8821au_bb_prog = {
	nitems(rtl8821au_bb_regs),
	rtl8821au_bb_regs,
	rtl8821au_bb_vals,
	nitems(rtl8821au_agc_vals),
	rtl8821au_agc_vals
};

static const struct urtwm_bb_prog rtl8821au_ext_5ghz_bb_prog = {
	nitems(rtl8821au_bb_regs),
	rtl8821au_bb_regs,
	rtl8821au_bb_vals,
	nitems(rtl8821au_ext_5ghz_agc_vals),
	rtl8821au_ext_5ghz_agc_vals
};


/*
 * RF initialization values.
 */
struct urtwm_rf_prog {
	int		count;
	const uint8_t	*regs;
	const uint32_t	*vals;
};

/*
 * RTL8821AU.
 */
static const uint8_t rtl8821au_rf_regs[] = {
	0x18, 0x56, 0x66, 0x00, 0x1e, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0xef, 0x3e, 0x3f,
	0x3e, 0x3f, 0x3e, 0x3f, 0x3e, 0x3f, 0xef, 0x18, 0x89, 0x8b, 0xef,
	0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b,
	0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a,
	0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c,
	0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b,
	0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a,
	0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0x3a, 0x3b, 0x3c, 0xef, 0xef, 0x34,
	0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34,
	0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34,
	0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0xef,
	0x18, 0xef, 0x35, 0x35, 0x35, 0x35, 0x35, 0x35, 0x35, 0x35, 0x35,
	0xef, 0x18, 0xef, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0xef, 0xef, 0x3c, 0x3c, 0x3c, 0xef, 0x18,
	0xef, 0x08, 0xef, 0xdf, 0x1f, 0x58, 0x59, 0x61, 0x62, 0x63, 0x64,
	0x65, 0x18, 0xef, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b,
	0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0xef, 0xef, 0x34,
	0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0xef,
	0xed, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
	0x44, 0xed, 0xed, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0xed, 0xef, 0xdf, 0x35, 0x35, 0x35, 0x36, 0x36, 0x36, 0x36, 0xef,
	0x51, 0x52, 0x53, 0x54, 0x56, 0x51, 0x52, 0x53, 0x70, 0x71, 0x72,
	0x74, 0x35, 0x35, 0x35, 0x36, 0x36, 0x36, 0x36, 0xed, 0x45, 0x45,
	0x45, 0x46, 0x46, 0x46, 0x46, 0xdf, 0xb3, 0xb4, 0xb7, 0x1c, 0x18,
	0xfe, 0xfe, 0x18
};

#define RTL8821AU_RF_VALS_START	\
	0x1712a, 0x51cf2, 0x40000, 0x10000, 0x80000, 0x00830, 0x21800, \
	0x28000, 0x48000, 0x94838, 0x44980, 0x48000, 0x0d480, 0x42240, \
	0xf0380, 0x90000, 0x22852, 0x65540, 0x88001, 0x20000, 0x00380, \
	0x90018, 0x20380, 0xa0018, 0x40308, 0xa0018, 0x60018, 0xa0018, \
	0x00000, 0x1712a, 0x00080, 0x80180, 0x01000, 0x00244, 0x38027, \
	0x82000, 0x00244, 0x30113, 0x82000, 0x0014c, 0x28027, 0x82000, \
	0x000cc, 0x27027, 0x42000, 0x0014c, 0x1f913, 0x42000, 0x0010c, \
	0x17f10, 0x12000, 0x000d0, 0x08027, 0xca000, 0x00244, 0x78027, \
	0x82000, 0x00244, 0x70113, 0x82000, 0x0014c, 0x68027, 0x82000, \
	0x000cc, 0x67027, 0x42000, 0x0014c, 0x5f913, 0x42000, 0x0010c, \
	0x57f10, 0x12000, 0x000d0, 0x48027, 0xca000, 0x00244, 0xb8027, \
	0x82000, 0x00244, 0xb0113, 0x82000, 0x0014c, 0xa8027, 0x82000, \
	0x000cc, 0xa7027, 0x42000, 0x0014c, 0x9f913, 0x42000, 0x0010c, \
	0x97f10, 0x12000, 0x000d0, 0x88027, 0xca000, 0x00000, 0x01100,

#define RTL8821AU_RF_VALS_END	\
	0x00006, 0x02000, 0x3824b, 0x3024b, 0x2844b, 0x20f4b, 0x18f4b, \
	0x104b2, 0x08049, 0x00148, 0x7824b, 0x7024b, 0x6824b, 0x60f4b, \
	0x58f4b, 0x504b2, 0x48049, 0x40148, 0x00000, 0x00100, 0x0adf3, \
	0x09def, 0x08dec, 0x07de9, 0x06ced, 0x05ce9, 0x044e9, 0x034e6, \
	0x0246a, 0x01467, 0x00068, 0x00000, 0x00010, 0x0adf2, 0x09def, \
	0x08dec, 0x07de9, 0x06cec, 0x05ce9, 0x044ec, 0x034e9, 0x0246c, \
	0x01469, 0x0006c, 0x00000, 0x00001, 0x38da7, 0x300c2, 0x288e2, \
	0x200b8, 0x188a5, 0x10fbc, 0x08f71, 0x00240, 0x00000, 0x020a2, \
	0x00080, 0x00120, 0x08120, 0x10120, 0x00085, 0x08085, 0x10085, \
	0x18085, 0x00000, 0x00c31, 0x00622, 0xfc70b, 0x0017e, 0x51df3, \
	0x00c01, 0x006d6, 0xfc649, 0x49661, 0x7843e, 0x00382, 0x51400, \
	0x00160, 0x08160, 0x10160, 0x00124, 0x08124, 0x10124, 0x18124, \
	0x0000c, 0x00140, 0x08140, 0x10140, 0x00124, 0x08124, 0x10124, \
	0x18124, 0x00088, 0xf0e18, 0x1214c, 0x3000c, 0x539d2, 0x1f12a, \
	0x00000, 0x00000, 0x1712a

/*
 * No external 5GHz PA/LNA.
 */
static const uint32_t rtl8821au_rf_vals[] = {
	RTL8821AU_RF_VALS_START
	0x4adf5, 0x49df2, 0x48def, 0x47dec, 0x46de9, 0x45ccb, 0x4488d,
	0x4348d, 0x4248a, 0x4108d, 0x4008a, 0x2adf4, 0x29df1, 0x28dee,
	0x27deb, 0x26ccd, 0x25cca, 0x2488c, 0x2384c, 0x22849, 0x21449,
	0x2004d, 0x0adf7, 0x09df4, 0x08df1, 0x07dee, 0x06dcd, 0x05ccd,
	0x04cca, 0x0388c, 0x02888, 0x01488, 0x00486, 0x00000, 0x1712a,
	0x00040, 0x00128, 0x08128, 0x10128, 0x201c8, 0x281c8, 0x301c8,
	0x401c8, 0x481c8, 0x501c8, 0x00000, 0x1712a, 0x00010, 0x063b5,
	0x0e3b5, 0x163b5, 0x1e3b5, 0x263b5, 0x2e3b5, 0x363b5, 0x3e3b5,
	0x463b5, 0x4e3b5, 0x563b5, 0x5e3b5, 0x00000, 0x00008, 0x001b6,
	0x00492, 0x00800, 0x00000, 0x1712a, 0x00002, 0x02000, 0x00000,
	0x000c0, 0x40064, 0x81184, 0x6016c, 0xefd83, 0x93fcc, 0x110eb,
	0x1c27c, 0x93016,
	RTL8821AU_RF_VALS_END
};

/*
 * With external 5GHz PA and LNA.
 */
static const uint32_t rtl8821au_ext_5ghz_rf_vals[] = {
	RTL8821AU_RF_VALS_START
	0x4a0f3, 0x490b1, 0x480ae, 0x470ab, 0x4608b, 0x45069, 0x44048,
	0x43045, 0x42026, 0x41023, 0x40002, 0x2a0f3, 0x290f0, 0x280af,
	0x270ac, 0x2608b, 0x25069, 0x24048, 0x23045, 0x22026, 0x21023,
	0x20002, 0x0a0d7, 0x090d3, 0x080b1, 0x070ae, 0x0608d, 0x0506b,
	0x0404a, 0x03047, 0x02044, 0x01025, 0x00004, 0x00000, 0x1712a,
	0x00040, 0x00145, 0x08145, 0x10145, 0x20196, 0x28196, 0x30196,
	0x401c7, 0x481c7, 0x501c7, 0x00000, 0x1712a, 0x00010, 0x056b3,
	0x0d6b3, 0x156b3, 0x1d6b3, 0x26634, 0x2e634, 0x36634, 0x3e634,
	0x467b4, 0x4e7b4, 0x567b4, 0x5e7b4, 0x00000, 0x00008, 0x0022a,
	0x00594, 0x00820, 0x00000, 0x1712a, 0x00002, 0x02000, 0x00000,
	0x000c0, 0x40064, 0x81184, 0x6016c, 0xead53, 0x93bc4, 0x110e9,
	0x1c67c, 0x93015,
	RTL8821AU_RF_VALS_END
};

/*
 * Others.
 */
static const uint32_t rtl8821au_1_rf_vals[] = {
	RTL8821AU_RF_VALS_START
	0x4adf7, 0x49df3, 0x48def, 0x47dec, 0x46de9, 0x45ccb, 0x4488d,
	0x4348d, 0x4248a, 0x4108d, 0x4008a, 0x2adf7, 0x29df2, 0x28dee,
	0x27deb, 0x26ccd, 0x25cca, 0x2488c, 0x2384c, 0x22849, 0x21449,
	0x2004d, 0x0adf7, 0x09df4, 0x08df1, 0x07dee, 0x06dcd, 0x05ccd,
	0x04cca, 0x0388c, 0x02888, 0x01488, 0x00486, 0x00000, 0x1712a,
	0x00040, 0x00145, 0x08145, 0x10145, 0x20196, 0x28196, 0x30196,
	0x401c7, 0x481c7, 0x501c7, 0x00000, 0x1712a, 0x00010, 0x056b3,
	0x0d6b3, 0x156b3, 0x1d6b3, 0x26634, 0x2e634, 0x36634, 0x3e634,
	0x467b4, 0x4e7b4, 0x567b4, 0x5e7b4, 0x00000, 0x00008, 0x0022a,
	0x00594, 0x00900, 0x00000, 0x1712a, 0x00002, 0x02000, 0x00000,
	0x000c0, 0x40064, 0x81184, 0x6016c, 0xead53, 0x93bc4, 0x714e9,
	0x1c67c, 0x91016,
	RTL8821AU_RF_VALS_END
};

static const struct urtwm_rf_prog rtl8821au_rf_prog[] = {
	{
		nitems(rtl8821au_rf_regs),
		rtl8821au_rf_regs,
		rtl8821au_rf_vals
	}
};

static const struct urtwm_rf_prog rtl8821au_ext_5ghz_rf_prog[] = {
	{
		nitems(rtl8821au_rf_regs),
		rtl8821au_rf_regs,
		rtl8821au_ext_5ghz_rf_vals
	}
};

static const struct urtwm_rf_prog rtl8821au_1_rf_prog[] = {
	{
		nitems(rtl8821au_rf_regs),
		rtl8821au_rf_regs,
		rtl8821au_1_rf_vals
	}
};


struct urtwn_txpwr {
	uint8_t	pwr[3][28];
};

struct urtwn_r88e_txpwr {
	uint8_t	pwr[6][28];
};

/*
 * Per RF chain/group/rate Tx gain values.
 */
static const struct urtwn_txpwr rtl8192cu_txagc[] = {
	{ {	/* Chain 0. */
	{	/* Group 0. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x0c, 0x0c, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x02,	/* OFDM6~54. */
	0x0e, 0x0d, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x02,	/* MCS0~7. */
	0x0e, 0x0d, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x02	/* MCS8~15. */
	},
	{	/* Group 1. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 2. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x04, 0x04, 0x04, 0x04, 0x04, 0x02, 0x02, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	}
	} },
	{ {	/* Chain 1. */
	{	/* Group 0. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 1. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 2. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x04, 0x04, 0x04, 0x04, 0x04, 0x02, 0x02, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	}
	} }
};

static const struct urtwn_txpwr rtl8188ru_txagc[] = {
	{ {	/* Chain 0. */
	{	/* Group 0. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x08, 0x08, 0x08, 0x06, 0x06, 0x04, 0x04, 0x00,	/* OFDM6~54. */
	0x08, 0x06, 0x06, 0x04, 0x04, 0x02, 0x02, 0x00,	/* MCS0~7. */
	0x08, 0x06, 0x06, 0x04, 0x04, 0x02, 0x02, 0x00	/* MCS8~15. */
	},
	{	/* Group 1. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 2. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	}
	} }
};

static const struct urtwn_r88e_txpwr rtl8188eu_txagc[] = {
	{ {	/* Chain 0. */
	{	/* Group 0. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 1. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 2. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 3. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 4. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	},
	{	/* Group 5. */
	0x00, 0x00, 0x00, 0x00,				/* CCK1~11. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* OFDM6~54. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* MCS0~7. */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	/* MCS8~15. */
	}
	} }
};
