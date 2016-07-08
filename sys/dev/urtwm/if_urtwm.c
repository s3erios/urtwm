/*	$OpenBSD: if_urtwn.c,v 1.16 2011/02/10 17:26:40 jakemsr Exp $	*/

/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2014 Kevin Lo <kevlo@FreeBSD.org>
 * Copyright (c) 2015-2016 Andriy Voskoboinyk <avos@FreeBSD.org>
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

/*
 * Driver for Realtek RTL8812AU/RTL8821AU.
 */
#include "opt_wlan.h"

#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/linker.h>
#include <sys/firmware.h>
#include <sys/kdb.h>

#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/rman.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_regdomain.h>
#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_ratectl.h>
#ifdef	IEEE80211_SUPPORT_SUPERG
#include <net80211/ieee80211_superg.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usb_device.h>
#include "usbdevs.h"

#include <dev/usb/usb_debug.h>

#include "if_urtwmreg.h"
#include "if_urtwmvar.h"

#ifdef USB_DEBUG
enum {
	URTWM_DEBUG_XMIT	= 0x00000001,	/* basic xmit operation */
	URTWM_DEBUG_RECV	= 0x00000002,	/* basic recv operation */
	URTWM_DEBUG_STATE	= 0x00000004,	/* 802.11 state transitions */
	URTWM_DEBUG_RA		= 0x00000008,	/* f/w rate adaptation setup */
	URTWM_DEBUG_USB		= 0x00000010,	/* usb requests */
	URTWM_DEBUG_FIRMWARE	= 0x00000020,	/* firmware(9) loading debug */
	URTWM_DEBUG_BEACON	= 0x00000040,	/* beacon handling */
	URTWM_DEBUG_INTR	= 0x00000080,	/* ISR */
	URTWM_DEBUG_TEMP	= 0x00000100,	/* temperature calibration */
	URTWM_DEBUG_ROM		= 0x00000200,	/* various ROM info */
	URTWM_DEBUG_KEY		= 0x00000400,	/* crypto keys management */
	URTWM_DEBUG_TXPWR	= 0x00000800,	/* dump Tx power values */
	URTWM_DEBUG_RSSI	= 0x00001000,	/* dump RSSI lookups */
	URTWM_DEBUG_RESET	= 0x00002000,	/* initialization progress */
	URTWM_DEBUG_CALIB	= 0x00004000,	/* calibration progress */
	URTWM_DEBUG_ANY		= 0xffffffff
};

#define URTWM_DPRINTF(_sc, _m, ...) do {			\
	if ((_sc)->sc_debug & (_m))				\
		device_printf((_sc)->sc_dev, __VA_ARGS__);	\
} while(0)

#else
#define URTWM_DPRINTF(_sc, _m, ...)	do { (void) sc; } while (0)
#endif

/* various supported device vendors/products */
static const STRUCT_USB_HOST_ID urtwm_devs[] = {
#define URTWM_DEV(v,p)  { USB_VP(USB_VENDOR_##v, USB_PRODUCT_##v##_##p) }
#define URTWM_RTL8812A_DEV(v,p) \
	{ USB_VPI(USB_VENDOR_##v, USB_PRODUCT_##v##_##p, URTWM_RTL8812A) }
#define URTWM_RTL8812A	1
	URTWM_RTL8812A_DEV(ASUS,		USBAC56),
	URTWM_RTL8812A_DEV(CISCOLINKSYS,	WUSB6300),
	URTWM_RTL8812A_DEV(DLINK,		DWA182C1),
	URTWM_RTL8812A_DEV(DLINK,		DWA180A1),
	URTWM_RTL8812A_DEV(EDIMAX,		EW7822UAC),
	URTWM_RTL8812A_DEV(IODATA,		WNAC867U),
	URTWM_RTL8812A_DEV(MELCO,		WIU3866D),
	URTWM_RTL8812A_DEV(NEC,			WL900U),
	URTWM_RTL8812A_DEV(PLANEX2,		GW900D),
	URTWM_RTL8812A_DEV(SENAO,		EUB1200AC),
	URTWM_RTL8812A_DEV(SITECOMEU,		WLA7100),
	URTWM_RTL8812A_DEV(TRENDNET,		TEW805UB),
	URTWM_RTL8812A_DEV(ZYXEL,		NWD6605),
	URTWM_DEV(DLINK,	DWA171A1),
	URTWM_DEV(DLINK,	DWA172A1),
	URTWM_DEV(EDIMAX,	EW7811UTC_1),
	URTWM_DEV(EDIMAX,	EW7811UTC_2),
	URTWM_DEV(HAWKING,	HD65U),
	URTWM_DEV(MELCO,	WIU2433DM),
	URTWM_DEV(NETGEAR,	A6100)
#undef URTWM_RTL8812A_DEV
#undef URTWM_DEV
};

static device_probe_t	urtwm_match;
static device_attach_t	urtwm_attach;
static device_detach_t	urtwm_detach;

static usb_callback_t	urtwm_bulk_tx_callback;
static usb_callback_t	urtwm_bulk_rx_callback;

static void		urtwm_radiotap_attach(struct urtwm_softc *);
static void		urtwm_sysctlattach(struct urtwm_softc *);
static void		urtwm_drain_mbufq(struct urtwm_softc *);
static usb_error_t	urtwm_do_request(struct urtwm_softc *,
			    struct usb_device_request *, void *);
static struct ieee80211vap *urtwm_vap_create(struct ieee80211com *,
		    const char [IFNAMSIZ], int, enum ieee80211_opmode, int,
                    const uint8_t [IEEE80211_ADDR_LEN],
                    const uint8_t [IEEE80211_ADDR_LEN]);
static void		urtwm_vap_delete(struct ieee80211vap *);
static void		urtwm_vap_clear_tx(struct urtwm_softc *,
			    struct ieee80211vap *);
static void		urtwm_vap_clear_tx_queue(struct urtwm_softc *,
			    urtwm_datahead *, struct ieee80211vap *);
#ifdef IEEE80211_SUPPORT_SUPERG
static void		urtwm_ff_flush_all(struct urtwm_softc *,
			    union sec_param *);
#endif
static struct mbuf *	urtwm_rx_copy_to_mbuf(struct urtwm_softc *,
			    struct r92c_rx_stat *, int);
static struct mbuf *	urtwm_report_intr(struct urtwm_softc *,
			    struct usb_xfer *, struct urtwm_data *);
static void		urtwm_c2h_report(struct urtwm_softc *, uint8_t *, int);
static void		urtwm_ratectl_tx_complete(struct urtwm_softc *,
			    void *, int);
static struct mbuf *	urtwm_rxeof(struct urtwm_softc *, uint8_t *, int);
static struct ieee80211_node *urtwm_rx_frame(struct urtwm_softc *,
			    struct mbuf *, int8_t *);
static void		urtwm_txeof(struct urtwm_softc *, struct urtwm_data *,
			    int);
static int		urtwm_alloc_list(struct urtwm_softc *,
			    struct urtwm_data[], int, int);
static int		urtwm_alloc_rx_list(struct urtwm_softc *);
static int		urtwm_alloc_tx_list(struct urtwm_softc *);
static void		urtwm_free_list(struct urtwm_softc *,
			    struct urtwm_data data[], int);
static void		urtwm_free_rx_list(struct urtwm_softc *);
static void		urtwm_free_tx_list(struct urtwm_softc *);
static struct urtwm_data *	_urtwm_getbuf(struct urtwm_softc *);
static struct urtwm_data *	urtwm_getbuf(struct urtwm_softc *);
static usb_error_t	urtwm_write_region_1(struct urtwm_softc *, uint16_t,
			    uint8_t *, int);
static usb_error_t	urtwm_write_1(struct urtwm_softc *, uint16_t, uint8_t);
static usb_error_t	urtwm_write_2(struct urtwm_softc *, uint16_t, uint16_t);
static usb_error_t	urtwm_write_4(struct urtwm_softc *, uint16_t, uint32_t);
static usb_error_t	urtwm_read_region_1(struct urtwm_softc *, uint16_t,
			    uint8_t *, int);
static uint8_t		urtwm_read_1(struct urtwm_softc *, uint16_t);
static uint16_t		urtwm_read_2(struct urtwm_softc *, uint16_t);
static uint32_t		urtwm_read_4(struct urtwm_softc *, uint16_t);
static usb_error_t	urtwm_setbits_1(struct urtwm_softc *, uint16_t,
			    uint8_t, uint8_t);
static usb_error_t	urtwm_setbits_1_shift(struct urtwm_softc *, uint16_t,
			    uint32_t, uint32_t, int);
static usb_error_t	urtwm_setbits_2(struct urtwm_softc *, uint16_t,
			    uint16_t, uint16_t);
static usb_error_t	urtwm_setbits_4(struct urtwm_softc *, uint16_t,
			    uint32_t, uint32_t);
#ifndef URTWM_WITHOUT_UCODE
static int		urtwm_fw_cmd(struct urtwm_softc *, uint8_t,
			    const void *, int);
#endif
static void		urtwm_cmdq_cb(void *, int);
static int		urtwm_cmd_sleepable(struct urtwm_softc *, const void *,
			    size_t, CMD_FUNC_PROTO);
static void		urtwm_rf_write(struct urtwm_softc *, int,
			    uint8_t, uint32_t);
static uint32_t		urtwm_r12a_rf_read(struct urtwm_softc *, int, uint8_t);
static uint32_t		urtwm_r21a_rf_read(struct urtwm_softc *, int, uint8_t);
static void		urtwm_rf_setbits(struct urtwm_softc *, int, uint8_t,
			    uint32_t, uint32_t);
static int		urtwm_llt_write(struct urtwm_softc *, uint32_t,
			    uint32_t);
static int		urtwm_efuse_read_next(struct urtwm_softc *, uint8_t *);
static int		urtwm_efuse_read_data(struct urtwm_softc *, uint8_t *,
			    uint8_t, uint8_t);
#ifdef USB_DEBUG
static void		urtwm_dump_rom_contents(struct urtwm_softc *,
			    uint8_t *, uint16_t);
#endif
static int		urtwm_efuse_read(struct urtwm_softc *, uint8_t *,
			    uint16_t);
static int		urtwm_efuse_switch_power(struct urtwm_softc *);
static int		urtwm_setup_endpoints(struct urtwm_softc *);
static int		urtwm_read_chipid(struct urtwm_softc *);
static int		urtwm_r12a_check_condition(struct urtwm_softc *,
			    const uint8_t[]);
static int		urtwm_r21a_check_condition(struct urtwm_softc *,
			    const uint8_t[]);
static void		urtwm_config_specific(struct urtwm_softc *);
static void		urtwm_config_specific_rom(struct urtwm_softc *);
static int		urtwm_read_rom(struct urtwm_softc *);
static void		urtwm_r12a_parse_rom(struct urtwm_softc *,
			    struct r12a_rom *);
static void		urtwm_r21a_parse_rom(struct urtwm_softc *,
			    struct r12a_rom *);
static void		urtwm_parse_rom(struct urtwm_softc *,
			    struct r12a_rom *);
#ifdef URTWM_TODO
static int		urtwm_ra_init(struct urtwm_softc *);
#endif
static int		urtwm_ioctl_reset(struct ieee80211vap *, u_long);
static void		urtwm_init_beacon(struct urtwm_softc *,
			    struct urtwm_vap *);
static int		urtwm_setup_beacon(struct urtwm_softc *,
			    struct ieee80211_node *);
static void		urtwm_update_beacon(struct ieee80211vap *, int);
static int		urtwm_tx_beacon(struct urtwm_softc *sc,
			    struct urtwm_vap *);
#ifndef URTWM_WITHOUT_UCODE
static int		urtwm_construct_nulldata(struct urtwm_softc *,
			    struct ieee80211vap *, uint8_t *, int);
static int		urtwm_push_nulldata(struct urtwm_softc *,
			    struct ieee80211vap *);
static void		urtwm_pwrmode_init(void *);
static void		urtwm_set_pwrmode_cb(struct urtwm_softc *,
			    union sec_param *);
static int		urtwm_set_pwrmode(struct urtwm_softc *,
			    struct ieee80211vap *, int);
static void		urtwm_set_media_status(struct urtwm_softc *,
			    union sec_param *);
#endif
static int		urtwm_key_alloc(struct ieee80211vap *,
			    struct ieee80211_key *, ieee80211_keyix *,
			    ieee80211_keyix *);
static void		urtwm_key_set_cb(struct urtwm_softc *,
			    union sec_param *);
static void		urtwm_key_del_cb(struct urtwm_softc *,
			    union sec_param *);
static int		urtwm_process_key(struct ieee80211vap *,
			    const struct ieee80211_key *, int);
static int		urtwm_key_set(struct ieee80211vap *,
			    const struct ieee80211_key *);
static int		urtwm_key_delete(struct ieee80211vap *,
			    const struct ieee80211_key *);
static void		urtwm_tsf_sync_adhoc(void *);
static void		urtwm_tsf_sync_adhoc_task(void *, int);
static void		urtwm_tsf_sync_enable(struct urtwm_softc *,
			    struct ieee80211vap *);
static uint32_t		urtwm_get_tsf_low(struct urtwm_softc *, int);
static uint32_t		urtwm_get_tsf_high(struct urtwm_softc *, int);
static void		urtwm_get_tsf(struct urtwm_softc *, uint64_t *, int);
static void		urtwm_r12a_set_led_mini(struct urtwm_softc *, int,
			    int);
static void		urtwm_r12a_set_led(struct urtwm_softc *, int, int);
static void		urtwm_r21a_set_led(struct urtwm_softc *, int, int);
static void		urtwm_set_mode(struct urtwm_softc *, uint8_t, int);
static void		urtwm_adhoc_recv_mgmt(struct ieee80211_node *,
			    struct mbuf *, int,
			    const struct ieee80211_rx_stats *, int, int);
static int		urtwm_newstate(struct ieee80211vap *,
			    enum ieee80211_state, int);
static void		urtwm_calib_to(void *);
static void		urtwm_calib_cb(struct urtwm_softc *,
			    union sec_param *);
static int8_t		urtwm_r12a_get_rssi_cck(struct urtwm_softc *, void *);
static int8_t		urtwm_r21a_get_rssi_cck(struct urtwm_softc *, void *);
static int8_t		urtwm_get_rssi_ofdm(struct urtwm_softc *, void *);
static int8_t		urtwm_get_rssi(struct urtwm_softc *, int, void *);
static void		urtwm_tx_protection(struct urtwm_softc *,
			    struct r12a_tx_desc *, enum ieee80211_protmode);
static void		urtwm_tx_raid(struct urtwm_softc *,
			    struct r12a_tx_desc *, struct ieee80211_node *,
			    int);
static void		urtwm_tx_set_sgi(struct urtwm_softc *,
			    struct r12a_tx_desc *, struct ieee80211_node *);
static int		urtwm_tx_data(struct urtwm_softc *,
			    struct ieee80211_node *, struct mbuf *,
			    struct urtwm_data *);
static int		urtwm_tx_raw(struct urtwm_softc *,
			    struct ieee80211_node *, struct mbuf *,
			    struct urtwm_data *,
			    const struct ieee80211_bpf_params *);
static void		urtwm_tx_start(struct urtwm_softc *, struct mbuf *,
			    uint8_t, struct urtwm_data *);
static void		urtwm_tx_checksum(struct r12a_tx_desc *);
static int		urtwm_transmit(struct ieee80211com *, struct mbuf *);
static void		urtwm_start(struct urtwm_softc *);
static void		urtwm_parent(struct ieee80211com *);
static int		urtwm_ioctl_net(struct ieee80211com *, u_long, void *);
static int		urtwm_r12a_power_on(struct urtwm_softc *);
static int		urtwm_r21a_power_on(struct urtwm_softc *);
static void		urtwm_r12a_power_off(struct urtwm_softc *);
static void		urtwm_r21a_power_off(struct urtwm_softc *);
static int		urtwm_llt_init(struct urtwm_softc *);
#ifndef URTWM_WITHOUT_UCODE
static void		urtwm_r12a_fw_reset(struct urtwm_softc *);
static void		urtwm_r21a_fw_reset(struct urtwm_softc *);
static usb_error_t	urtwm_fw_loadpage(struct urtwm_softc *, int,
			    const uint8_t *, int);
static int		urtwm_fw_checksum_report(struct urtwm_softc *);
static int		urtwm_load_firmware(struct urtwm_softc *);
#endif
static int		urtwm_r12a_set_page_size(struct urtwm_softc *);
static int		urtwm_r21a_set_page_size(struct urtwm_softc *);
static int		urtwm_dma_init(struct urtwm_softc *);
static int		urtwm_mac_init(struct urtwm_softc *);
static void		urtwm_bb_init(struct urtwm_softc *);
static void		urtwm_r12a_crystalcap_write(struct urtwm_softc *);
static void		urtwm_r21a_crystalcap_write(struct urtwm_softc *);
static void		urtwm_rf_init(struct urtwm_softc *);
static int		urtwm_rf_init_chain(struct urtwm_softc *,
			    const struct urtwm_rf_prog *, int);
static void		urtwm_arfb_init(struct urtwm_softc *);
static void		urtwm_r21a_bypass_ext_lna_2ghz(struct urtwm_softc *);
static void		urtwm_r12a_set_band_2ghz(struct urtwm_softc *);
static void		urtwm_r21a_set_band_2ghz(struct urtwm_softc *);
static void		urtwm_r12a_set_band_5ghz(struct urtwm_softc *);
static void		urtwm_r21a_set_band_5ghz(struct urtwm_softc *);
static void		urtwm_set_band(struct urtwm_softc *,
			    struct ieee80211_channel *, int);
static void		urtwm_cam_init(struct urtwm_softc *);
static int		urtwm_cam_write(struct urtwm_softc *, uint32_t,
			    uint32_t);
static void		urtwm_rxfilter_init(struct urtwm_softc *);
static void		urtwm_edca_init(struct urtwm_softc *);
static void		urtwm_mrr_init(struct urtwm_softc *);
static void		urtwm_write_txpower(struct urtwm_softc *, int,
			    struct ieee80211_channel *, uint16_t[]);
static int		urtwm_get_power_group(struct urtwm_softc *,
			    struct ieee80211_channel *);
static void		urtwm_get_txpower(struct urtwm_softc *, int,
		      	    struct ieee80211_channel *, uint16_t[]);
static void		urtwm_set_txpower(struct urtwm_softc *,
		    	    struct ieee80211_channel *);
static void		urtwm_set_rx_bssid_all(struct urtwm_softc *, int);
static void		urtwm_scan_start(struct ieee80211com *);
static void		urtwm_scan_curchan(struct ieee80211_scan_state *,
			    unsigned long);
static void		urtwm_scan_end(struct ieee80211com *);
static void		urtwm_getradiocaps(struct ieee80211com *, int, int *,
			    struct ieee80211_channel[]);
static void		urtwm_update_chw(struct ieee80211com *);
static void		urtwm_set_channel(struct ieee80211com *);
static int		urtwm_wme_update(struct ieee80211com *);
static void		urtwm_update_slot(struct ieee80211com *);
static void		urtwm_update_slot_cb(struct urtwm_softc *,
			    union sec_param *);
static void		urtwm_update_aifs(struct urtwm_softc *, uint8_t);
static uint8_t		urtwm_get_multi_pos(const uint8_t[]);
static void		urtwm_set_multi(struct urtwm_softc *);
static void		urtwm_set_promisc(struct urtwm_softc *);
static void		urtwm_update_promisc(struct ieee80211com *);
static void		urtwm_update_mcast(struct ieee80211com *);
static struct ieee80211_node *urtwm_node_alloc(struct ieee80211vap *,
			    const uint8_t mac[IEEE80211_ADDR_LEN]);
static void		urtwm_newassoc(struct ieee80211_node *, int);
static void		urtwm_node_free(struct ieee80211_node *);
static void		urtwm_fix_spur(struct urtwm_softc *,
			    struct ieee80211_channel *);
static void		urtwm_set_chan(struct urtwm_softc *,
		    	    struct ieee80211_channel *);
static void		urtwm_antsel_init(struct urtwm_softc *);
static void		urtwm_iq_calib_sw(struct urtwm_softc *);
#ifndef URTWM_WITHOUT_UCODE
static int		urtwm_iq_calib_fw_supported(struct urtwm_softc *);
static void		urtwm_iq_calib_fw(struct urtwm_softc *);
#endif
static void		urtwm_iq_calib(struct urtwm_softc *);
static void		urtwm_lc_calib(struct urtwm_softc *);
static void		urtwm_temp_calib(struct urtwm_softc *);
static int		urtwm_init(struct urtwm_softc *);
static void		urtwm_stop(struct urtwm_softc *);
static void		urtwm_abort_xfers(struct urtwm_softc *);
static int		urtwm_raw_xmit(struct ieee80211_node *, struct mbuf *,
			    const struct ieee80211_bpf_params *);
static void		urtwm_delay(struct urtwm_softc *, int);

/* Aliases. */
#define	urtwm_bb_write		urtwm_write_4
#define urtwm_bb_read		urtwm_read_4
#define urtwm_bb_setbits	urtwm_setbits_4

#define urtwm_rf_read(_sc, _chain, _addr) \
	(((_sc)->sc_rf_read)((_sc), (_chain), (_addr)))
#define urtwm_check_condition(_sc, _cond) \
	(((_sc)->sc_check_condition)((_sc), (_cond)))
#define urtwm_parse_rom_specific(_sc, _rom) \
	(((_sc)->sc_parse_rom)((_sc), (_rom)))
#define urtwm_set_led(_sc, _led, _on) \
	(((_sc)->sc_set_led)((_sc), (_led), (_on)))
#define urtwm_get_rssi_cck(_sc, _physt) \
	(((_sc)->sc_get_rssi_cck)((_sc), (_physt)))
#define urtwm_power_on(_sc) \
	(((_sc)->sc_power_on)((_sc)))
#define urtwm_power_off(_sc) \
	(((_sc)->sc_power_off)((_sc)))
#define urtwm_fw_reset(_sc) \
	(((_sc)->sc_fw_reset)((_sc)))
#define urtwm_set_page_size(_sc) \
	(((_sc)->sc_set_page_size)((_sc)))
#define urtwm_crystalcap_write(_sc) \
	(((_sc)->sc_crystalcap_write)((_sc)))
#define urtwm_set_band_2ghz(_sc) \
	(((_sc)->sc_set_band_2ghz)((_sc)))
#define urtwm_set_band_5ghz(_sc) \
	(((_sc)->sc_set_band_5ghz)((_sc)))

static struct usb_config urtwm_config[URTWM_N_TRANSFER] = {
	[URTWM_BULK_RX] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_IN,
		.bufsize = URTWM_RXBUFSZ,
		.flags = {
			.pipe_bof = 1,
			.short_xfer_ok = 1
		},
		.callback = urtwm_bulk_rx_callback,
	},
	[URTWM_BULK_TX_BE] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_OUT,
		.bufsize = URTWM_TXBUFSZ,
		.flags = {
			.ext_buffer = 1,
			.pipe_bof = 1,
			.force_short_xfer = 1,
		},
		.callback = urtwm_bulk_tx_callback,
		.timeout = URTWM_TX_TIMEOUT,	/* ms */
	},
	[URTWM_BULK_TX_BK] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_OUT,
		.bufsize = URTWM_TXBUFSZ,
		.flags = {
			.ext_buffer = 1,
			.pipe_bof = 1,
			.force_short_xfer = 1,
		},
		.callback = urtwm_bulk_tx_callback,
		.timeout = URTWM_TX_TIMEOUT,	/* ms */
	},
	[URTWM_BULK_TX_VI] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_OUT,
		.bufsize = URTWM_TXBUFSZ,
		.flags = {
			.ext_buffer = 1,
			.pipe_bof = 1,
			.force_short_xfer = 1
		},
		.callback = urtwm_bulk_tx_callback,
		.timeout = URTWM_TX_TIMEOUT,	/* ms */
	},
	[URTWM_BULK_TX_VO] = {
		.type = UE_BULK,
		.endpoint = UE_ADDR_ANY,
		.direction = UE_DIR_OUT,
		.bufsize = URTWM_TXBUFSZ,
		.flags = {
			.ext_buffer = 1,
			.pipe_bof = 1,
			.force_short_xfer = 1
		},
		.callback = urtwm_bulk_tx_callback,
		.timeout = URTWM_TX_TIMEOUT,	/* ms */
	},
};

static const struct wme_to_queue {
	uint16_t reg;
	uint8_t qid;
} wme2queue[WME_NUM_AC] = {
	{ R92C_EDCA_BE_PARAM, URTWM_BULK_TX_BE},
	{ R92C_EDCA_BK_PARAM, URTWM_BULK_TX_BK},
	{ R92C_EDCA_VI_PARAM, URTWM_BULK_TX_VI},
	{ R92C_EDCA_VO_PARAM, URTWM_BULK_TX_VO}
};

static const uint8_t urtwm_chan_2ghz[] =
	{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };

static const uint8_t urtwm_chan_5ghz[] =
	{ 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64,
	  100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124,
	  126, 128, 130, 132, 134, 136, 138, 140, 142, 144,
	  149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 168, 169, 171,
	  173, 175, 177 };

static int
urtwm_match(device_t self)
{
	struct usb_attach_arg *uaa = device_get_ivars(self);

	if (uaa->usb_mode != USB_MODE_HOST)
		return (ENXIO);
	if (uaa->info.bConfigIndex != URTWM_CONFIG_INDEX)
		return (ENXIO);
	if (uaa->info.bIfaceIndex != URTWM_IFACE_INDEX)
		return (ENXIO);

	return (usbd_lookup_id_by_uaa(urtwm_devs, sizeof(urtwm_devs), uaa));
}

static int
urtwm_attach(device_t self)
{
	struct usb_attach_arg *uaa = device_get_ivars(self);
	struct urtwm_softc *sc = device_get_softc(self);
	struct ieee80211com *ic = &sc->sc_ic;
	int error;

	device_set_usb_desc(self);
	sc->sc_flags = URTWM_RXCKSUM_EN | URTWM_RXCKSUM6_EN;
	sc->sc_udev = uaa->device;
	sc->sc_dev = self;
	if (USB_GET_DRIVER_INFO(uaa) == URTWM_RTL8812A)
		sc->chip |= URTWM_CHIP_12A;

#ifdef USB_DEBUG
	int debug;
	if (resource_int_value(device_get_name(sc->sc_dev),
	    device_get_unit(sc->sc_dev), "debug", &debug) == 0)
		sc->sc_debug = debug;
#endif

	mtx_init(&sc->sc_mtx, device_get_nameunit(self),
	    MTX_NETWORK_LOCK, MTX_DEF);
	URTWM_CMDQ_LOCK_INIT(sc);
	URTWM_NT_LOCK_INIT(sc);
	callout_init(&sc->sc_calib_to, 0);
	callout_init(&sc->sc_pwrmode_init, 0);
	mbufq_init(&sc->sc_snd, ifqmaxlen);

	error = urtwm_setup_endpoints(sc);
	if (error != 0)
		goto detach;

	URTWM_LOCK(sc);
	error = urtwm_read_chipid(sc);
	URTWM_UNLOCK(sc);
	if (error) {
		device_printf(sc->sc_dev, "unsupported test chip\n");
		goto detach;
	}

	/* Setup device-specific configuration (before ROM parsing). */
	urtwm_config_specific(sc);

	error = urtwm_read_rom(sc);
	if (error != 0) {
		device_printf(sc->sc_dev, "%s: cannot read rom, error %d\n",
		    __func__, error);
		goto detach;
	}

	/* Setup device-specific configuration (after ROM parsing). */
	urtwm_config_specific_rom(sc);

	device_printf(sc->sc_dev, "MAC/BB RTL%sAU, RF 6052 %dT%dR\n",
	    URTWM_CHIP_IS_12A(sc) ? "8812" : "8821",
	    sc->ntxchains, sc->nrxchains);

	ic->ic_softc = sc;
	ic->ic_name = device_get_nameunit(self);
	ic->ic_phytype = IEEE80211_T_OFDM;	/* not only, but not used */
	ic->ic_opmode = IEEE80211_M_STA;	/* default to BSS mode */

	/* set device capabilities */
	ic->ic_caps =
		  IEEE80211_C_STA		/* station mode */
		| IEEE80211_C_MONITOR		/* monitor mode */
		| IEEE80211_C_IBSS		/* adhoc mode */
		| IEEE80211_C_HOSTAP		/* hostap mode */
#ifndef URTWM_WITHOUT_UCODE
		| IEEE80211_C_PMGT		/* Station-side power mgmt */
#endif
		| IEEE80211_C_SHPREAMBLE	/* short preamble supported */
		| IEEE80211_C_SHSLOT		/* short slot time supported */
#if 0
		| IEEE80211_C_BGSCAN		/* capable of bg scanning */
#endif
		| IEEE80211_C_WPA		/* 802.11i */
		| IEEE80211_C_WME		/* 802.11e */
		| IEEE80211_C_SWAMSDUTX		/* Do software A-MSDU TX */
		| IEEE80211_C_FF		/* Atheros fast-frames */
		;

	ic->ic_cryptocaps =
	    IEEE80211_CRYPTO_WEP |
	    IEEE80211_CRYPTO_TKIP |
	    IEEE80211_CRYPTO_AES_CCM;

	ic->ic_htcaps =
	      IEEE80211_HTCAP_SHORTGI20		/* short GI in 20MHz */
#ifdef URTWM_TODO	/* no HT40 just yet */
	    | IEEE80211_HTCAP_CHWIDTH40		/* 40 MHz channel width */
	    | IEEE80211_HTCAP_SHORTGI40		/* short GI in 40MHz */
#endif
	    | IEEE80211_HTCAP_MAXAMSDU_3839	/* max A-MSDU length */
	    | IEEE80211_HTCAP_SMPS_OFF		/* SM PS mode disabled */
	    /* s/w capabilities */
	    | IEEE80211_HTC_HT			/* HT operation */
	    | IEEE80211_HTC_AMPDU		/* A-MPDU tx */
	    | IEEE80211_HTC_AMSDU		/* A-MSDU tx */
	    ;

	ic->ic_txstream = sc->ntxchains;
	ic->ic_rxstream = sc->nrxchains;

	/* Enable TX watchdog */
#ifdef D4054
	ic->ic_flags_ext |= IEEE80211_FEXT_WATCHDOG;
#endif

	urtwm_getradiocaps(ic, IEEE80211_CHAN_MAX, &ic->ic_nchans,
	    ic->ic_channels);

	ieee80211_ifattach(ic);
	ic->ic_raw_xmit = urtwm_raw_xmit;
	ic->ic_scan_start = urtwm_scan_start;
	sc->sc_scan_curchan = ic->ic_scan_curchan;
	ic->ic_scan_curchan = urtwm_scan_curchan;
	ic->ic_scan_end = urtwm_scan_end;
	ic->ic_getradiocaps = urtwm_getradiocaps;
	ic->ic_update_chw = urtwm_update_chw;
	ic->ic_set_channel = urtwm_set_channel;
	ic->ic_transmit = urtwm_transmit;
	ic->ic_parent = urtwm_parent;
	ic->ic_ioctl = urtwm_ioctl_net;
	ic->ic_vap_create = urtwm_vap_create;
	ic->ic_vap_delete = urtwm_vap_delete;
	ic->ic_wme.wme_update = urtwm_wme_update;
	ic->ic_updateslot = urtwm_update_slot;
	ic->ic_update_promisc = urtwm_update_promisc;
	ic->ic_update_mcast = urtwm_update_mcast;
	ic->ic_node_alloc = urtwm_node_alloc;
	ic->ic_newassoc = urtwm_newassoc;
	sc->sc_node_free = ic->ic_node_free;
	ic->ic_node_free = urtwm_node_free;

	TASK_INIT(&sc->cmdq_task, 0, urtwm_cmdq_cb, sc);

	urtwm_radiotap_attach(sc);
	urtwm_sysctlattach(sc);

	if (bootverbose)
		ieee80211_announce(ic);

	return (0);

detach:
	urtwm_detach(self);
	return (ENXIO);			/* failure */
}

static void
urtwm_radiotap_attach(struct urtwm_softc *sc)
{
	struct urtwm_rx_radiotap_header *rxtap = &sc->sc_rxtap;
	struct urtwm_tx_radiotap_header *txtap = &sc->sc_txtap;

	ieee80211_radiotap_attach(&sc->sc_ic,
	    &txtap->wt_ihdr, sizeof(*txtap), URTWM_TX_RADIOTAP_PRESENT,
	    &rxtap->wr_ihdr, sizeof(*rxtap), URTWM_RX_RADIOTAP_PRESENT);
}

static void
urtwm_sysctlattach(struct urtwm_softc *sc)
{
#ifdef USB_DEBUG
	struct sysctl_ctx_list *ctx = device_get_sysctl_ctx(sc->sc_dev);
	struct sysctl_oid *tree = device_get_sysctl_tree(sc->sc_dev);

	SYSCTL_ADD_U32(ctx, SYSCTL_CHILDREN(tree), OID_AUTO,
	    "debug", CTLFLAG_RW, &sc->sc_debug, sc->sc_debug,
	    "control debugging printfs");
#endif
}

static int
urtwm_detach(device_t self)
{
	struct urtwm_softc *sc = device_get_softc(self);
	struct ieee80211com *ic = &sc->sc_ic;

	/* Prevent further ioctls. */
	URTWM_LOCK(sc);
	sc->sc_flags |= URTWM_DETACHED;
	URTWM_UNLOCK(sc);

	callout_drain(&sc->sc_calib_to);

	urtwm_stop(sc);

	/* stop all USB transfers */
	usbd_transfer_unsetup(sc->sc_xfer, URTWM_N_TRANSFER);

	if (ic->ic_softc == sc) {
		callout_drain(&sc->sc_pwrmode_init);
		ieee80211_draintask(ic, &sc->cmdq_task);
		ieee80211_ifdetach(ic);
	}

	URTWM_NT_LOCK_DESTROY(sc);
	URTWM_CMDQ_LOCK_DESTROY(sc);
	mtx_destroy(&sc->sc_mtx);

	return (0);
}

static void
urtwm_drain_mbufq(struct urtwm_softc *sc)
{
	struct mbuf *m;
	struct ieee80211_node *ni;
	URTWM_ASSERT_LOCKED(sc);
	while ((m = mbufq_dequeue(&sc->sc_snd)) != NULL) {
		ni = (struct ieee80211_node *)m->m_pkthdr.rcvif;
		m->m_pkthdr.rcvif = NULL;
		ieee80211_free_node(ni);
		m_freem(m);
	}
}

static usb_error_t
urtwm_do_request(struct urtwm_softc *sc, struct usb_device_request *req,
    void *data)
{
	usb_error_t err;
	int ntries = 10;

	URTWM_ASSERT_LOCKED(sc);

	while (ntries--) {
		err = usbd_do_request_flags(sc->sc_udev, &sc->sc_mtx,
		    req, data, 0, NULL, 250 /* ms */);
		if (err == 0)
			break;

		URTWM_DPRINTF(sc, URTWM_DEBUG_USB,
		    "%s: control request failed, %s (retries left: %d)\n",
		    __func__, usbd_errstr(err), ntries);
		usb_pause_mtx(&sc->sc_mtx, hz / 100);
	}
	return (err);
}

static struct ieee80211vap *
urtwm_vap_create(struct ieee80211com *ic, const char name[IFNAMSIZ], int unit,
    enum ieee80211_opmode opmode, int flags,
    const uint8_t bssid[IEEE80211_ADDR_LEN],
    const uint8_t mac[IEEE80211_ADDR_LEN])
{
	struct urtwm_softc *sc = ic->ic_softc;
	struct urtwm_vap *uvp;
	struct ieee80211vap *vap;
	struct ifnet *ifp;

	if (!TAILQ_EMPTY(&ic->ic_vaps))		/* only one at a time */
		return (NULL);

	uvp = malloc(sizeof(struct urtwm_vap), M_80211_VAP, M_WAITOK | M_ZERO);
	vap = &uvp->vap;
	/* enable s/w bmiss handling for sta mode */

	if (ieee80211_vap_setup(ic, vap, name, unit, opmode,
	    flags | IEEE80211_CLONE_NOBEACONS, bssid) != 0) {
		/* out of memory */
		free(uvp, M_80211_VAP);
		return (NULL);
	}

	ifp = vap->iv_ifp;
	ifp->if_capabilities = IFCAP_RXCSUM | IFCAP_RXCSUM_IPV6;
	URTWM_LOCK(sc);
	if (sc->sc_flags & URTWM_RXCKSUM_EN)
		ifp->if_capenable |= IFCAP_RXCSUM;
	if (sc->sc_flags & URTWM_RXCKSUM6_EN)
		ifp->if_capenable |= IFCAP_RXCSUM_IPV6;
	URTWM_UNLOCK(sc);

	urtwm_init_beacon(sc, uvp);

	/* override state transition machine */
	uvp->newstate = vap->iv_newstate;
	vap->iv_newstate = urtwm_newstate;
	vap->iv_update_beacon = urtwm_update_beacon;
	vap->iv_reset = urtwm_ioctl_reset;
	vap->iv_key_alloc = urtwm_key_alloc;
	vap->iv_key_set = urtwm_key_set;
	vap->iv_key_delete = urtwm_key_delete;
	vap->iv_max_aid = URTWM_MACID_MAX(sc) + 1;

	/* 802.11n parameters */
	vap->iv_ampdu_density = IEEE80211_HTCAP_MPDUDENSITY_16;
	vap->iv_ampdu_rxmax = IEEE80211_HTCAP_MAXRXAMPDU_64K;

	if (opmode == IEEE80211_M_IBSS) {
		uvp->recv_mgmt = vap->iv_recv_mgmt;
		vap->iv_recv_mgmt = urtwm_adhoc_recv_mgmt;
		TASK_INIT(&uvp->tsf_sync_adhoc_task, 0,
		    urtwm_tsf_sync_adhoc_task, vap);
		callout_init(&uvp->tsf_sync_adhoc, 0);
	}

	ieee80211_ratectl_init(vap);
	/* complete setup */
	ieee80211_vap_attach(vap, ieee80211_media_change,
	    ieee80211_media_status, mac);
	ic->ic_opmode = opmode;
	return (vap);
}

static void
urtwm_vap_delete(struct ieee80211vap *vap)
{
	struct ieee80211com *ic = vap->iv_ic;
	struct urtwm_softc *sc = ic->ic_softc;
	struct urtwm_vap *uvp = URTWM_VAP(vap);

	/* Guarantee that nothing will go through this vap. */
	ieee80211_new_state(vap, IEEE80211_S_INIT, -1);
	ieee80211_draintask(ic, &vap->iv_nstate_task);

	URTWM_LOCK(sc);
	if (uvp->bcn_mbuf != NULL)
		m_freem(uvp->bcn_mbuf);
	/* Cancel any unfinished Tx. */
	urtwm_vap_clear_tx(sc, vap);
	URTWM_UNLOCK(sc);
	if (vap->iv_opmode == IEEE80211_M_IBSS) {
		ieee80211_draintask(ic, &uvp->tsf_sync_adhoc_task);
		callout_drain(&uvp->tsf_sync_adhoc);
	}
	ieee80211_ratectl_deinit(vap);
	ieee80211_vap_detach(vap);
	free(uvp, M_80211_VAP);
}

static void
urtwm_vap_clear_tx(struct urtwm_softc *sc, struct ieee80211vap *vap)
{

	URTWM_ASSERT_LOCKED(sc);

	urtwm_vap_clear_tx_queue(sc, &sc->sc_tx_active, vap);
	urtwm_vap_clear_tx_queue(sc, &sc->sc_tx_pending, vap);
}

static void
urtwm_vap_clear_tx_queue(struct urtwm_softc *sc, urtwm_datahead *head,
    struct ieee80211vap *vap)
{
	struct urtwm_data *dp, *tmp;

	STAILQ_FOREACH_SAFE(dp, head, next, tmp) {
		if (dp->ni != NULL) {
			if (dp->ni->ni_vap == vap) {
				ieee80211_free_node(dp->ni);
				dp->ni = NULL;

				if (dp->m != NULL) {
					m_freem(dp->m);
					dp->m = NULL;
				}

				STAILQ_REMOVE(head, dp, urtwm_data, next);
				STAILQ_INSERT_TAIL(&sc->sc_tx_inactive, dp,
				    next);
			}
		}
	}
}

#ifdef IEEE80211_SUPPORT_SUPERG
static void
urtwm_ff_flush_all(struct urtwm_softc *sc, union sec_param *data)
{
	struct ieee80211com *ic = &sc->sc_ic;

	URTWM_UNLOCK(sc);
#ifdef D6958
	ieee80211_ff_flush_all(ic);
#else
	ieee80211_ff_flush(ic, WME_AC_VO);
	ieee80211_ff_flush(ic, WME_AC_VI);
	ieee80211_ff_flush(ic, WME_AC_BE);
	ieee80211_ff_flush(ic, WME_AC_BK);
#endif
	URTWM_LOCK(sc);
}
#endif

static struct mbuf *
urtwm_rx_copy_to_mbuf(struct urtwm_softc *sc, struct r92c_rx_stat *stat,
    int totlen)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct mbuf *m;
	uint32_t rxdw0, rxdw1;
	int pktlen;

	URTWM_ASSERT_LOCKED(sc);

	/*
	 * don't pass packets to the ieee80211 framework if the driver isn't
	 * RUNNING.
	 */
	if (!(sc->sc_flags & URTWM_RUNNING))
		return (NULL);

	rxdw0 = le32toh(stat->rxdw0);
	if (__predict_false(rxdw0 & (R92C_RXDW0_CRCERR | R92C_RXDW0_ICVERR))) {
		/*
		 * This should not happen since we setup our Rx filter
		 * to not receive these frames.
		 */
		URTWM_DPRINTF(sc, URTWM_DEBUG_RECV,
		    "%s: RX flags error (%s)\n", __func__,
		    rxdw0 & R92C_RXDW0_CRCERR ? "CRC" : "ICV");
		goto fail;
	}

	pktlen = MS(rxdw0, R92C_RXDW0_PKTLEN);
	if (__predict_false(pktlen < sizeof(struct ieee80211_frame_ack))) {
		/*
		 * Should not happen (because of Rx filter setup).
		 */
		URTWM_DPRINTF(sc, URTWM_DEBUG_RECV,
		    "%s: frame is too short: %d\n", __func__, pktlen);
		goto fail;
	}

	m = m_get2(totlen, M_NOWAIT, MT_DATA, M_PKTHDR);
	if (__predict_false(m == NULL)) {
		device_printf(sc->sc_dev, "%s: could not allocate RX mbuf\n",
		    __func__);
		goto fail;
	}

	/* Finalize mbuf. */
	memcpy(mtod(m, uint8_t *), (uint8_t *)stat, totlen);
	m->m_pkthdr.len = m->m_len = totlen;

	rxdw1 = le32toh(stat->rxdw1);
	if (rxdw1 & R12A_RXDW1_CKSUM) {
		URTWM_DPRINTF(sc, URTWM_DEBUG_RECV,
		    "%s: %s/%s checksum is %s\n", __func__,
		    (rxdw1 & R12A_RXDW1_UDP) ? "UDP" : "TCP",
		    (rxdw1 & R12A_RXDW1_IPV6) ? "IPv6" : "IP",
		    (rxdw1 & R12A_RXDW1_CKSUM_ERR) ? "invalid" : "valid");

		if (rxdw1 & R12A_RXDW1_CKSUM_ERR) {
			m_freem(m);
			goto fail;
		}

		if ((rxdw1 & R12A_RXDW1_IPV6) ?
		    (sc->sc_flags & URTWM_RXCKSUM6_EN) :
		    (sc->sc_flags & URTWM_RXCKSUM_EN)) {
			m->m_pkthdr.csum_flags = CSUM_IP_CHECKED |
			    CSUM_IP_VALID | CSUM_DATA_VALID | CSUM_PSEUDO_HDR;
			m->m_pkthdr.csum_data = 0xffff;
		}
	}

	return (m);
fail:
	counter_u64_add(ic->ic_ierrors, 1);
	return (NULL);
}

static struct mbuf *
urtwm_report_intr(struct urtwm_softc *sc, struct usb_xfer *xfer,
    struct urtwm_data *data)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r92c_rx_stat *stat;
	uint8_t *buf;
	uint32_t rxdw2;
	int len;

	usbd_xfer_status(xfer, &len, NULL, NULL, NULL);

	if (__predict_false(len < sizeof(*stat))) {
		counter_u64_add(ic->ic_ierrors, 1);
		return (NULL);
	}

	buf = data->buf;
	stat = (struct r92c_rx_stat *)buf;
	rxdw2 = le32toh(stat->rxdw2);

	if (rxdw2 & R12A_RXDW2_RPT_C2H)
		urtwm_c2h_report(sc, (uint8_t *)&stat[1], len - sizeof(*stat));
	else
		return (urtwm_rxeof(sc, buf, len));

	return (NULL);
}

static void
urtwm_c2h_report(struct urtwm_softc *sc, uint8_t *buf, int len)
{

	if (len < 2) {
		device_printf(sc->sc_dev, "C2H report too short (len %d)\n",
		    len);
		return;
	}
	len -= 2;

	switch (buf[0]) {	/* command id */
	case R12A_C2H_TX_REPORT:
		urtwm_ratectl_tx_complete(sc, &buf[2], len);
		break;
	case R12A_C2H_IQK_FINISHED:
		URTWM_DPRINTF(sc, URTWM_DEBUG_CALIB,
		    "FW IQ calibration finished\n");
		sc->sc_flags &= ~URTWM_IQK_RUNNING;
		break;
	default:
		device_printf(sc->sc_dev,
		    "%s: C2H report %d was not handled\n",
		    __func__, buf[0]);
	}
}

static void
urtwm_ratectl_tx_complete(struct urtwm_softc *sc, void *buf, int len)
{
	struct r12a_c2h_tx_rpt *rpt = buf;
	struct ieee80211vap *vap;
	struct ieee80211_node *ni;
	int ntries;

	if (len != sizeof(*rpt)) {
		device_printf(sc->sc_dev,
		    "%s: wrong report size (%d, must be %d)\n",
		    __func__, len, sizeof(*rpt));
		return;
	}

	if (rpt->macid > URTWM_MACID_MAX(sc)) {
		device_printf(sc->sc_dev,
		    "macid %u is too big; increase MACID_MAX limit\n",
		    rpt->macid);
		return;
	}

	ntries = MS(rpt->txrptb2, R12A_TXRPTB2_RETRY_CNT);

	URTWM_NT_LOCK(sc);
	ni = sc->node_list[rpt->macid];
	if (ni != NULL) {
		vap = ni->ni_vap;
		URTWM_DPRINTF(sc, URTWM_DEBUG_INTR, "%s: frame for macid %d was"
		    "%s sent (%d retries)\n", __func__, rpt->macid,
		    (rpt->txrptb0 & (R12A_TXRPTB0_RETRY_OVER |
		    R12A_TXRPTB0_LIFE_EXPIRE)) ? " not" : "", ntries);

		if (rpt->txrptb0 & R12A_TXRPTB0_RETRY_OVER) {
			ieee80211_ratectl_tx_complete(vap, ni,
			    IEEE80211_RATECTL_TX_FAILURE, &ntries, NULL);
		} else {
			ieee80211_ratectl_tx_complete(vap, ni,
			    IEEE80211_RATECTL_TX_SUCCESS, &ntries, NULL);
		}
	} else {
		URTWM_DPRINTF(sc, URTWM_DEBUG_INTR,
		    "%s: macid %d, ni is NULL\n", __func__, rpt->macid);
	}
	URTWM_NT_UNLOCK(sc);

#ifdef IEEE80211_SUPPORT_SUPERG
	/* NB: this will be never executed when firmware is not loaded. */
	if (sc->sc_tx_n_active > 0)
		if (--sc->sc_tx_n_active <= 1)
			urtwm_cmd_sleepable(sc, NULL, 0, urtwm_ff_flush_all);
#endif
}

static struct mbuf *
urtwm_rxeof(struct urtwm_softc *sc, uint8_t *buf, int len)
{
	struct r92c_rx_stat *stat;
	struct mbuf *m, *m0 = NULL;
	uint32_t rxdw0;
	int totlen, pktlen, infosz;

	/* Process packets. */
	while (len >= sizeof(*stat)) {
		stat = (struct r92c_rx_stat *)buf;
		rxdw0 = le32toh(stat->rxdw0);

		pktlen = MS(rxdw0, R92C_RXDW0_PKTLEN);
		if (__predict_false(pktlen == 0))
			break;

		infosz = MS(rxdw0, R92C_RXDW0_INFOSZ) * 8;

		/* Make sure everything fits in xfer. */
		totlen = sizeof(*stat) + infosz + pktlen;
		if (totlen > len)
			break;

		if (m0 == NULL)
			m0 = m = urtwm_rx_copy_to_mbuf(sc, stat, totlen);
		else {
			m->m_next = urtwm_rx_copy_to_mbuf(sc, stat, totlen);
			if (m->m_next != NULL)
				m = m->m_next;
		}

		/* Next chunk is 8-byte aligned. */
		if (totlen < len)
			totlen = roundup2(totlen, 8);
		buf += totlen;
		len -= totlen;
	}

	return (m0);
}

static struct ieee80211_node *
urtwm_rx_frame(struct urtwm_softc *sc, struct mbuf *m, int8_t *rssi)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct ieee80211_frame_min *wh;
	struct urtwm_node *un;
	struct r92c_rx_stat *stat;
	uint32_t rxdw0, rxdw3;
	uint8_t rate, cipher;
	int infosz;

	stat = mtod(m, struct r92c_rx_stat *);
	rxdw0 = le32toh(stat->rxdw0);
	rxdw3 = le32toh(stat->rxdw3);

	rate = MS(rxdw3, R92C_RXDW3_RATE);
	cipher = MS(rxdw0, R92C_RXDW0_CIPHER);
	infosz = MS(rxdw0, R92C_RXDW0_INFOSZ) * 8;

	wh = (struct ieee80211_frame_min *)(mtod(m, uint8_t *) +
	    sizeof(*stat) + infosz);
	if ((wh->i_fc[1] & IEEE80211_FC1_PROTECTED) &&
	    cipher != R92C_CAM_ALGO_NONE)
		m->m_flags |= M_WEP;

	if (m->m_len >= sizeof(*wh))
		ni = ieee80211_find_rxnode(ic, wh);
	else
		ni = NULL;
	un = URTWM_NODE(ni);

	/* Get RSSI from PHY status descriptor if present. */
	if (infosz != 0 && (rxdw0 & R92C_RXDW0_PHYST)) {
		*rssi = urtwm_get_rssi(sc, rate, &stat[1]);
		URTWM_DPRINTF(sc, URTWM_DEBUG_RSSI, "%s: rssi=%d\n", __func__,
		    *rssi);

		sc->last_rssi = *rssi;
		if (un != NULL)
			un->last_rssi = *rssi;
	} else
		*rssi = (un != NULL) ? un->last_rssi : sc->last_rssi;

	if (ieee80211_radiotap_active(ic)) {
		struct urtwm_rx_radiotap_header *tap = &sc->sc_rxtap;

		tap->wr_flags = 0;
		if (le32toh(stat->rxdw4) & R92C_RXDW4_SGI)
			tap->wr_flags |= IEEE80211_RADIOTAP_F_SHORTGI;

		/* XXX TODO: multi-vap */
		tap->wr_tsft = urtwm_get_tsf_high(sc, 0);
		if (le32toh(stat->rxdw5) > urtwm_get_tsf_low(sc, 0))
			tap->wr_tsft--;
		tap->wr_tsft = (uint64_t)htole32(tap->wr_tsft) << 32;
		tap->wr_tsft += stat->rxdw5;

		/* XXX 20/40? */

		/* Map HW rate index to 802.11 rate. */
		if (rate < URTWM_RIDX_MCS(0))
			tap->wr_rate = ridx2rate[rate];
		else	/* MCS0~15. */
			tap->wr_rate = IEEE80211_RATE_MCS | (rate - 12);

		tap->wr_dbm_antsignal = *rssi;
		tap->wr_dbm_antnoise = URTWM_NOISE_FLOOR;
	}

	/* Drop descriptor. */
	m_adj(m, sizeof(*stat) + infosz);

	return (ni);
}

static void
urtwm_bulk_rx_callback(struct usb_xfer *xfer, usb_error_t error)
{
	struct urtwm_softc *sc = usbd_xfer_softc(xfer);
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct mbuf *m = NULL, *next;
	struct urtwm_data *data;
	int8_t nf, rssi;

	URTWM_ASSERT_LOCKED(sc);

	switch (USB_GET_STATE(xfer)) {
	case USB_ST_TRANSFERRED:
		data = STAILQ_FIRST(&sc->sc_rx_active);
		if (data == NULL)
			goto tr_setup;
		STAILQ_REMOVE_HEAD(&sc->sc_rx_active, next);
		m = urtwm_report_intr(sc, xfer, data);
		STAILQ_INSERT_TAIL(&sc->sc_rx_inactive, data, next);
		/* FALLTHROUGH */
	case USB_ST_SETUP:
tr_setup:
		data = STAILQ_FIRST(&sc->sc_rx_inactive);
		if (data == NULL) {
			KASSERT(m == NULL, ("mbuf isn't NULL"));
			goto finish;
		}
		STAILQ_REMOVE_HEAD(&sc->sc_rx_inactive, next);
		STAILQ_INSERT_TAIL(&sc->sc_rx_active, data, next);
		usbd_xfer_set_frame_data(xfer, 0, data->buf,
		    usbd_xfer_max_len(xfer));
		usbd_transfer_submit(xfer);

		/*
		 * To avoid LOR we should unlock our private mutex here to call
		 * ieee80211_input() because here is at the end of a USB
		 * callback and safe to unlock.
		 */
		while (m != NULL) {
			next = m->m_next;
			m->m_next = NULL;

			ni = urtwm_rx_frame(sc, m, &rssi);

			URTWM_UNLOCK(sc);

			nf = URTWM_NOISE_FLOOR;
			if (ni != NULL) {
				if (ni->ni_flags & IEEE80211_NODE_HT)
					m->m_flags |= M_AMPDU;
				(void)ieee80211_input(ni, m, rssi - nf, nf);
				ieee80211_free_node(ni);
			} else {
				(void)ieee80211_input_all(ic, m,
				    rssi - nf, nf);
			}
			URTWM_LOCK(sc);
			m = next;
		}
		break;
	default:
		/* needs it to the inactive queue due to a error. */
		data = STAILQ_FIRST(&sc->sc_rx_active);
		if (data != NULL) {
			STAILQ_REMOVE_HEAD(&sc->sc_rx_active, next);
			STAILQ_INSERT_TAIL(&sc->sc_rx_inactive, data, next);
		}
		if (error != USB_ERR_CANCELLED) {
			usbd_xfer_set_stall(xfer);
			counter_u64_add(ic->ic_ierrors, 1);
			goto tr_setup;
		}
		break;
	}
finish:
	/* Finished receive; age anything left on the FF queue by a little bump */
	/*
	 * XXX TODO: just make this a callout timer schedule so we can
	 * flush the FF staging queue if we're approaching idle.
	 */
#ifdef	IEEE80211_SUPPORT_SUPERG
	if (!(sc->sc_flags & URTWM_FW_LOADED)) {
		URTWM_UNLOCK(sc);
		ieee80211_ff_age_all(ic, 1);
		URTWM_LOCK(sc);
	}
#endif

	/* Kick-start more transmit in case we stalled */
	urtwm_start(sc);
}

static void
urtwm_txeof(struct urtwm_softc *sc, struct urtwm_data *data, int status)
{

	URTWM_ASSERT_LOCKED(sc);

	if (data->ni != NULL)	/* not a beacon frame */
		ieee80211_tx_complete(data->ni, data->m, status);

	if (!(sc->sc_flags & URTWM_FW_LOADED))
		if (sc->sc_tx_n_active > 0)
			sc->sc_tx_n_active--;

	data->ni = NULL;
	data->m = NULL;

	STAILQ_INSERT_TAIL(&sc->sc_tx_inactive, data, next);
}

static int
urtwm_alloc_list(struct urtwm_softc *sc, struct urtwm_data data[],
    int ndata, int maxsz)
{
	int i, error;

	for (i = 0; i < ndata; i++) {
		struct urtwm_data *dp = &data[i];
		dp->m = NULL;
		dp->buf = malloc(maxsz, M_USBDEV, M_NOWAIT);
		if (dp->buf == NULL) {
			device_printf(sc->sc_dev,
			    "could not allocate buffer\n");
			error = ENOMEM;
			goto fail;
		}
		dp->ni = NULL;
	}

	return (0);
fail:
	urtwm_free_list(sc, data, ndata);
	return (error);
}

static int
urtwm_alloc_rx_list(struct urtwm_softc *sc)
{
        int error, i;

	error = urtwm_alloc_list(sc, sc->sc_rx, URTWM_RX_LIST_COUNT,
	    URTWM_RXBUFSZ);
	if (error != 0)
		return (error);

	STAILQ_INIT(&sc->sc_rx_active);
	STAILQ_INIT(&sc->sc_rx_inactive);

	for (i = 0; i < URTWM_RX_LIST_COUNT; i++)
		STAILQ_INSERT_HEAD(&sc->sc_rx_inactive, &sc->sc_rx[i], next);

	return (0);
}

static int
urtwm_alloc_tx_list(struct urtwm_softc *sc)
{
	int error, i;

	error = urtwm_alloc_list(sc, sc->sc_tx, URTWM_TX_LIST_COUNT,
	    URTWM_TXBUFSZ);
	if (error != 0)
		return (error);

	STAILQ_INIT(&sc->sc_tx_active);
	STAILQ_INIT(&sc->sc_tx_inactive);
	STAILQ_INIT(&sc->sc_tx_pending);

	for (i = 0; i < URTWM_TX_LIST_COUNT; i++)
		STAILQ_INSERT_HEAD(&sc->sc_tx_inactive, &sc->sc_tx[i], next);

	return (0);
}

static void
urtwm_free_list(struct urtwm_softc *sc, struct urtwm_data data[], int ndata)
{
	int i;

	for (i = 0; i < ndata; i++) {
		struct urtwm_data *dp = &data[i];

		if (dp->buf != NULL) {
			free(dp->buf, M_USBDEV);
			dp->buf = NULL;
		}
		if (dp->ni != NULL) {
			ieee80211_free_node(dp->ni);
			dp->ni = NULL;
		}
		if (dp->m != NULL) {
			m_freem(dp->m);
			dp->m = NULL;
		}
	}
}

static void
urtwm_free_rx_list(struct urtwm_softc *sc)
{
	urtwm_free_list(sc, sc->sc_rx, URTWM_RX_LIST_COUNT);

	STAILQ_INIT(&sc->sc_rx_active);
	STAILQ_INIT(&sc->sc_rx_inactive);
}

static void
urtwm_free_tx_list(struct urtwm_softc *sc)
{
	urtwm_free_list(sc, sc->sc_tx, URTWM_TX_LIST_COUNT);

	STAILQ_INIT(&sc->sc_tx_active);
	STAILQ_INIT(&sc->sc_tx_inactive);
	STAILQ_INIT(&sc->sc_tx_pending);
}

static void
urtwm_bulk_tx_callback(struct usb_xfer *xfer, usb_error_t error)
{
	struct urtwm_softc *sc = usbd_xfer_softc(xfer);
	struct urtwm_data *data;

	URTWM_ASSERT_LOCKED(sc);

	switch (USB_GET_STATE(xfer)){
	case USB_ST_TRANSFERRED:
		data = STAILQ_FIRST(&sc->sc_tx_active);
		if (data == NULL)
			goto tr_setup;
		STAILQ_REMOVE_HEAD(&sc->sc_tx_active, next);
		urtwm_txeof(sc, data, 0);
		/* FALLTHROUGH */
	case USB_ST_SETUP:
tr_setup:
		data = STAILQ_FIRST(&sc->sc_tx_pending);
		if (data == NULL) {
			URTWM_DPRINTF(sc, URTWM_DEBUG_XMIT,
			    "%s: empty pending queue\n", __func__);
			sc->sc_tx_n_active = 0;
			goto finish;
		}
		STAILQ_REMOVE_HEAD(&sc->sc_tx_pending, next);
		STAILQ_INSERT_TAIL(&sc->sc_tx_active, data, next);
		usbd_xfer_set_frame_data(xfer, 0, data->buf, data->buflen);
		usbd_transfer_submit(xfer);
		if (!(sc->sc_flags & URTWM_FW_LOADED))
			sc->sc_tx_n_active++;
		break;
	default:
		data = STAILQ_FIRST(&sc->sc_tx_active);
		if (data == NULL)
			goto tr_setup;
		STAILQ_REMOVE_HEAD(&sc->sc_tx_active, next);
		urtwm_txeof(sc, data, 1);
		if (error != USB_ERR_CANCELLED) {
			usbd_xfer_set_stall(xfer);
			goto tr_setup;
		}
		break;
	}
finish:
#ifdef	IEEE80211_SUPPORT_SUPERG
	/*
	 * If the TX active queue drops below a certain
	 * threshold, ensure we age fast-frames out so they're
	 * transmitted.
	 */
	if (!(sc->sc_flags & URTWM_FW_LOADED) && sc->sc_tx_n_active <= 1) {
		/* XXX ew - net80211 should defer this for us! */

		/*
		 * Note: this sc_tx_n_active currently tracks
		 * the number of pending transmit submissions
		 * and not the actual depth of the TX frames
		 * pending to the hardware.  That means that
		 * we're going to end up with some sub-optimal
		 * aggregation behaviour.
		 */
		/*
		 * XXX TODO: just make this a callout timer schedule so we can
		 * flush the FF staging queue if we're approaching idle.
		 */
		urtwm_cmd_sleepable(sc, NULL, 0, urtwm_ff_flush_all);
	}
#endif
	/* Kick-start more transmit */
	urtwm_start(sc);
}

static struct urtwm_data *
_urtwm_getbuf(struct urtwm_softc *sc)
{
	struct urtwm_data *bf;

	bf = STAILQ_FIRST(&sc->sc_tx_inactive);
	if (bf != NULL)
		STAILQ_REMOVE_HEAD(&sc->sc_tx_inactive, next);
	else {
		URTWM_DPRINTF(sc, URTWM_DEBUG_XMIT,
		    "%s: out of xmit buffers\n", __func__);
	}
	return (bf);
}

static struct urtwm_data *
urtwm_getbuf(struct urtwm_softc *sc)
{
	struct urtwm_data *bf;

	URTWM_ASSERT_LOCKED(sc);

	bf = _urtwm_getbuf(sc);
	if (bf == NULL) {
		URTWM_DPRINTF(sc, URTWM_DEBUG_XMIT, "%s: stop queue\n",
		    __func__);
	}
	return (bf);
}

static usb_error_t
urtwm_write_region_1(struct urtwm_softc *sc, uint16_t addr, uint8_t *buf,
    int len)
{
	usb_device_request_t req;

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = R92C_REQ_REGS;
	USETW(req.wValue, addr);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);
	return (urtwm_do_request(sc, &req, buf));
}

static usb_error_t
urtwm_write_1(struct urtwm_softc *sc, uint16_t addr, uint8_t val)
{
	return (urtwm_write_region_1(sc, addr, &val, sizeof(val)));
}

static usb_error_t
urtwm_write_2(struct urtwm_softc *sc, uint16_t addr, uint16_t val)
{
	val = htole16(val);
	return (urtwm_write_region_1(sc, addr, (uint8_t *)&val, sizeof(val)));
}

static usb_error_t
urtwm_write_4(struct urtwm_softc *sc, uint16_t addr, uint32_t val)
{
	val = htole32(val);
	return (urtwm_write_region_1(sc, addr, (uint8_t *)&val, sizeof(val)));
}

static usb_error_t
urtwm_read_region_1(struct urtwm_softc *sc, uint16_t addr, uint8_t *buf,
    int len)
{
	usb_device_request_t req;

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = R92C_REQ_REGS;
	USETW(req.wValue, addr);
	USETW(req.wIndex, 0);
	USETW(req.wLength, len);
	return (urtwm_do_request(sc, &req, buf));
}

static uint8_t
urtwm_read_1(struct urtwm_softc *sc, uint16_t addr)
{
	uint8_t val;

	if (urtwm_read_region_1(sc, addr, &val, 1) != 0)
		return (0xff);
	return (val);
}

static uint16_t
urtwm_read_2(struct urtwm_softc *sc, uint16_t addr)
{
	uint16_t val;

	if (urtwm_read_region_1(sc, addr, (uint8_t *)&val, 2) != 0)
		return (0xffff);
	return (le16toh(val));
}

static uint32_t
urtwm_read_4(struct urtwm_softc *sc, uint16_t addr)
{
	uint32_t val;

	if (urtwm_read_region_1(sc, addr, (uint8_t *)&val, 4) != 0)
		return (0xffffffff);
	return (le32toh(val));
}

static usb_error_t
urtwm_setbits_1(struct urtwm_softc *sc, uint16_t addr, uint8_t clr,
    uint8_t set)
{
	return (urtwm_write_1(sc, addr,
	    (urtwm_read_1(sc, addr) & ~clr) | set));
}

static usb_error_t
urtwm_setbits_1_shift(struct urtwm_softc *sc, uint16_t addr, uint32_t clr,
    uint32_t set, int shift)
{
	return (urtwm_setbits_1(sc, addr + shift, clr >> shift * NBBY,
	    set >> shift * NBBY));
}

static usb_error_t
urtwm_setbits_2(struct urtwm_softc *sc, uint16_t addr, uint16_t clr,
    uint16_t set)
{
	return (urtwm_write_2(sc, addr,
	    (urtwm_read_2(sc, addr) & ~clr) | set));
}

static usb_error_t
urtwm_setbits_4(struct urtwm_softc *sc, uint16_t addr, uint32_t clr,
    uint32_t set)
{
	return (urtwm_write_4(sc, addr,
	    (urtwm_read_4(sc, addr) & ~clr) | set));
}

#ifndef URTWM_WITHOUT_UCODE
static int
urtwm_fw_cmd(struct urtwm_softc *sc, uint8_t id, const void *buf, int len)
{
	struct r88e_fw_cmd cmd;
	usb_error_t error;
	int ntries;

	if (!(sc->sc_flags & URTWM_FW_LOADED)) {
		URTWM_DPRINTF(sc, URTWM_DEBUG_FIRMWARE, "%s: firmware "
		    "was not loaded; command (id %d) will be discarded\n",
		    __func__, id);
		return (0);
	}

	/* Wait for current FW box to be empty. */
	for (ntries = 0; ntries < 100; ntries++) {
		if (!(urtwm_read_1(sc, R92C_HMETFR) & (1 << sc->fwcur)))
			break;
		urtwm_delay(sc, 2000);
	}
	if (ntries == 100) {
		device_printf(sc->sc_dev,
		    "could not send firmware command\n");
		return (ETIMEDOUT);
	}
	memset(&cmd, 0, sizeof(cmd));
	cmd.id = id;
	KASSERT(len <= sizeof(cmd.msg), ("urtwm_fw_cmd\n"));
	memcpy(cmd.msg, buf, len);

	/* Write the first word last since that will trigger the FW. */
	if (len > 3) {
		error = urtwm_write_4(sc, R88E_HMEBOX_EXT(sc->fwcur),
		    *(uint32_t *)((uint8_t *)&cmd + 4));
		if (error != USB_ERR_NORMAL_COMPLETION)
			return (EIO);
	}
	error = urtwm_write_4(sc, R92C_HMEBOX(sc->fwcur), *(uint32_t *)&cmd);
	if (error != USB_ERR_NORMAL_COMPLETION)
		return (EIO);

	sc->fwcur = (sc->fwcur + 1) % R92C_H2C_NBOX;

	return (0);
}
#endif	/* URTWM_WITHOUT_UCODE */

static void
urtwm_cmdq_cb(void *arg, int pending)
{
	struct urtwm_softc *sc = arg;
	struct urtwm_cmdq *item;

	/*
	 * Device must be powered on (via urtwm_power_on())
	 * before any command may be sent.
	 */
	URTWM_LOCK(sc);
	if (!(sc->sc_flags & URTWM_RUNNING)) {
		URTWM_UNLOCK(sc);
		return;
	}

	URTWM_CMDQ_LOCK(sc);
	while (sc->cmdq[sc->cmdq_first].func != NULL) {
		item = &sc->cmdq[sc->cmdq_first];
		sc->cmdq_first = (sc->cmdq_first + 1) % URTWM_CMDQ_SIZE;
		URTWM_CMDQ_UNLOCK(sc);

		item->func(sc, &item->data);

		URTWM_CMDQ_LOCK(sc);
		memset(item, 0, sizeof (*item));
	}
	URTWM_CMDQ_UNLOCK(sc);
	URTWM_UNLOCK(sc);
}

static int
urtwm_cmd_sleepable(struct urtwm_softc *sc, const void *ptr, size_t len,
    CMD_FUNC_PROTO)
{
	struct ieee80211com *ic = &sc->sc_ic;

	KASSERT(len <= sizeof(union sec_param), ("buffer overflow"));

	URTWM_CMDQ_LOCK(sc);
	if (sc->cmdq[sc->cmdq_last].func != NULL) {
		device_printf(sc->sc_dev, "%s: cmdq overflow\n", __func__);
		URTWM_CMDQ_UNLOCK(sc);

		return (EAGAIN);
	}

	if (ptr != NULL)
		memcpy(&sc->cmdq[sc->cmdq_last].data, ptr, len);
	sc->cmdq[sc->cmdq_last].func = func;
	sc->cmdq_last = (sc->cmdq_last + 1) % URTWM_CMDQ_SIZE;
	URTWM_CMDQ_UNLOCK(sc);

	ieee80211_runtask(ic, &sc->cmdq_task);

	return (0);
}

static void
urtwm_rf_write(struct urtwm_softc *sc, int chain, uint8_t addr,
    uint32_t val)
{
	urtwm_bb_write(sc, R12A_LSSI_PARAM(chain),
	    SM(R88E_LSSI_PARAM_ADDR, addr) |
	    SM(R92C_LSSI_PARAM_DATA, val));
}

static uint32_t
urtwm_r12a_rf_read(struct urtwm_softc *sc, int chain, uint8_t addr)
{
	uint32_t pi_mode, val;

	/* Turn off CCA (avoids reading the wrong value). */
	if (addr != R92C_RF_AC)
		urtwm_bb_setbits(sc, R12A_CCA_ON_SEC, 0, 0x08);

	val = urtwm_bb_read(sc, R12A_HSSI_PARAM1(chain));
	pi_mode = (val & R12A_HSSI_PARAM1_PI) ? 1 : 0;

	urtwm_bb_setbits(sc, R12A_HSSI_PARAM2,
	    R12A_HSSI_PARAM2_READ_ADDR_MASK, addr);

	val = urtwm_bb_read(sc, pi_mode ? R12A_HSPI_READBACK(chain) :
	    R12A_LSSI_READBACK(chain));

	/* Turn on CCA (when exiting). */
	if (addr != R92C_RF_AC)
		urtwm_bb_setbits(sc, R12A_CCA_ON_SEC, 0x08, 0);

	return (MS(val, R92C_LSSI_READBACK_DATA));
}

static uint32_t
urtwm_r21a_rf_read(struct urtwm_softc *sc, int chain, uint8_t addr)
{
	uint32_t pi_mode, val;

	val = urtwm_bb_read(sc, R12A_HSSI_PARAM1(chain));
	pi_mode = (val & R12A_HSSI_PARAM1_PI) ? 1 : 0;

	urtwm_bb_setbits(sc, R12A_HSSI_PARAM2,
	    R12A_HSSI_PARAM2_READ_ADDR_MASK, addr);
	urtwm_delay(sc, 20);

	val = urtwm_bb_read(sc, pi_mode ? R12A_HSPI_READBACK(chain) :
	    R12A_LSSI_READBACK(chain));

	return (MS(val, R92C_LSSI_READBACK_DATA));
}

static void
urtwm_rf_setbits(struct urtwm_softc *sc, int chain, uint8_t addr,
    uint32_t clr, uint32_t set)
{
	urtwm_rf_write(sc, chain, addr,
	    (urtwm_rf_read(sc, chain, addr) & ~clr) | set);
}

static int
urtwm_llt_write(struct urtwm_softc *sc, uint32_t addr, uint32_t data)
{
	usb_error_t error;
	int ntries;

	error = urtwm_write_4(sc, R92C_LLT_INIT,
	    SM(R92C_LLT_INIT_OP, R92C_LLT_INIT_OP_WRITE) |
	    SM(R92C_LLT_INIT_ADDR, addr) |
	    SM(R92C_LLT_INIT_DATA, data));
	if (error != USB_ERR_NORMAL_COMPLETION)
		return (EIO);
	/* Wait for write operation to complete. */
	for (ntries = 0; ntries < 20; ntries++) {
		if (MS(urtwm_read_4(sc, R92C_LLT_INIT), R92C_LLT_INIT_OP) ==
		    R92C_LLT_INIT_OP_NO_ACTIVE)
			return (0);
		urtwm_delay(sc, 10);
	}
	return (ETIMEDOUT);
}

static int
urtwm_efuse_read_next(struct urtwm_softc *sc, uint8_t *val)
{
	uint32_t reg;
	usb_error_t error;
	int ntries;

	if (sc->next_rom_addr >= URTWM_EFUSE_MAX_LEN)
		return (EFAULT);

	reg = urtwm_read_4(sc, R92C_EFUSE_CTRL);
	reg = RW(reg, R92C_EFUSE_CTRL_ADDR, sc->next_rom_addr);
	reg &= ~R92C_EFUSE_CTRL_VALID;

	error = urtwm_write_4(sc, R92C_EFUSE_CTRL, reg);
	if (error != USB_ERR_NORMAL_COMPLETION)
		return (EIO);
	/* Wait for read operation to complete. */
	for (ntries = 0; ntries < 100; ntries++) {
		reg = urtwm_read_4(sc, R92C_EFUSE_CTRL);
		if (reg & R92C_EFUSE_CTRL_VALID)
			break;
		urtwm_delay(sc, 1000);	/* XXX */
	}
	if (ntries == 100) {
		device_printf(sc->sc_dev,
		    "could not read efuse byte at address 0x%x\n",
		    sc->next_rom_addr);
		return (ETIMEDOUT);
	}

	*val = MS(reg, R92C_EFUSE_CTRL_DATA);
	sc->next_rom_addr++;

	return (0);
}

static int
urtwm_efuse_read_data(struct urtwm_softc *sc, uint8_t *rom, uint8_t off,
    uint8_t msk)
{
	uint8_t reg;
	int i, error;

	for (i = 0; i < 4; i++) {
		if (msk & (1 << i))
			continue;
		error = urtwm_efuse_read_next(sc, &reg);
		if (error != 0)
			return (error);
		URTWM_DPRINTF(sc, URTWM_DEBUG_ROM, "rom[0x%03X] == 0x%02X\n",
		    off * 8 + i * 2, reg);
		rom[off * 8 + i * 2 + 0] = reg;

		error = urtwm_efuse_read_next(sc, &reg);
		if (error != 0)
			return (error);
		URTWM_DPRINTF(sc, URTWM_DEBUG_ROM, "rom[0x%03X] == 0x%02X\n",
		    off * 8 + i * 2 + 1, reg);
		rom[off * 8 + i * 2 + 1] = reg;
	}

	return (0);
}

#ifdef USB_DEBUG
static void
urtwm_dump_rom_contents(struct urtwm_softc *sc, uint8_t *rom, uint16_t size)
{
	int i;

	/* Dump ROM contents. */
	device_printf(sc->sc_dev, "%s:", __func__);
	for (i = 0; i < size; i++) {
		if (i % 32 == 0)
			printf("\n%03X: ", i);
		else if (i % 4 == 0)
			printf(" ");

		printf("%02X", rom[i]);
	}
	printf("\n");
}
#endif

static int
urtwm_efuse_read(struct urtwm_softc *sc, uint8_t *rom, uint16_t size)
{
#define URTWM_CHK(res) do {	\
	if ((error = res) != 0)	\
		goto end;	\
} while(0)
	uint8_t msk, off, reg;
	int error;

	URTWM_CHK(urtwm_efuse_switch_power(sc));

	/* Read full ROM image. */
	sc->next_rom_addr = 0;
	memset(rom, 0xff, size);

	URTWM_CHK(urtwm_efuse_read_next(sc, &reg));
	while (reg != 0xff) {
		/* check for extended header */
		if ((reg & 0x1f) == 0x0f) {
			off = reg >> 5;
			URTWM_CHK(urtwm_efuse_read_next(sc, &reg));

			if ((reg & 0x0f) != 0x0f)
				off = ((reg & 0xf0) >> 1) | off;
			else
				continue;
		} else
			off = reg >> 4;
		msk = reg & 0xf;

		URTWM_CHK(urtwm_efuse_read_data(sc, rom, off, msk));
		URTWM_CHK(urtwm_efuse_read_next(sc, &reg));
	}

end:

#ifdef USB_DEBUG
	if (sc->sc_debug & URTWM_DEBUG_ROM)
		urtwm_dump_rom_contents(sc, rom, size);
#endif

	urtwm_write_1(sc, R92C_EFUSE_ACCESS, R92C_EFUSE_ACCESS_OFF);

	if (error != 0) {
		device_printf(sc->sc_dev, "%s: error while reading ROM\n",
		    __func__);
	}

	return (error);
#undef URTWM_CHK
}

static int
urtwm_efuse_switch_power(struct urtwm_softc *sc)
{
	usb_error_t error;
	uint32_t reg;

	error = urtwm_write_1(sc, R92C_EFUSE_ACCESS, R92C_EFUSE_ACCESS_ON);
	if (error != USB_ERR_NORMAL_COMPLETION)
		return (EIO);

	reg = urtwm_read_2(sc, R92C_SYS_FUNC_EN);
	if (!(reg & R92C_SYS_FUNC_EN_ELDR)) {
		error = urtwm_write_2(sc, R92C_SYS_FUNC_EN,
		    reg | R92C_SYS_FUNC_EN_ELDR);
		if (error != USB_ERR_NORMAL_COMPLETION)
			return (EIO);
	}
	reg = urtwm_read_2(sc, R92C_SYS_CLKR);
	if ((reg & (R92C_SYS_CLKR_LOADER_EN | R92C_SYS_CLKR_ANA8M)) !=
	    (R92C_SYS_CLKR_LOADER_EN | R92C_SYS_CLKR_ANA8M)) {
		error = urtwm_write_2(sc, R92C_SYS_CLKR,
		    reg | R92C_SYS_CLKR_LOADER_EN | R92C_SYS_CLKR_ANA8M);
		if (error != USB_ERR_NORMAL_COMPLETION)
			return (EIO);
	}

	return (0);
}

static int
urtwm_setup_endpoints(struct urtwm_softc *sc)
{
	struct usb_endpoint *ep, *ep_end;
	uint8_t addr[R12A_MAX_EPOUT];
	int error;

	/* Determine the number of bulk-out pipes. */
	sc->ntx = 0;
	sc->sc_iface_index = URTWM_IFACE_INDEX;
	ep = sc->sc_udev->endpoints;
	ep_end = sc->sc_udev->endpoints + sc->sc_udev->endpoints_max;
	for (; ep != ep_end; ep++) {
		uint8_t eaddr;

		if ((ep->edesc == NULL) ||
		    (ep->iface_index != sc->sc_iface_index))
			continue;

		eaddr = ep->edesc->bEndpointAddress;
		URTWM_DPRINTF(sc, URTWM_DEBUG_USB,
		    "%s: endpoint: addr %d, direction %s\n", __func__,
		    UE_GET_ADDR(eaddr), UE_GET_DIR(eaddr) == UE_DIR_OUT ?
		    "output" : "input");

		if (UE_GET_DIR(eaddr) == UE_DIR_OUT) {
			if (sc->ntx == R12A_MAX_EPOUT)
				break;

			addr[sc->ntx++] = UE_GET_ADDR(eaddr);
		}
	}
	if (sc->ntx == 0 || sc->ntx > R12A_MAX_EPOUT) {
		device_printf(sc->sc_dev,
		    "%s: invalid number of Tx bulk pipes (%d)\n", __func__,
		    sc->ntx);
		return (EINVAL);
	}

	/* NB: keep in sync with urtwm_dma_init(). */
	urtwm_config[URTWM_BULK_TX_VO].endpoint = addr[0];
	switch (sc->ntx) {
	case 4:
	case 3:
		urtwm_config[URTWM_BULK_TX_BE].endpoint = addr[2];
		urtwm_config[URTWM_BULK_TX_BK].endpoint = addr[2];
		urtwm_config[URTWM_BULK_TX_VI].endpoint = addr[1];
		break;
	case 2:
		urtwm_config[URTWM_BULK_TX_BE].endpoint = addr[1];
		urtwm_config[URTWM_BULK_TX_BK].endpoint = addr[1];
		urtwm_config[URTWM_BULK_TX_VI].endpoint = addr[0];
		break;
	case 1:
		urtwm_config[URTWM_BULK_TX_BE].endpoint = addr[0];
		urtwm_config[URTWM_BULK_TX_BK].endpoint = addr[0];
		urtwm_config[URTWM_BULK_TX_VI].endpoint = addr[0];
		break;
	default:
		KASSERT(1, ("unhandled number of endpoints %d\n", sc->ntx));
		break;
	}

	error = usbd_transfer_setup(sc->sc_udev, &sc->sc_iface_index,
	    sc->sc_xfer, urtwm_config, URTWM_N_TRANSFER, sc, &sc->sc_mtx);
	if (error) {
		device_printf(sc->sc_dev, "could not allocate USB transfers, "
		    "err=%s\n", usbd_errstr(error));
		return (error);
	}

	return (0);
}

static int
urtwm_read_chipid(struct urtwm_softc *sc)
{
	uint32_t reg;

	reg = urtwm_read_4(sc, R92C_SYS_CFG);
	if (reg & R92C_SYS_CFG_TRP_VAUX_EN)	/* test chip */
		return (EIO);

	if (URTWM_CHIP_IS_12A(sc)) {
		if (MS(reg, R92C_SYS_CFG_CHIP_VER_RTL) == 1)
			sc->chip |= URTWM_CHIP_12A_C_CUT;
	}

	return (0);
}

static int
urtwm_r12a_check_condition(struct urtwm_softc *sc, const uint8_t cond[])
{
	uint8_t mask[4];
	int i, j, nmasks;

	URTWM_DPRINTF(sc, URTWM_DEBUG_RESET,
	    "%s: condition byte 0: %02X; ext PA/LNA: %d/%d (2 GHz), "
	    "%d/%d (5 GHz)\n", __func__, cond[0], sc->ext_pa_2g,
	    sc->ext_lna_2g, sc->ext_pa_5g, sc->ext_lna_5g);

	if (cond[0] == 0)
		return (1);

	if (!sc->ext_pa_2g && !sc->ext_lna_2g &&
	    !sc->ext_pa_5g && !sc->ext_lna_5g)
		return (0);

	nmasks = 0;
	if (sc->ext_pa_2g) {
		mask[nmasks] = R12A_COND_GPA;
		mask[nmasks] |= R12A_COND_TYPE(sc->type_pa_2g);
		nmasks++;
	}
	if (sc->ext_pa_5g) {
		mask[nmasks] = R12A_COND_APA;
		mask[nmasks] |= R12A_COND_TYPE(sc->type_pa_5g);
		nmasks++;
	}
	if (sc->ext_lna_2g) {
		mask[nmasks] = R12A_COND_GLNA;
		mask[nmasks] |= R12A_COND_TYPE(sc->type_lna_2g);
		nmasks++;
	}
	if (sc->ext_lna_5g) {
		mask[nmasks] = R12A_COND_ALNA;
		mask[nmasks] |= R12A_COND_TYPE(sc->type_lna_5g);
		nmasks++;
	}

	for (i = 0; i < URTWM_MAX_CONDITIONS && cond[i] != 0; i++)
		for (j = 0; j < nmasks; j++)
			if ((cond[i] & mask[j]) == mask[j])
				return (1);

	return (0);
}

static int
urtwm_r21a_check_condition(struct urtwm_softc *sc, const uint8_t cond[])
{
	uint8_t mask;
	int i;

	URTWM_DPRINTF(sc, URTWM_DEBUG_RESET,
	    "%s: condition byte 0: %02X; ext 5ghz pa/lna %d/%d\n",
	    __func__, cond[0], sc->ext_pa_5g, sc->ext_lna_5g);

	if (cond[0] == 0)
		return (1);

	mask = 0;
	if (sc->ext_pa_5g)
		mask |= R21A_COND_EXT_PA_5G;
	if (sc->ext_lna_5g)
		mask |= R21A_COND_EXT_LNA_5G;
	if (sc->bt_coex)
		mask |= R21A_COND_BT;
	if (!sc->ext_pa_2g && !sc->ext_lna_2g &&
	    !sc->ext_pa_5g && !sc->ext_lna_5g && !sc->bt_coex)
		mask = R21A_COND_BOARD_DEF;

	if (mask == 0)
		return (0);

	for (i = 0; i < URTWM_MAX_CONDITIONS && cond[i] != 0; i++)
		if (cond[i] == mask)
			return (1);

	return (0);
}

static void
urtwm_config_specific(struct urtwm_softc *sc)
{

	if (URTWM_CHIP_IS_12A(sc)) {
		if (sc->chip & URTWM_CHIP_12A_C_CUT)
			sc->sc_rf_read = urtwm_r21a_rf_read;
		else
			sc->sc_rf_read = urtwm_r12a_rf_read;
		sc->sc_check_condition = urtwm_r12a_check_condition;
		sc->sc_parse_rom = urtwm_r12a_parse_rom;
		sc->sc_get_rssi_cck = urtwm_r12a_get_rssi_cck;
		sc->sc_power_on = urtwm_r12a_power_on;
		sc->sc_power_off = urtwm_r12a_power_off;
#ifndef URTWM_WITHOUT_UCODE
		sc->sc_fw_reset = urtwm_r12a_fw_reset;
#endif
		sc->sc_set_page_size = urtwm_r12a_set_page_size;
		sc->sc_crystalcap_write = urtwm_r12a_crystalcap_write;
		sc->sc_set_band_2ghz = urtwm_r12a_set_band_2ghz;
		sc->sc_set_band_5ghz = urtwm_r12a_set_band_5ghz;

		sc->mac_prog = &rtl8812au_mac[0];
		sc->mac_size = nitems(rtl8812au_mac);
		sc->bb_prog = &rtl8812au_bb[0];
		sc->bb_size = nitems(rtl8812au_bb);
		sc->agc_prog = &rtl8812au_agc[0];
		sc->agc_size = nitems(rtl8812au_agc);
		sc->rf_prog = &rtl8812au_rf[0];

		sc->fwname = "urtwm-rtl8812aufw";
		sc->fwsig = 0x950;

		sc->page_count = R12A_TX_PAGE_COUNT;
		sc->pktbuf_count = R12A_TXPKTBUF_COUNT;
		sc->tx_boundary = R12A_TX_PAGE_BOUNDARY;
		sc->tx_agg_desc_num = 1;
		sc->npubqpages = R12A_PUBQ_NPAGES;
		sc->page_size = R12A_TX_PAGE_SIZE;
		sc->rx_dma_size = R12A_RX_DMA_BUFFER_SIZE;

		sc->ntxchains = 2;
		sc->nrxchains = 2;
	} else {
		sc->sc_rf_read = urtwm_r21a_rf_read;
		sc->sc_check_condition = urtwm_r21a_check_condition;
		sc->sc_parse_rom = urtwm_r21a_parse_rom;
		sc->sc_get_rssi_cck = urtwm_r21a_get_rssi_cck;
		sc->sc_power_on = urtwm_r21a_power_on;
		sc->sc_power_off = urtwm_r21a_power_off;
#ifndef URTWM_WITHOUT_UCODE
		sc->sc_fw_reset = urtwm_r21a_fw_reset;
#endif
		sc->sc_set_page_size = urtwm_r21a_set_page_size;
		sc->sc_crystalcap_write = urtwm_r21a_crystalcap_write;
		sc->sc_set_band_2ghz = urtwm_r21a_set_band_2ghz;
		sc->sc_set_band_5ghz = urtwm_r21a_set_band_5ghz;

		sc->mac_prog = &rtl8821au_mac[0];
		sc->mac_size = nitems(rtl8821au_mac);
		sc->bb_prog = &rtl8821au_bb[0];
		sc->bb_size = nitems(rtl8821au_bb);
		sc->agc_prog = &rtl8821au_agc[0];
		sc->agc_size = nitems(rtl8821au_agc);
		sc->rf_prog = &rtl8821au_rf[0];

		sc->fwname = "urtwm-rtl8821aufw";
		sc->fwsig = 0x210;

		sc->page_count = R21A_TX_PAGE_COUNT;
		sc->pktbuf_count = R12A_TXPKTBUF_COUNT;
		sc->tx_boundary = R21A_TX_PAGE_BOUNDARY;
		sc->tx_agg_desc_num = 6;
		sc->npubqpages = R12A_PUBQ_NPAGES;
		sc->page_size = R21A_TX_PAGE_SIZE;
		sc->rx_dma_size = R12A_RX_DMA_BUFFER_SIZE;

		sc->ntxchains = 1;
		sc->nrxchains = 1;
	}

	if (usbd_get_speed(sc->sc_udev) == USB_SPEED_SUPER) {
		sc->ac_usb_dma_size = 0x07;
		sc->ac_usb_dma_time = 0x1a;
	} else {
		sc->ac_usb_dma_size = 0x01;
		sc->ac_usb_dma_time = 0x10;
	}
}

static void
urtwm_config_specific_rom(struct urtwm_softc *sc)
{
	if (URTWM_CHIP_IS_12A(sc)) {
		if (sc->board_type == R92C_BOARD_TYPE_MINICARD ||
		    sc->board_type == R92C_BOARD_TYPE_SOLO ||
		    sc->board_type == R92C_BOARD_TYPE_COMBO)
			sc->sc_set_led = urtwm_r12a_set_led_mini;
		else
			sc->sc_set_led = urtwm_r12a_set_led;

		if (!(sc->ext_pa_2g || sc->ext_lna_2g ||
		    sc->ext_pa_5g || sc->ext_lna_5g))
			sc->mac_prog = &rtl8812au_mac_no_ext_pa_lna[0];
	} else {
		if (sc->board_type == R92C_BOARD_TYPE_MINICARD ||
		    sc->board_type == R92C_BOARD_TYPE_SOLO ||
		    sc->board_type == R92C_BOARD_TYPE_COMBO)
			sc->sc_set_led = urtwm_r12a_set_led_mini;
		else
			sc->sc_set_led = urtwm_r21a_set_led;
	}
}

static int
urtwm_read_rom(struct urtwm_softc *sc)
{
	struct r12a_rom *rom;
	int error;

	rom = malloc(URTWM_EFUSE_MAX_LEN, M_TEMP, M_WAITOK);

	/* Read full ROM image. */
	URTWM_LOCK(sc);
	error = urtwm_efuse_read(sc, (uint8_t *)rom, sizeof(*rom));
	URTWM_UNLOCK(sc);
	if (error != 0)
		goto fail;

	/* Parse & save data in softc. */
	urtwm_parse_rom(sc, rom);

fail:
	free(rom, M_TEMP);

	return (error);
}

static void
urtwm_r12a_parse_rom(struct urtwm_softc *sc, struct r12a_rom *rom)
{
#define URTWM_GET_ROM_VAR(var, def)	(((var) != 0xff) ? (var) : (def))
#define URTWM_SIGN4TO8(val)		(((val) & 0x08) ? (val) | 0xf0 : (val))
	uint8_t pa_type, lna_type_2g, lna_type_5g;

	/* Read PA/LNA types. */
	pa_type = URTWM_GET_ROM_VAR(rom->pa_type, 0);
	lna_type_2g = URTWM_GET_ROM_VAR(rom->lna_type_2g, 0);
	lna_type_5g = URTWM_GET_ROM_VAR(rom->lna_type_5g, 0);

	sc->ext_pa_2g = R12A_ROM_IS_PA_EXT_2GHZ(pa_type);
	sc->ext_pa_5g = R12A_ROM_IS_PA_EXT_5GHZ(pa_type);
	sc->ext_lna_2g = R21A_ROM_IS_LNA_EXT(lna_type_2g);
	sc->ext_lna_5g = R21A_ROM_IS_LNA_EXT(lna_type_5g);
	sc->bt_coex = (MS(rom->rf_board_opt, R92C_ROM_RF1_BOARD_TYPE) ==
	    R92C_BOARD_TYPE_HIGHPA);
	sc->bt_ant_num = (rom->rf_bt_opt & R12A_RF_BT_OPT_ANT_NUM);

	if (sc->ext_pa_2g) {
		sc->type_pa_2g =
		    R12A_GET_ROM_PA_TYPE(lna_type_2g, 0) |
		    (R12A_GET_ROM_PA_TYPE(lna_type_2g, 1) << 2);
	}
	if (sc->ext_pa_5g) {
		sc->type_pa_5g =
		    R12A_GET_ROM_PA_TYPE(lna_type_5g, 0) |
		    (R12A_GET_ROM_PA_TYPE(lna_type_5g, 1) << 2);
	}
	if (sc->ext_lna_2g) {
		sc->type_lna_2g =
		    R12A_GET_ROM_LNA_TYPE(lna_type_2g, 0) |
		    (R12A_GET_ROM_LNA_TYPE(lna_type_2g, 1) << 2);
	}
	if (sc->ext_lna_5g) {
		sc->type_lna_5g =
		    R12A_GET_ROM_LNA_TYPE(lna_type_5g, 0) |
		    (R12A_GET_ROM_LNA_TYPE(lna_type_5g, 1) << 2);
	}

	if (rom->rfe_option & 0x80) {
		if (sc->ext_lna_5g) {
			if (sc->ext_pa_5g) {
				if (sc->ext_pa_2g && sc->ext_lna_2g)
					sc->rfe_type = 3;
				else
					sc->rfe_type = 0;
			} else
				sc->rfe_type = 2;
		} else
			sc->rfe_type = 4;
	} else {
		sc->rfe_type = rom->rfe_option & 0x3f;

		/* workaround for incorrect EFUSE map */
		if (sc->rfe_type == 4 &&
		    sc->ext_pa_2g && sc->ext_lna_2g &&
		    sc->ext_pa_5g && sc->ext_lna_5g)
			sc->rfe_type = 0;
	}

	/* Read MAC address. */
	IEEE80211_ADDR_COPY(sc->sc_ic.ic_macaddr, rom->macaddr_12a);
}

static void
urtwm_r21a_parse_rom(struct urtwm_softc *sc, struct r12a_rom *rom)
{
	uint8_t pa_type, lna_type_2g, lna_type_5g;

	/* Read PA/LNA types. */
	pa_type = URTWM_GET_ROM_VAR(rom->pa_type, 0);
	lna_type_2g = URTWM_GET_ROM_VAR(rom->lna_type_2g, 0);
	lna_type_5g = URTWM_GET_ROM_VAR(rom->lna_type_5g, 0);

	sc->ext_pa_2g = R21A_ROM_IS_PA_EXT_2GHZ(pa_type);
	sc->ext_pa_5g = R21A_ROM_IS_PA_EXT_5GHZ(pa_type);
	sc->ext_lna_2g = R21A_ROM_IS_LNA_EXT(lna_type_2g);
	sc->ext_lna_5g = R21A_ROM_IS_LNA_EXT(lna_type_5g);

	URTWM_LOCK(sc);
	sc->bt_coex =
	    !!(urtwm_read_4(sc, R92C_MULTI_FUNC_CTRL) & R92C_MULTI_BT_FUNC_EN);
	URTWM_UNLOCK(sc);
	sc->bt_ant_num = (rom->rf_bt_opt & R12A_RF_BT_OPT_ANT_NUM);

	/* Read MAC address. */
	IEEE80211_ADDR_COPY(sc->sc_ic.ic_macaddr, rom->macaddr_21a);
}

static void
urtwm_parse_rom(struct urtwm_softc *sc, struct r12a_rom *rom)
{
	int i, j;

	sc->crystalcap = URTWM_GET_ROM_VAR(rom->crystalcap,
	    R12A_ROM_CRYSTALCAP_DEF);
	sc->thermal_meter = rom->thermal_meter;
	sc->tx_bbswing_2g = URTWM_GET_ROM_VAR(rom->tx_bbswing_2g, 0);
	sc->tx_bbswing_5g = URTWM_GET_ROM_VAR(rom->tx_bbswing_5g, 0);

	for (i = 0; i < sc->ntxchains; i++) {
		struct r12a_tx_pwr_2g *pwr_2g = &rom->tx_pwr[i].pwr_2g;
		struct r12a_tx_pwr_5g *pwr_5g = &rom->tx_pwr[i].pwr_5g;
		struct r12a_tx_pwr_diff_2g *pwr_diff_2g =
		    &rom->tx_pwr[i].pwr_diff_2g;
		struct r12a_tx_pwr_diff_5g *pwr_diff_5g =
		    &rom->tx_pwr[i].pwr_diff_5g;

		for (j = 0; j < URTWM_MAX_GROUP_2G - 1; j++) {
			sc->cck_tx_pwr[i][j] =
			    URTWM_GET_ROM_VAR(pwr_2g->cck[j],
				URTWM_DEF_TX_PWR_2G);
			sc->ht40_tx_pwr_2g[i][j] =
			    URTWM_GET_ROM_VAR(pwr_2g->ht40[j],
				URTWM_DEF_TX_PWR_2G);
		}
		sc->cck_tx_pwr[i][j] = URTWM_GET_ROM_VAR(pwr_2g->cck[j],
		    URTWM_DEF_TX_PWR_2G);

		sc->cck_tx_pwr_diff_2g[i][0] = 0;
		sc->ofdm_tx_pwr_diff_2g[i][0] = URTWM_SIGN4TO8(
		    MS(pwr_diff_2g->ht20_ofdm, LOW_PART));
		sc->bw20_tx_pwr_diff_2g[i][0] = URTWM_SIGN4TO8(
		    MS(pwr_diff_2g->ht20_ofdm, HIGH_PART));
		sc->bw40_tx_pwr_diff_2g[i][0] = 0;

		for (j = 1; j < nitems(pwr_diff_2g->diff123); j++) {
			sc->cck_tx_pwr_diff_2g[i][j] = URTWM_SIGN4TO8(
			    MS(pwr_diff_2g->diff123[j].ofdm_cck, LOW_PART));
			sc->ofdm_tx_pwr_diff_2g[i][j] = URTWM_SIGN4TO8(
			    MS(pwr_diff_2g->diff123[j].ofdm_cck, HIGH_PART));
			sc->bw20_tx_pwr_diff_2g[i][j] = URTWM_SIGN4TO8(
			    MS(pwr_diff_2g->diff123[j].ht40_ht20, LOW_PART));
			sc->bw40_tx_pwr_diff_2g[i][j] = URTWM_SIGN4TO8(
			    MS(pwr_diff_2g->diff123[j].ht40_ht20, HIGH_PART));
		}

		for (j = 0; j < URTWM_MAX_GROUP_5G; j++) {
			sc->ht40_tx_pwr_5g[i][j] =
			    URTWM_GET_ROM_VAR(pwr_5g->ht40[j],
				URTWM_DEF_TX_PWR_5G);
		}

		sc->ofdm_tx_pwr_diff_5g[i][0] = URTWM_SIGN4TO8(
		    MS(pwr_diff_5g->ht20_ofdm, LOW_PART));
		sc->ofdm_tx_pwr_diff_5g[i][1] = URTWM_SIGN4TO8(
		    MS(pwr_diff_5g->ofdm_ofdm[0], HIGH_PART));
		sc->ofdm_tx_pwr_diff_5g[i][2] = URTWM_SIGN4TO8(
		    MS(pwr_diff_5g->ofdm_ofdm[0], LOW_PART));
		sc->ofdm_tx_pwr_diff_5g[i][3] = URTWM_SIGN4TO8(
		    MS(pwr_diff_5g->ofdm_ofdm[1], LOW_PART));

		sc->bw20_tx_pwr_diff_5g[i][0] = URTWM_SIGN4TO8(
		    MS(pwr_diff_5g->ht20_ofdm, HIGH_PART));
		sc->bw40_tx_pwr_diff_5g[i][0] = 0;
		for (j = 1; j < nitems(pwr_diff_5g->ht40_ht20); j++) {
			sc->bw20_tx_pwr_diff_5g[i][j] = URTWM_SIGN4TO8(
			    MS(pwr_diff_5g->ht40_ht20[j], LOW_PART));
			sc->bw40_tx_pwr_diff_5g[i][j] = URTWM_SIGN4TO8(
			    MS(pwr_diff_5g->ht40_ht20[j], HIGH_PART));
		}

		for (j = 0; j < nitems(pwr_diff_5g->ht80_ht160); j++) {
			sc->bw80_tx_pwr_diff_5g[i][j] = URTWM_SIGN4TO8(
			    MS(pwr_diff_5g->ht80_ht160[j], HIGH_PART));
			sc->bw160_tx_pwr_diff_5g[i][j] = URTWM_SIGN4TO8(
			    MS(pwr_diff_5g->ht80_ht160[j], LOW_PART));
		}
	}

	sc->regulatory = MS(rom->rf_board_opt, R92C_ROM_RF1_REGULATORY);
	sc->board_type = MS(URTWM_GET_ROM_VAR(rom->rf_board_opt, 0),
	    R92C_ROM_RF1_BOARD_TYPE);
	URTWM_DPRINTF(sc, URTWM_DEBUG_ROM, "%s: regulatory type=%d\n",
	    __func__, sc->regulatory);

	/* Read device-specific parameters. */
	urtwm_parse_rom_specific(sc, rom);
#undef URTWM_SIGN4TO8
#undef URTWM_GET_ROM_VAR
}

static __inline uint8_t
rate2ridx(uint8_t rate)
{
	if (rate & IEEE80211_RATE_MCS) {
		/* 11n rates start at idx 12 */
		return ((rate & 0xf) + 12);
	}
	switch (rate) {
	/* 11g */
	case 12:	return 4;
	case 18:	return 5;
	case 24:	return 6;
	case 36:	return 7;
	case 48:	return 8;
	case 72:	return 9;
	case 96:	return 10;
	case 108:	return 11;
	/* 11b */
	case 2:		return 0;
	case 4:		return 1;
	case 11:	return 2;
	case 22:	return 3;
	default:	return URTWM_RIDX_UNKNOWN;
	}
}

#ifdef URTWM_TODO
/* XXX TODO: provide a sysctl to switch between f/w and net80211 ratectl. */
/*
 * Initialize firmware rate adaptation.
 */
static int
urtwn_ra_init(struct urtwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);
	struct ieee80211_node *ni;
	struct ieee80211_rateset *rs, *rs_ht;
	struct r92c_fw_cmd_macid_cfg cmd;
	uint32_t rates, basicrates;
	uint8_t mode, ridx;
	int maxrate, maxbasicrate, error = 0, i;

	ni = ieee80211_ref_node(vap->iv_bss);
	rs = &ni->ni_rates;
	rs_ht = (struct ieee80211_rateset *) &ni->ni_htrates;

	/* Get normal and basic rates mask. */
	rates = basicrates = 0;
	maxrate = maxbasicrate = 0;

	/* This is for 11bg */
	for (i = 0; i < rs->rs_nrates; i++) {
		/* Convert 802.11 rate to HW rate index. */
		ridx = rate2ridx(IEEE80211_RV(rs->rs_rates[i]));
		if (ridx == URTWN_RIDX_UNKNOWN)	/* Unknown rate, skip. */
			continue;
		rates |= 1 << ridx;
		if (ridx > maxrate)
			maxrate = ridx;
		if (rs->rs_rates[i] & IEEE80211_RATE_BASIC) {
			basicrates |= 1 << ridx;
			if (ridx > maxbasicrate)
				maxbasicrate = ridx;
		}
	}

	/* If we're doing 11n, enable 11n rates */
	if (ni->ni_flags & IEEE80211_NODE_HT) {
		for (i = 0; i < rs_ht->rs_nrates; i++) {
			if ((rs_ht->rs_rates[i] & 0x7f) > 0xf)
				continue;
			/* 11n rates start at index 12 */
			ridx = ((rs_ht->rs_rates[i]) & 0xf) + 12;
			rates |= (1 << ridx);

			/* Guard against the rate table being oddly ordered */
			if (ridx > maxrate)
				maxrate = ridx;
		}
	}

	/* NB: group addressed frames are done at 11bg rates for now */
	if (ic->ic_curmode == IEEE80211_MODE_11B)
		mode = R92C_RAID_11B;
	else
		mode = R92C_RAID_11BG;
	/* XXX misleading 'mode' value here for unicast frames */
	URTWN_DPRINTF(sc, URTWN_DEBUG_RA,
	    "%s: mode 0x%x, rates 0x%08x, basicrates 0x%08x\n", __func__,
	    mode, rates, basicrates);

	/* Set rates mask for group addressed frames. */
	cmd.macid = URTWN_MACID_BC | URTWN_MACID_VALID;
	cmd.mask = htole32(mode << 28 | basicrates);
	error = urtwn_fw_cmd(sc, R92C_CMD_MACID_CONFIG, &cmd, sizeof(cmd));
	if (error != 0) {
		ieee80211_free_node(ni);
		device_printf(sc->sc_dev,
		    "could not add broadcast station\n");
		return (error);
	}

	/* Set initial MRR rate. */
	URTWN_DPRINTF(sc, URTWN_DEBUG_RA, "%s: maxbasicrate %d\n", __func__,
	    maxbasicrate);
	urtwn_write_1(sc, R92C_INIDATA_RATE_SEL(URTWN_MACID_BC),
	    maxbasicrate);

	/* Set rates mask for unicast frames. */
	if (ni->ni_flags & IEEE80211_NODE_HT)
		mode = R92C_RAID_11GN;
	else if (ic->ic_curmode == IEEE80211_MODE_11B)
		mode = R92C_RAID_11B;
	else
		mode = R92C_RAID_11BG;
	cmd.macid = URTWN_MACID_BSS | URTWN_MACID_VALID;
	cmd.mask = htole32(mode << 28 | rates);
	error = urtwn_fw_cmd(sc, R92C_CMD_MACID_CONFIG, &cmd, sizeof(cmd));
	if (error != 0) {
		ieee80211_free_node(ni);
		device_printf(sc->sc_dev, "could not add BSS station\n");
		return (error);
	}
	/* Set initial MRR rate. */
	URTWN_DPRINTF(sc, URTWN_DEBUG_RA, "%s: maxrate %d\n", __func__,
	    maxrate);
	urtwn_write_1(sc, R92C_INIDATA_RATE_SEL(URTWN_MACID_BSS),
	    maxrate);

	/* Indicate highest supported rate. */
	if (ni->ni_flags & IEEE80211_NODE_HT)
		ni->ni_txrate = rs_ht->rs_rates[rs_ht->rs_nrates - 1]
		    | IEEE80211_RATE_MCS;
	else
		ni->ni_txrate = rs->rs_rates[rs->rs_nrates - 1];
	ieee80211_free_node(ni);

	URTWN_DPRINTF(sc, URTWN_DEBUG_BEACON, "%s: beacon was %srecognized\n",
	    __func__, urtwn_read_1(sc, R92C_TDECTRL + 2) &
	    (R92C_TDECTRL_BCN_VALID >> 16) ? "" : "not ");

	return (0);
}
#endif	/* URTWM_TODO */

static int
urtwm_ioctl_reset(struct ieee80211vap *vap, u_long cmd)
{
	int error;

	switch (cmd) {
#ifndef URTWM_WITHOUT_UCODE
	case IEEE80211_IOC_POWERSAVE:
	case IEEE80211_IOC_POWERSAVESLEEP:
	{
		struct urtwm_softc *sc = vap->iv_ic->ic_softc;

		if (vap->iv_opmode == IEEE80211_M_STA) {
			URTWM_LOCK(sc);
			if (sc->sc_flags & URTWM_RUNNING)
				error = urtwm_set_pwrmode(sc, vap, 1);
			else
				error = 0;
			URTWM_UNLOCK(sc);
			if (error != 0)
				error = ENETRESET;
		} else
			error = EOPNOTSUPP;
		break;
	}
#endif
	case IEEE80211_IOC_SHORTGI:
		error = 0;
		break;
	default:
		error = ENETRESET;
		break;
	}

	return (error);
}

static void
urtwm_init_beacon(struct urtwm_softc *sc, struct urtwm_vap *uvp)
{
	struct r12a_tx_desc *txd = &uvp->bcn_desc;

	txd->offset = sizeof(*txd);
	txd->flags0 = R12A_FLAGS0_LSG | R12A_FLAGS0_FSG | R12A_FLAGS0_OWN |
	    R12A_FLAGS0_BMCAST;

	/*
	 * NB: there is no need to setup HWSEQ_EN bit;
	 * QSEL_BEACON already implies it.
	 */
	txd->txdw1 = htole32(SM(R12A_TXDW1_QSEL, R12A_TXDW1_QSEL_BEACON));
	txd->txdw1 |= htole32(SM(R12A_TXDW1_MACID, URTWM_MACID_BC));

	txd->txdw3 = htole32(R12A_TXDW3_DRVRATE);
}

static int
urtwm_setup_beacon(struct urtwm_softc *sc, struct ieee80211_node *ni)
{
 	struct ieee80211vap *vap = ni->ni_vap;
	struct urtwm_vap *uvp = URTWM_VAP(vap);
	struct r12a_tx_desc *txd = &uvp->bcn_desc;
	struct mbuf *m;
	int error;

	URTWM_ASSERT_LOCKED(sc);

	if (ni->ni_chan == IEEE80211_CHAN_ANYC)
		return (EINVAL);

	m = ieee80211_beacon_alloc(ni);
	if (m == NULL) {
		device_printf(sc->sc_dev,
		    "%s: could not allocate beacon frame\n", __func__);
		return (ENOMEM);
	}

	if (uvp->bcn_mbuf != NULL)
		m_freem(uvp->bcn_mbuf);

	uvp->bcn_mbuf = m;

	txd->txdw4 &= ~htole32(R12A_TXDW4_DATARATE_M);
	if (IEEE80211_IS_CHAN_5GHZ(ni->ni_chan)) {
		txd->txdw4 = htole32(SM(R12A_TXDW4_DATARATE,
		    URTWM_RIDX_OFDM6));
	} else
		txd->txdw4 = htole32(SM(R12A_TXDW4_DATARATE, URTWM_RIDX_CCK1));

	if ((error = urtwm_tx_beacon(sc, uvp)) != 0)
		return (error);

	/* XXX bcnq stuck workaround */
	if ((error = urtwm_tx_beacon(sc, uvp)) != 0)
		return (error);

	return (0);
}

static void
urtwm_update_beacon(struct ieee80211vap *vap, int item)
{
	struct urtwm_softc *sc = vap->iv_ic->ic_softc;
	struct urtwm_vap *uvp = URTWM_VAP(vap);
	struct ieee80211_beacon_offsets *bo = &vap->iv_bcn_off;
	struct ieee80211_node *ni = vap->iv_bss;
	int mcast = 0;

	URTWM_LOCK(sc);
	if (uvp->bcn_mbuf == NULL) {
		uvp->bcn_mbuf = ieee80211_beacon_alloc(ni);
		if (uvp->bcn_mbuf == NULL) {
			device_printf(sc->sc_dev,
			    "%s: could not allocate beacon frame\n", __func__);
			URTWM_UNLOCK(sc);
			return;
		}
	}
	URTWM_UNLOCK(sc);

	if (item == IEEE80211_BEACON_TIM)
		mcast = 1;	/* XXX */

	setbit(bo->bo_flags, item);
	ieee80211_beacon_update(ni, uvp->bcn_mbuf, mcast);

	URTWM_LOCK(sc);
	urtwm_tx_beacon(sc, uvp);
	URTWM_UNLOCK(sc);
}

/*
 * Push a beacon frame into the chip. Beacon will
 * be repeated by the chip every R92C_BCN_INTERVAL.
 */
static int
urtwm_tx_beacon(struct urtwm_softc *sc, struct urtwm_vap *uvp)
{
	struct r12a_tx_desc *desc = &uvp->bcn_desc;
	struct urtwm_data *bf;

	URTWM_ASSERT_LOCKED(sc);

	bf = urtwm_getbuf(sc);
	if (bf == NULL)
		return (ENOMEM);

	memcpy(bf->buf, desc, sizeof(*desc));
	urtwm_tx_start(sc, uvp->bcn_mbuf, IEEE80211_FC0_TYPE_MGT, bf);

	return (0);
}

#ifndef URTWM_WITHOUT_UCODE
static int
urtwm_construct_nulldata(struct urtwm_softc *sc, struct ieee80211vap *vap,
    uint8_t *ptr, int qos)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct r12a_tx_desc *txd;
	struct ieee80211_frame *wh;

	txd = (struct r12a_tx_desc *)ptr;
	txd->offset = sizeof(*txd);
	txd->flags0 = R12A_FLAGS0_OWN | R12A_FLAGS0_FSG | R12A_FLAGS0_LSG;
	txd->txdw1 = htole32(
	    SM(R12A_TXDW1_QSEL, R12A_TXDW1_QSEL_MGNT));

	txd->txdw3 = htole32(R12A_TXDW3_DRVRATE);
	if (ic->ic_curmode == IEEE80211_MODE_11B) {
		txd->txdw4 = htole32(SM(R12A_TXDW4_DATARATE,
		    URTWM_RIDX_CCK1));
	} else {
		txd->txdw4 = htole32(SM(R12A_TXDW4_DATARATE,
		    URTWM_RIDX_OFDM6));
	}

	/* XXX obtain from net80211 */
	wh = (struct ieee80211_frame *)(txd + 1);
	wh->i_fc[0] = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_DATA;
	wh->i_fc[1] = IEEE80211_FC1_DIR_TODS;
	IEEE80211_ADDR_COPY(wh->i_addr1, vap->iv_bss->ni_bssid);
	IEEE80211_ADDR_COPY(wh->i_addr2, vap->iv_myaddr);
	IEEE80211_ADDR_COPY(wh->i_addr3, vap->iv_bss->ni_macaddr);

	if (qos) {
		struct ieee80211_qosframe *qwh;
		const int tid = WME_AC_TO_TID(WME_AC_BE);

		txd->pktlen = htole16(sizeof(*qwh));
		qwh = (struct ieee80211_qosframe *)ptr;
		qwh->i_fc[0] |= IEEE80211_FC0_SUBTYPE_QOS_NULL;
		qwh->i_qos[0] = tid & IEEE80211_QOS_TID;
	} else {
		txd->txdw8 = htole32(R12A_TXDW8_HWSEQ_EN);

		txd->pktlen = htole16(sizeof(*wh));
		wh->i_fc[0] |= IEEE80211_FC0_SUBTYPE_NODATA;
	}

	urtwm_tx_checksum(txd);

	return (sizeof(*txd) + le16toh(txd->pktlen));
}

static int
urtwm_push_nulldata(struct urtwm_softc *sc, struct ieee80211vap *vap)
{
	struct urtwm_vap *uvp = URTWM_VAP(vap);
	struct ieee80211com *ic = vap->iv_ic;
	struct ieee80211_channel *c = ic->ic_curchan;
	struct r12a_fw_cmd_rsvdpage rsvd;
	struct r12a_tx_desc *txd;
	struct urtwm_data *data;
	uint8_t *ptr;
	int required_size, bcn_size, null_size, error, ntries;

	if (!(sc->sc_flags & URTWM_FW_LOADED))
		return (0);	/* requires firmware */

	KASSERT(sc->page_size > 0, ("page size was not set!\n"));

	data = urtwm_getbuf(sc);
	if (data == NULL)
		return (ENOMEM);

	/* Leave some space for beacon (multi-vap) */
	bcn_size = roundup(URTWM_BCN_MAX_SIZE, sc->page_size);
	/* 1 page for Null Data + 1 page for Qos Null Data frames. */
	required_size = bcn_size + sc->page_size * 2;

	/* Setup beacon descriptor. */
	memcpy(data->buf, &uvp->bcn_desc, sizeof(uvp->bcn_desc));

	txd = (struct r12a_tx_desc *)data->buf;
	txd->txdw4 &= ~htole32(R12A_TXDW4_DATARATE_M);
	if (IEEE80211_IS_CHAN_5GHZ(c)) {
		txd->txdw4 = htole32(SM(R12A_TXDW4_DATARATE,
		    URTWM_RIDX_OFDM6));
	} else
		txd->txdw4 = htole32(SM(R12A_TXDW4_DATARATE, URTWM_RIDX_CCK1));
	txd->pktlen = htole16(required_size - sizeof(*txd));

	/* Compute Tx descriptor checksum. */
	urtwm_tx_checksum(txd);

	ptr = (uint8_t *)(txd + 1);
	memset(ptr, 0, required_size - sizeof(*txd));

	/* Construct Null Data frame. */
	ptr += bcn_size - sizeof(*txd);
	null_size = urtwm_construct_nulldata(sc, vap, ptr, 0);
	KASSERT(null_size < sc->page_size,
	    ("recalculate size for Null Data frame\n"));

	/* Construct Qos Null Data frame. */
	ptr += roundup(null_size, sc->page_size);
	null_size = urtwm_construct_nulldata(sc, vap, ptr, 1);
	KASSERT(null_size < sc->page_size,
	    ("recalculate size for Qos Null Data frame\n"));

	/* Do not try to detect a beacon here. */
	urtwm_setbits_1_shift(sc, R92C_CR, 0, R92C_CR_ENSWBCN, 1);
	urtwm_setbits_1_shift(sc, R92C_FWHW_TXQ_CTRL,
	    R92C_FWHW_TXQ_CTRL_REAL_BEACON, 0, 2);
	/* Clear 'beacon valid' bit. */
	urtwm_setbits_1_shift(sc, R92C_TDECTRL, R92C_TDECTRL_BCN_VALID, 0, 2);

	data->buflen = required_size;
	STAILQ_INSERT_TAIL(&sc->sc_tx_pending, data, next);
	usbd_transfer_start(sc->sc_xfer[URTWM_BULK_TX_VO]);

	for (ntries = 0; ntries < 10; ntries++) {
		if (urtwm_read_4(sc, R92C_TDECTRL) & R92C_TDECTRL_BCN_VALID) {
			URTWM_DPRINTF(sc, URTWM_DEBUG_BEACON,
			    "%s: frame was recognized\n", __func__);
			break;
		}
		urtwm_delay(sc, 100);
	}
	if (ntries == 10) {
		device_printf(sc->sc_dev, "%s: frame was not recognized!\n",
		    __func__);
		return (EINVAL);
	}

	/* Setup addresses in firmware. */
	rsvd.probe_resp = 0;
	rsvd.ps_poll = 0;
	rsvd.null_data = howmany(bcn_size, sc->page_size);
	rsvd.null_data_qos = rsvd.null_data + 1;
	rsvd.null_data_qos_bt = 0;
	error = urtwm_fw_cmd(sc, R12A_CMD_RSVD_PAGE, &rsvd, sizeof(rsvd));
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "%s: CMD_RSVD_PAGE was not sent, error %d\n",
		    __func__, error);
		return (error);
	}

	/* Re-enable beacon detection. */
	urtwm_setbits_1_shift(sc, R92C_FWHW_TXQ_CTRL,
	    0, R92C_FWHW_TXQ_CTRL_REAL_BEACON, 2);
	urtwm_setbits_1_shift(sc, R92C_CR, R92C_CR_ENSWBCN, 0, 1);

	/* Setup power management. */
	/*
	 * NB: it will be enabled immediately - delay it,
	 * so 4-Way handshake will not be interrupted.
	 */
	callout_reset(&sc->sc_pwrmode_init, 5*hz, urtwm_pwrmode_init, sc);

	return (0);
}

static void
urtwm_pwrmode_init(void *arg)
{
	struct urtwm_softc *sc = arg;

	urtwm_cmd_sleepable(sc, NULL, 0, urtwm_set_pwrmode_cb);
}

static void
urtwm_set_pwrmode_cb(struct urtwm_softc *sc, union sec_param *data)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);

	if (vap != NULL)
		urtwm_set_pwrmode(sc, vap, 1);
}

static int
urtwm_set_pwrmode(struct urtwm_softc *sc, struct ieee80211vap *vap, int off)
{
	struct r12a_fw_cmd_pwrmode mode;
	int error;

	if (off && vap->iv_state == IEEE80211_S_RUN &&
	    (vap->iv_flags & IEEE80211_F_PMGTON)) {
		mode.mode = R12A_PWRMODE_LEG;
		/*
		 * TODO: switch to RFOFF state
		 * (something is missing here - Rx stops with it).
		 */
#ifdef URTWM_TODO
		mode.pwr_state = R12A_PWRMODE_STATE_RFOFF;
#else
		mode.pwr_state = R12A_PWRMODE_STATE_RFON;
#endif
	} else {
		mode.mode = R12A_PWRMODE_CAM;
		mode.pwr_state = R12A_PWRMODE_STATE_ALLON;
	}
	mode.pwrb1 =
	    SM(R12A_PWRMODE_B1_SMART_PS, R12A_PWRMODE_B1_LEG_NULLDATA) |
	    SM(R12A_PWRMODE_B1_RLBM, R12A_PWRMODE_B1_MODE_MIN);
	/* XXX ignored */
	mode.bcn_pass = 0;
	mode.queue_uapsd = 0;
	mode.pwrb5 = R12A_PWRMODE_B5_NO_BTCOEX;
	error = urtwm_fw_cmd(sc, R12A_CMD_SET_PWRMODE, &mode, sizeof(mode));
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "%s: CMD_SET_PWRMODE was not sent, error %d\n",
		    __func__, error);
	}

	return (error);
}

static void
urtwm_set_media_status(struct urtwm_softc *sc, union sec_param *data)
{
	uint8_t macid = data->macid;
	struct r12a_fw_cmd_msrrpt status;
	int error;

	if (macid & URTWM_MACID_VALID)
		status.msrb0 = R12A_MSRRPT_B0_ASSOC;
	else
		status.msrb0 = R12A_MSRRPT_B0_DISASSOC;

	status.macid = (macid & ~URTWM_MACID_VALID);
	status.macid_end = 0;

	error = urtwm_fw_cmd(sc, R12A_CMD_MSR_RPT, &status, sizeof(status));
	if (error != 0)
		device_printf(sc->sc_dev, "cannot change media status!\n");
}
#endif

static int
urtwm_key_alloc(struct ieee80211vap *vap, struct ieee80211_key *k,
    ieee80211_keyix *keyix, ieee80211_keyix *rxkeyix)
{
	struct urtwm_softc *sc = vap->iv_ic->ic_softc;
	uint8_t i;

	if (!(&vap->iv_nw_keys[0] <= k &&
	     k < &vap->iv_nw_keys[IEEE80211_WEP_NKID])) {
		if (!(k->wk_flags & IEEE80211_KEY_SWCRYPT)) {
			URTWM_LOCK(sc);
			/*
			 * First 4 slots for group keys,
			 * what is left - for pairwise.
			 * XXX incompatible with IBSS RSN.
			 */
			for (i = IEEE80211_WEP_NKID;
			     i < R92C_CAM_ENTRY_COUNT; i++) {
				if ((sc->keys_bmap & (1 << i)) == 0) {
					sc->keys_bmap |= 1 << i;
					*keyix = i;
					break;
				}
			}
			URTWM_UNLOCK(sc);
			if (i == R92C_CAM_ENTRY_COUNT) {
				device_printf(sc->sc_dev,
				    "%s: no free space in the key table\n",
				    __func__);
				return 0;
			}
		} else
			*keyix = 0;
	} else {
		*keyix = k - vap->iv_nw_keys;
	}
	*rxkeyix = *keyix;
	return 1;
}

static void
urtwm_key_set_cb(struct urtwm_softc *sc, union sec_param *data)
{
	struct ieee80211_key *k = &data->key;
	uint8_t algo, keyid;
	int i, error;

	if (k->wk_keyix < IEEE80211_WEP_NKID)
		keyid = k->wk_keyix;
	else
		keyid = 0;

	/* Map net80211 cipher to HW crypto algorithm. */
	switch (k->wk_cipher->ic_cipher) {
	case IEEE80211_CIPHER_WEP:
		if (k->wk_keylen < 8)
			algo = R92C_CAM_ALGO_WEP40;
		else
			algo = R92C_CAM_ALGO_WEP104;
		break;
	case IEEE80211_CIPHER_TKIP:
		algo = R92C_CAM_ALGO_TKIP;
		break;
	case IEEE80211_CIPHER_AES_CCM:
		algo = R92C_CAM_ALGO_AES;
		break;
	default:
		device_printf(sc->sc_dev, "%s: unknown cipher %d\n",
		    __func__, k->wk_cipher->ic_cipher);
		return;
	}

	URTWM_DPRINTF(sc, URTWM_DEBUG_KEY,
	    "%s: keyix %d, keyid %d, algo %d/%d, flags %04X, len %d, "
	    "macaddr %s\n", __func__, k->wk_keyix, keyid,
	    k->wk_cipher->ic_cipher, algo, k->wk_flags, k->wk_keylen,
	    ether_sprintf(k->wk_macaddr));

	/* Clear high bits. */
	urtwm_cam_write(sc, R92C_CAM_CTL6(k->wk_keyix), 0);
	urtwm_cam_write(sc, R92C_CAM_CTL7(k->wk_keyix), 0);

	/* Write key. */
	for (i = 0; i < 4; i++) {
		error = urtwm_cam_write(sc, R92C_CAM_KEY(k->wk_keyix, i),
		    le32dec(&k->wk_key[i * 4]));
		if (error != 0)
			goto fail;
	}

	/* Write CTL0 last since that will validate the CAM entry. */
	error = urtwm_cam_write(sc, R92C_CAM_CTL1(k->wk_keyix),
	    le32dec(&k->wk_macaddr[2]));
	if (error != 0)
		goto fail;
	error = urtwm_cam_write(sc, R92C_CAM_CTL0(k->wk_keyix),
	    SM(R92C_CAM_ALGO, algo) |
	    SM(R92C_CAM_KEYID, keyid) |
	    SM(R92C_CAM_MACLO, le16dec(&k->wk_macaddr[0])) |
	    R92C_CAM_VALID);
	if (error != 0)
		goto fail;

	return;

fail:
	device_printf(sc->sc_dev, "%s fails, error %d\n", __func__, error);
}

static void
urtwm_key_del_cb(struct urtwm_softc *sc, union sec_param *data)
{
	struct ieee80211_key *k = &data->key;
	int i;

	URTWM_DPRINTF(sc, URTWM_DEBUG_KEY,
	    "%s: keyix %d, flags %04X, macaddr %s\n", __func__,
	    k->wk_keyix, k->wk_flags, ether_sprintf(k->wk_macaddr));

	urtwm_cam_write(sc, R92C_CAM_CTL0(k->wk_keyix), 0);
	urtwm_cam_write(sc, R92C_CAM_CTL1(k->wk_keyix), 0);

	/* Clear key. */
	for (i = 0; i < 4; i++)
		urtwm_cam_write(sc, R92C_CAM_KEY(k->wk_keyix, i), 0);
	sc->keys_bmap &= ~(1 << k->wk_keyix);
}

static int
urtwm_process_key(struct ieee80211vap *vap, const struct ieee80211_key *k,
    int set)
{
	struct urtwm_softc *sc = vap->iv_ic->ic_softc;
	struct urtwm_vap *uvp = URTWM_VAP(vap);

	if (k->wk_flags & IEEE80211_KEY_SWCRYPT) {
		/* Not for us. */
		return (1);
	}

	if (&vap->iv_nw_keys[0] <= k &&
	    k < &vap->iv_nw_keys[IEEE80211_WEP_NKID]) {
		URTWM_LOCK(sc);		/* XXX */
		uvp->keys[k->wk_keyix] = set ? k : NULL;
		if ((sc->sc_flags & URTWM_RUNNING) == 0) {
			/*
			 * The device was not started;
			 * the key will be installed later.
			 */
			URTWM_UNLOCK(sc);
			return (1);
		}
		URTWM_UNLOCK(sc);
	}

	return (!urtwm_cmd_sleepable(sc, k, sizeof(*k),
	    set ? urtwm_key_set_cb : urtwm_key_del_cb));
}

static int
urtwm_key_set(struct ieee80211vap *vap, const struct ieee80211_key *k)
{
	return (urtwm_process_key(vap, k, 1));
}

static int
urtwm_key_delete(struct ieee80211vap *vap, const struct ieee80211_key *k)
{
	return (urtwm_process_key(vap, k, 0));
}

static void
urtwm_tsf_sync_adhoc(void *arg)
{
	struct ieee80211vap *vap = arg;
	struct ieee80211com *ic = vap->iv_ic;
	struct urtwm_vap *uvp = URTWM_VAP(vap);

	if (vap->iv_state == IEEE80211_S_RUN) {
		/* Do it in process context. */
		ieee80211_runtask(ic, &uvp->tsf_sync_adhoc_task);
	}
}

/*
 * Workaround for TSF synchronization:
 * when BSSID filter in IBSS mode is not set
 * (and TSF synchronization is enabled), then any beacon may update it.
 * This routine synchronizes it when BSSID matching is enabled (IBSS merge
 * is not possible during this period).
 */
static void
urtwm_tsf_sync_adhoc_task(void *arg, int pending)
{
	struct ieee80211vap *vap = arg;
	struct urtwm_vap *uvp = URTWM_VAP(vap);
	struct urtwm_softc *sc = vap->iv_ic->ic_softc;
	struct ieee80211_node *ni;

	URTWM_LOCK(sc);
	ni = ieee80211_ref_node(vap->iv_bss);

	/* Accept beacons with the same BSSID. */
	urtwm_set_rx_bssid_all(sc, 0);

        /* Enable synchronization. */
	urtwm_setbits_1(sc, R92C_BCN_CTRL, R92C_BCN_CTRL_DIS_TSF_UDT0, 0);

	/* Synchronize. */
	usb_pause_mtx(&sc->sc_mtx, hz * ni->ni_intval * 5 / 1000);

	/* Disable synchronization. */
	urtwm_setbits_1(sc, R92C_BCN_CTRL, 0, R92C_BCN_CTRL_DIS_TSF_UDT0);

	/* Accept all beacons. */
	urtwm_set_rx_bssid_all(sc, 1);

	/* Schedule next TSF synchronization. */
	callout_reset(&uvp->tsf_sync_adhoc, 60*hz, urtwm_tsf_sync_adhoc, vap);

	ieee80211_free_node(ni);
	URTWM_UNLOCK(sc);
}

static void
urtwm_tsf_sync_enable(struct urtwm_softc *sc, struct ieee80211vap *vap)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct urtwm_vap *uvp = URTWM_VAP(vap);

	/* Reset TSF. */
	urtwm_write_1(sc, R92C_DUAL_TSF_RST, R92C_DUAL_TSF_RST0);

	switch (vap->iv_opmode) {
	case IEEE80211_M_STA:
		/* Enable TSF synchronization. */
		urtwm_setbits_1(sc, R92C_BCN_CTRL, R92C_BCN_CTRL_DIS_TSF_UDT0,
		    0);
		break;
	case IEEE80211_M_IBSS:
		ieee80211_runtask(ic, &uvp->tsf_sync_adhoc_task);
		/* FALLTHROUGH */
	case IEEE80211_M_HOSTAP:
		/* Enable beaconing. */
		urtwm_setbits_1(sc, R92C_BCN_CTRL, 0, R92C_BCN_CTRL_EN_BCN);
		break;
	default:
		device_printf(sc->sc_dev, "undefined opmode %d\n",
		    vap->iv_opmode);
		return;
	}
}

static uint32_t
urtwm_get_tsf_low(struct urtwm_softc *sc, int id)
{
	return (urtwm_read_4(sc, R92C_TSFTR(id)));
}

static uint32_t
urtwm_get_tsf_high(struct urtwm_softc *sc, int id)
{
	return (urtwm_read_4(sc, R92C_TSFTR(id) + 4));
}

static void
urtwm_get_tsf(struct urtwm_softc *sc, uint64_t *buf, int id)
{
	/* NB: we cannot read it at once. */
	*buf = urtwm_get_tsf_high(sc, id);
	*buf <<= 32;
	*buf += urtwm_get_tsf_low(sc, id);
}

static void
urtwm_r12a_set_led_mini(struct urtwm_softc *sc, int led, int on)
{
	if (led == URTWM_LED_LINK) {
		if (on)
			urtwm_setbits_1(sc, R92C_LEDCFG2, 0x0f, 0x60);
		else {
			urtwm_setbits_1(sc, R92C_LEDCFG2, 0x6f, 0x08);
			urtwm_setbits_1(sc, R92C_MAC_PINMUX_CFG, 0x01, 0);
		}
		sc->ledlink = on;	/* Save LED state. */
	}

	/* XXX led #1? */
}

static void
urtwm_r12a_set_led(struct urtwm_softc *sc, int led, int on)
{
	/* XXX assume led #0 == LED_LINK */
	/* XXX antenna diversity */

	if (led == URTWM_LED_LINK) {
		urtwm_setbits_1(sc, R92C_LEDCFG0, 0x8f,
		    R12A_LEDCFG2_ENA | (on ? 0 : R92C_LEDCFG0_DIS));
		sc->ledlink = on;	/* Save LED state. */
	}

	/* XXX leds #1/#2 ? */
}

static void
urtwm_r21a_set_led(struct urtwm_softc *sc, int led, int on)
{
	if (led == URTWM_LED_LINK) {
		urtwm_write_1(sc, R92C_LEDCFG2,
		    R12A_LEDCFG2_ENA | (on ? 0 : R92C_LEDCFG0_DIS));
		sc->ledlink = on;	/* Save LED state. */
	}
}

static void
urtwm_set_mode(struct urtwm_softc *sc, uint8_t mode, int id)
{
	urtwm_setbits_1(sc, R92C_MSR, R92C_MSR_MASK << id * 2, mode << id * 2);
}

static void
urtwm_adhoc_recv_mgmt(struct ieee80211_node *ni, struct mbuf *m, int subtype,
    const struct ieee80211_rx_stats *rxs,
    int rssi, int nf)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct urtwm_softc *sc = vap->iv_ic->ic_softc;
	struct urtwm_vap *uvp = URTWM_VAP(vap);
	uint64_t ni_tstamp, curr_tstamp;

	uvp->recv_mgmt(ni, m, subtype, rxs, rssi, nf);

	if (vap->iv_state == IEEE80211_S_RUN &&
	    (subtype == IEEE80211_FC0_SUBTYPE_BEACON ||
	    subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP)) {
		ni_tstamp = le64toh(ni->ni_tstamp.tsf);
		URTWM_LOCK(sc);
		urtwm_get_tsf(sc, &curr_tstamp, 0);
		URTWM_UNLOCK(sc);

		if (ni_tstamp >= curr_tstamp)
			(void) ieee80211_ibss_merge(ni);
	}
}

static int
urtwm_newstate(struct ieee80211vap *vap, enum ieee80211_state nstate, int arg)
{
	struct urtwm_vap *uvp = URTWM_VAP(vap);
	struct ieee80211com *ic = vap->iv_ic;
	struct urtwm_softc *sc = ic->ic_softc;
	struct ieee80211_node *ni;
	enum ieee80211_state ostate;
	uint32_t reg;
	uint8_t mode;
	int error = 0;

	ostate = vap->iv_state;
	URTWM_DPRINTF(sc, URTWM_DEBUG_STATE, "%s -> %s\n",
	    ieee80211_state_name[ostate], ieee80211_state_name[nstate]);

	IEEE80211_UNLOCK(ic);
	URTWM_LOCK(sc);
	if (ostate == IEEE80211_S_RUN) {
		/* Stop calibration. */
		callout_stop(&sc->sc_calib_to);

		if (vap->iv_opmode == IEEE80211_M_IBSS) {
			/* Stop periodical TSF synchronization. */
			callout_stop(&uvp->tsf_sync_adhoc);
		}

#ifndef URTWM_WITHOUT_UCODE
		/* Disable power management. */
		callout_stop(&sc->sc_pwrmode_init);
		urtwm_set_pwrmode(sc, vap, 0);
#endif

		/* Turn link LED off. */
		urtwm_set_led(sc, URTWM_LED_LINK, 0);

		/* Set media status to 'No Link'. */
		urtwm_set_mode(sc, R92C_MSR_NOLINK, 0);

		/* Stop Rx of data frames. */
		urtwm_write_2(sc, R92C_RXFLTMAP2, 0);

		/* Disable TSF synchronization / beaconing. */
		urtwm_setbits_1(sc, R92C_BCN_CTRL, R92C_BCN_CTRL_EN_BCN,
		    R92C_BCN_CTRL_DIS_TSF_UDT0);

		/* Reset TSF. */
		urtwm_write_1(sc, R92C_DUAL_TSF_RST, R92C_DUAL_TSF_RST0);

		/* Reset EDCA parameters. */
		urtwm_write_4(sc, R92C_EDCA_VO_PARAM, 0x002f3217);
		urtwm_write_4(sc, R92C_EDCA_VI_PARAM, 0x005e4317);
		urtwm_write_4(sc, R92C_EDCA_BE_PARAM, 0x00105320);
		urtwm_write_4(sc, R92C_EDCA_BK_PARAM, 0x0000a444);
	}

	switch (nstate) {
	case IEEE80211_S_SCAN:
		/* Pause AC Tx queues. */
		urtwm_setbits_1(sc, R92C_TXPAUSE, 0, R92C_TX_QUEUE_AC);
		break;
	case IEEE80211_S_RUN:
		if (vap->iv_opmode == IEEE80211_M_MONITOR) {
			/* Turn link LED on. */
			urtwm_set_led(sc, URTWM_LED_LINK, 1);
			break;
		}

		ni = ieee80211_ref_node(vap->iv_bss);

		if (ic->ic_bsschan == IEEE80211_CHAN_ANYC ||
		    ni->ni_chan == IEEE80211_CHAN_ANYC) {
			device_printf(sc->sc_dev,
			    "%s: could not move to RUN state\n", __func__);
			error = EINVAL;
			goto end_run;
		}

		switch (vap->iv_opmode) {
		case IEEE80211_M_STA:
			mode = R92C_MSR_INFRA;
			break;
		case IEEE80211_M_IBSS:
			mode = R92C_MSR_ADHOC;
			break;
		case IEEE80211_M_HOSTAP:
			mode = R92C_MSR_AP;
			break;
		default:
			device_printf(sc->sc_dev, "undefined opmode %d\n",
			    vap->iv_opmode);
			error = EINVAL;
			goto end_run;
		}

		/* Set media status to 'Associated'. */
		urtwm_set_mode(sc, mode, 0);

		/* Set AssocID. */
		urtwm_write_2(sc, R92C_BCN_PSR_RPT,
		    0xc000 | IEEE80211_NODE_AID(ni));

		/* Set BSSID. */
		urtwm_write_4(sc, R92C_BSSID + 0, le32dec(&ni->ni_bssid[0]));
		urtwm_write_4(sc, R92C_BSSID + 4, le16dec(&ni->ni_bssid[4]));

		/* Enable Rx of data frames. */
		urtwm_write_2(sc, R92C_RXFLTMAP2, 0xffff);

		/* Flush all AC queues. */
		urtwm_write_1(sc, R92C_TXPAUSE, 0);

		/* Set beacon interval. */
		urtwm_write_2(sc, R92C_BCN_INTERVAL, ni->ni_intval);

		/* Allow Rx from our BSSID only. */
		if (ic->ic_promisc == 0) {
			reg = urtwm_read_4(sc, R92C_RCR);

			if (vap->iv_opmode != IEEE80211_M_HOSTAP) {
				reg |= R92C_RCR_CBSSID_DATA;
				if (vap->iv_opmode != IEEE80211_M_IBSS)
					reg |= R92C_RCR_CBSSID_BCN;
			}

			urtwm_write_4(sc, R92C_RCR, reg);
		}

#ifndef URTWM_WITHOUT_UCODE
		/* Upload (QoS) Null Data frame to firmware. */
		if (vap->iv_opmode == IEEE80211_M_STA)
			urtwm_push_nulldata(sc, vap);
#endif

		if (vap->iv_opmode == IEEE80211_M_HOSTAP ||
		    vap->iv_opmode == IEEE80211_M_IBSS) {
			error = urtwm_setup_beacon(sc, ni);
			if (error != 0) {
				device_printf(sc->sc_dev,
				    "unable to push beacon into the chip, "
				    "error %d\n", error);
				goto end_run;
			}
		}

		/* Enable TSF synchronization. */
		urtwm_tsf_sync_enable(sc, vap);

#ifdef URTWM_TODO
		urtwn_write_1(sc, R92C_SIFS_CCK + 1, 10);
		urtwn_write_1(sc, R92C_SIFS_OFDM + 1, 10);
		urtwn_write_1(sc, R92C_SPEC_SIFS + 1, 10);
		urtwn_write_1(sc, R92C_MAC_SPEC_SIFS + 1, 10);
		urtwn_write_1(sc, R92C_R2T_SIFS + 1, 10);
		urtwn_write_1(sc, R92C_T2T_SIFS + 1, 10);
#endif

		/* Turn link LED on. */
		urtwm_set_led(sc, URTWM_LED_LINK, 1);

		/* Reset temperature calibration state machine. */
		sc->sc_flags &= ~URTWM_TEMP_MEASURED;
		sc->thcal_temp = sc->thermal_meter;

		/* Start periodic calibration. */
		callout_reset(&sc->sc_calib_to, 2*hz, urtwm_calib_to, sc);

end_run:
		ieee80211_free_node(ni);
		break;
	default:
		break;
	}

	URTWM_UNLOCK(sc);
	IEEE80211_LOCK(ic);
	return (error != 0 ? error : uvp->newstate(vap, nstate, arg));
}

static void
urtwm_calib_to(void *arg)
{
	struct urtwm_softc *sc = arg;

	/* Do it in a process context. */
	urtwm_cmd_sleepable(sc, NULL, 0, urtwm_calib_cb);
}

static void
urtwm_calib_cb(struct urtwm_softc *sc, union sec_param *data)
{
	/* Do temperature compensation. */
	urtwm_temp_calib(sc);

	if ((urtwm_read_1(sc, R92C_MSR) & R92C_MSR_MASK) != R92C_MSR_NOLINK)
		callout_reset(&sc->sc_calib_to, 2*hz, urtwm_calib_to, sc);
}

static int8_t
urtwm_r12a_get_rssi_cck(struct urtwm_softc *sc, void *physt)
{
	struct r12a_rx_phystat *stat = (struct r12a_rx_phystat *)physt;
	int8_t lna_idx, vga_idx, pwdb;

	lna_idx = (stat->cfosho[0] & 0xe0) >> 5;
	vga_idx = (stat->cfosho[0] & 0x1f);
	pwdb = 6 - 2 * vga_idx;

	switch (lna_idx) {
	case 7:
		if (vga_idx > 27)
			pwdb = -100 + 6;
		else
			pwdb += -100 + 2 * 27;
		break;
	case 6:
		pwdb += -48 + 2 * 2;
		break;
	case 5:
		pwdb += -42 + 2 * 7;
		break;
	case 4:
		pwdb += -36 + 2 * 7;
		break;
	case 3:
		pwdb += -24 + 2 * 7;
		break;
	case 2:
		pwdb += -6 + 2 * 5;
		if (sc->sc_flags & URTWM_FLAG_CCK_HIPWR)
			pwdb -= 6;
		break;
	case 1:
		pwdb += 8;
		break;
	case 0:
		pwdb += 14;
		break;
	default:
		break;
	}

	return (pwdb);		/* XXX PWDB -> RSSI conversion? */
}

static int8_t
urtwm_r21a_get_rssi_cck(struct urtwm_softc *sc, void *physt)
{
	struct r12a_rx_phystat *stat = (struct r12a_rx_phystat *)physt;
	int8_t lna_idx, pwdb;

	lna_idx = (stat->cfosho[0] & 0xe0) >> 5;
	pwdb = -6 - 2*(stat->cfosho[0] & 0x1f);	/* Pout - (2 * VGA_idx) */

	switch (lna_idx) {
	case 5:
		pwdb -= 32;
		break;
	case 4:
		pwdb -= 24;
		break;
	case 2:
		pwdb -= 11;
		break;
	case 1:
		pwdb += 5;
		break;
	case 0:
		pwdb += 21;
		break;
	}

	return (pwdb);		/* XXX PWDB -> RSSI conversion? */
}

static int8_t
urtwm_get_rssi_ofdm(struct urtwm_softc *sc, void *physt)
{
	struct r12a_rx_phystat *stat = (struct r12a_rx_phystat *)physt;
	int i, rssi;

	rssi = 0;
	for (i = 0; i < sc->nrxchains; i++)
		rssi += (stat->gain_trsw[i] & 0x7f) - 110;

	return (rssi / sc->nrxchains);
}

static int8_t
urtwm_get_rssi(struct urtwm_softc *sc, int rate, void *physt)
{
	int8_t rssi;

	if (URTWM_RATE_IS_CCK(rate))
		rssi = urtwm_get_rssi_cck(sc, physt);
	else	/* OFDM/HT. */
		rssi = urtwm_get_rssi_ofdm(sc, physt);

	return (rssi);
}

static void
urtwm_tx_protection(struct urtwm_softc *sc, struct r12a_tx_desc *txd,
    enum ieee80211_protmode mode)
{

	switch (mode) {
	case IEEE80211_PROT_CTSONLY:
		txd->txdw3 |= htole32(R12A_TXDW3_CTS2SELF);
		break;
	case IEEE80211_PROT_RTSCTS:
		txd->txdw3 |= htole32(R12A_TXDW3_RTSEN);
		break;
	default:
		break;
	}

	if (mode == IEEE80211_PROT_CTSONLY ||
	    mode == IEEE80211_PROT_RTSCTS) {
		txd->txdw3 |= htole32(R12A_TXDW3_HWRTSEN);

		/* XXX TODO: rtsrate is configurable? 24mbit may
		 * be a bit high for RTS rate? */
		txd->txdw4 |= htole32(SM(R12A_TXDW4_RTSRATE,
		    URTWM_RIDX_OFDM24));
		/* RTS rate fallback limit (max). */
		txd->txdw4 |= htole32(SM(R12A_TXDW4_RTSRATE_FB_LMT, 0xf));
	}
}

static void
urtwm_tx_raid(struct urtwm_softc *sc, struct r12a_tx_desc *txd,
    struct ieee80211_node *ni, int ismcast)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_channel *c = ic->ic_curchan;
	enum ieee80211_phymode mode;
	uint8_t raid;

	mode = ic->ic_curmode;
	if (mode == IEEE80211_MODE_AUTO)
		mode = ieee80211_chan2mode(c);

	/* NB: group addressed frames are done at 11bg rates for now */
	/*
	 * XXX TODO: this should be per-node, for 11b versus 11bg
	 * nodes in hostap mode
	 */
	if (ismcast || !(ni->ni_flags & IEEE80211_NODE_HT)) {
		switch (mode) {
		case IEEE80211_MODE_11A:
		case IEEE80211_MODE_11B:
		case IEEE80211_MODE_11G:
			break;
		case IEEE80211_MODE_11NA:
			mode = IEEE80211_MODE_11A;
			break;
		case IEEE80211_MODE_11NG:
			mode = IEEE80211_MODE_11G;
			break;
		default:
			device_printf(sc->sc_dev, "unknown mode(1) %d!\n",
			    ic->ic_curmode);
			return;
		}
	}

	switch (mode) {
	case IEEE80211_MODE_11A:
		raid = R12A_RAID_11G;
		break;
	case IEEE80211_MODE_11B:
		raid = R12A_RAID_11B;
		break;
	case IEEE80211_MODE_11G:
		raid = R12A_RAID_11BG;
		break;
	case IEEE80211_MODE_11NA:
		if (sc->ntxchains == 1)
			raid = R12A_RAID_11GN_1;
		else
			raid = R12A_RAID_11GN_2;
		break;
	case IEEE80211_MODE_11NG:
		if (sc->ntxchains == 1) {
			if (IEEE80211_IS_CHAN_HT40(c))
				raid = R12A_RAID_11BGN_1_40;
			else
				raid = R12A_RAID_11BGN_1;
		} else {
			if (IEEE80211_IS_CHAN_HT40(c))
				raid = R12A_RAID_11BGN_2_40;
			else
				raid = R12A_RAID_11BGN_2;
		}
		break;
	default:
		/* TODO: 80 MHz / 11ac */
		device_printf(sc->sc_dev, "unknown mode(2) %d!\n", mode);
		return;
	}

	txd->txdw1 |= htole32(SM(R12A_TXDW1_RAID, raid));
}

static void
urtwm_tx_set_sgi(struct urtwm_softc *sc, struct r12a_tx_desc *txd,
    struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;

	if ((vap->iv_flags_ht & IEEE80211_FHT_SHORTGI20) &&	/* HT20 */
	    (ni->ni_htcap & IEEE80211_HTCAP_SHORTGI20))
		txd->txdw5 |= htole32(R12A_TXDW5_SGI);
	else if (ni->ni_chan != IEEE80211_CHAN_ANYC &&		/* HT40 */
	    IEEE80211_IS_CHAN_HT40(ni->ni_chan) &&
	    (ni->ni_htcap & IEEE80211_HTCAP_SHORTGI40) &&
	    (vap->iv_flags_ht & IEEE80211_FHT_SHORTGI40))
		txd->txdw5 |= htole32(R12A_TXDW5_SGI);
}

static int
urtwm_tx_data(struct urtwm_softc *sc, struct ieee80211_node *ni,
    struct mbuf *m, struct urtwm_data *data)
{
	const struct ieee80211_txparam *tp;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211vap *vap = ni->ni_vap;
	struct ieee80211_key *k = NULL;
	struct ieee80211_channel *chan;
	struct ieee80211_frame *wh;
	struct r12a_tx_desc *txd;
	uint8_t macid, rate, ridx, type, tid, qos, qsel;
	int hasqos, ismcast;

	URTWM_ASSERT_LOCKED(sc);

	wh = mtod(m, struct ieee80211_frame *);
	type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
	hasqos = IEEE80211_QOS_HAS_SEQ(wh);
	ismcast = IEEE80211_IS_MULTICAST(wh->i_addr1);

	/* Select TX ring for this frame. */
	if (hasqos) {
		qos = ((const struct ieee80211_qosframe *)wh)->i_qos[0];
		tid = qos & IEEE80211_QOS_TID;
	} else {
		qos = 0;
		tid = 0;
	}

	chan = (ni->ni_chan != IEEE80211_CHAN_ANYC) ?
		ni->ni_chan : ic->ic_curchan;
	tp = &vap->iv_txparms[ieee80211_chan2mode(chan)];

	/* Choose a TX rate index. */
	if (type == IEEE80211_FC0_TYPE_MGT)
		rate = tp->mgmtrate;
	else if (ismcast)
		rate = tp->mcastrate;
	else if (tp->ucastrate != IEEE80211_FIXED_RATE_NONE)
		rate = tp->ucastrate;
	else if (m->m_flags & M_EAPOL)
		rate = tp->mgmtrate;
	else {
		if (URTWM_USE_RATECTL(sc)) {
			/* XXX pass pktlen */
			(void) ieee80211_ratectl_rate(ni, NULL, 0);
			rate = ni->ni_txrate;
		} else {
			if (ni->ni_flags & IEEE80211_NODE_HT)
				rate = IEEE80211_RATE_MCS | 0x4; /* MCS4 */
			else if (ic->ic_curmode != IEEE80211_MODE_11B)
				rate = ridx2rate[URTWM_RIDX_OFDM36];
			else
				rate = ridx2rate[URTWM_RIDX_CCK55];
		}
	}

	ridx = rate2ridx(rate);

	if (wh->i_fc[1] & IEEE80211_FC1_PROTECTED) {
		k = ieee80211_crypto_encap(ni, m);
		if (k == NULL) {
			device_printf(sc->sc_dev,
			    "ieee80211_crypto_encap returns NULL.\n");
			return (ENOBUFS);
		}

		/* in case packet header moved, reset pointer */
		wh = mtod(m, struct ieee80211_frame *);
	}

	/* Fill Tx descriptor. */
	txd = (struct r12a_tx_desc *)data->buf;
	memset(txd, 0, sizeof(*txd));

	txd->offset = sizeof(*txd);
	txd->flags0 = R12A_FLAGS0_LSG | R12A_FLAGS0_FSG | R12A_FLAGS0_OWN;
	if (ismcast)
		txd->flags0 |= R12A_FLAGS0_BMCAST;

	if (!ismcast) {
		/* Unicast frame, check if an ACK is expected. */
		if (!qos || (qos & IEEE80211_QOS_ACKPOLICY) !=
		    IEEE80211_QOS_ACKPOLICY_NOACK) {
			txd->txdw4 = htole32(R12A_TXDW4_RETRY_LMT_ENA);
			txd->txdw4 |= htole32(SM(R12A_TXDW4_RETRY_LMT,
			    tp->maxretry));
		}

		struct urtwm_node *un = URTWM_NODE(ni);
		macid = un->id;

		if (type == IEEE80211_FC0_TYPE_DATA) {
			qsel = tid % URTWM_MAX_TID;

			if (m->m_flags & M_AMPDU_MPDU) {
				txd->txdw2 |= htole32(R12A_TXDW2_AGGEN);
				txd->txdw2 |= htole32(SM(R12A_TXDW2_AMPDU_DEN,
				    vap->iv_ampdu_density));
				txd->txdw3 |= htole32(SM(R12A_TXDW3_MAX_AGG,
				    0x1f));	/* XXX */
			} else
				txd->txdw2 |= htole32(R12A_TXDW2_AGGBK);

			txd->txdw2 |= htole32(R12A_TXDW2_SPE_RPT);
			if (sc->sc_flags & URTWM_FW_LOADED)
				sc->sc_tx_n_active++;

			if (ridx >= URTWM_RIDX_MCS(0))
				urtwm_tx_set_sgi(sc, txd, ni);

			if (rate & IEEE80211_RATE_MCS) {
				urtwm_tx_protection(sc, txd,
				    ic->ic_htprotmode);
			} else if (ic->ic_flags & IEEE80211_F_USEPROT)
				urtwm_tx_protection(sc, txd, ic->ic_protmode);

			/* Data rate fallback limit (max). */
			txd->txdw4 |= htole32(SM(R12A_TXDW4_DATARATE_FB_LMT,
			    0x1f));
		} else	/* IEEE80211_FC0_TYPE_MGT */
			qsel = R12A_TXDW1_QSEL_MGNT;
	} else {
		macid = URTWM_MACID_BC;
		qsel = R12A_TXDW1_QSEL_MGNT;
	}

	txd->txdw1 |= htole32(SM(R12A_TXDW1_QSEL, qsel));

	/* XXX TODO: 40MHZ flag? */
	/* XXX Short preamble? */

	txd->txdw1 |= htole32(SM(R12A_TXDW1_MACID, macid));
	txd->txdw4 |= htole32(SM(R12A_TXDW4_DATARATE, ridx));
	urtwm_tx_raid(sc, txd, ni, ismcast);

	/* Force this rate if needed. */
	if (URTWM_USE_RATECTL(sc) || ismcast ||
	    (tp->ucastrate != IEEE80211_FIXED_RATE_NONE) ||
	    (m->m_flags & M_EAPOL) || type != IEEE80211_FC0_TYPE_DATA)
		txd->txdw3 |= htole32(R12A_TXDW3_DRVRATE);

	if (!hasqos) {
		/* Use HW sequence numbering for non-QoS frames. */
		txd->txdw8 |= htole32(R12A_TXDW8_HWSEQ_EN);
	} else {
		uint16_t seqno;

		if (m->m_flags & M_AMPDU_MPDU) {
			seqno = ni->ni_txseqs[tid];
			/* NB: clear Fragment Number field. */
			*(uint16_t *)wh->i_seq = 0;
			ni->ni_txseqs[tid]++;
		} else
			seqno = M_SEQNO_GET(m) % IEEE80211_SEQ_RANGE;

		/* Set sequence number. */
		txd->txdw9 |= htole32(SM(R12A_TXDW9_SEQ, seqno));
	}

	if (k != NULL && !(k->wk_flags & IEEE80211_KEY_SWCRYPT)) {
		uint8_t cipher;

		switch (k->wk_cipher->ic_cipher) {
		case IEEE80211_CIPHER_WEP:
		case IEEE80211_CIPHER_TKIP:
			cipher = R12A_TXDW1_CIPHER_RC4;
			break;
		case IEEE80211_CIPHER_AES_CCM:
			cipher = R12A_TXDW1_CIPHER_AES;
			break;
		default:
			device_printf(sc->sc_dev, "%s: unknown cipher %d\n",
			    __func__, k->wk_cipher->ic_cipher);
			return (EINVAL);
		}

		txd->txdw1 |= htole32(SM(R12A_TXDW1_CIPHER, cipher));
	}

	if (ieee80211_radiotap_active_vap(vap)) {
		struct urtwm_tx_radiotap_header *tap = &sc->sc_txtap;

		tap->wt_flags = 0;
		if (k != NULL)
			tap->wt_flags |= IEEE80211_RADIOTAP_F_WEP;
		ieee80211_radiotap_tx(vap, m);
	}

	data->ni = ni;

	urtwm_tx_start(sc, m, type, data);

	return (0);
}

static int
urtwm_tx_raw(struct urtwm_softc *sc, struct ieee80211_node *ni,
    struct mbuf *m, struct urtwm_data *data,
    const struct ieee80211_bpf_params *params)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ieee80211_key *k = NULL;
	struct ieee80211_frame *wh;
	struct r12a_tx_desc *txd;
	uint8_t cipher, ridx, type;
	int ismcast;

	/* Encrypt the frame if need be. */
	cipher = R12A_TXDW1_CIPHER_NONE;
	if (params->ibp_flags & IEEE80211_BPF_CRYPTO) {
		/* Retrieve key for TX. */
		k = ieee80211_crypto_encap(ni, m);
		if (k == NULL)
			return (ENOBUFS);

		if (!(k->wk_flags & IEEE80211_KEY_SWCRYPT)) {
			switch (k->wk_cipher->ic_cipher) {
			case IEEE80211_CIPHER_WEP:
			case IEEE80211_CIPHER_TKIP:
				cipher = R12A_TXDW1_CIPHER_RC4;
				break;
			case IEEE80211_CIPHER_AES_CCM:
				cipher = R12A_TXDW1_CIPHER_AES;
				break;
			default:
				device_printf(sc->sc_dev,
				    "%s: unknown cipher %d\n",
				    __func__, k->wk_cipher->ic_cipher);
				return (EINVAL);
			}
		}
	}

	/* XXX TODO: 11n checks, matching urtwm_tx_data() */

	wh = mtod(m, struct ieee80211_frame *);
	type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
	ismcast = IEEE80211_IS_MULTICAST(wh->i_addr1);

	/* Fill Tx descriptor. */
	txd = (struct r12a_tx_desc *)data->buf;
	memset(txd, 0, sizeof(*txd));

	txd->offset = sizeof(*txd);
	txd->flags0 |= R12A_FLAGS0_LSG | R12A_FLAGS0_FSG | R12A_FLAGS0_OWN;
	if (ismcast)
		txd->flags0 |= R12A_FLAGS0_BMCAST;

	if ((params->ibp_flags & IEEE80211_BPF_NOACK) == 0) {
		txd->txdw4 = htole32(R12A_TXDW4_RETRY_LMT_ENA);
		txd->txdw4 |= htole32(SM(R12A_TXDW4_RETRY_LMT,
		    params->ibp_try0));
	}
	if (params->ibp_flags & IEEE80211_BPF_RTS)
		urtwm_tx_protection(sc, txd, IEEE80211_PROT_RTSCTS);
	if (params->ibp_flags & IEEE80211_BPF_CTS)
		urtwm_tx_protection(sc, txd, IEEE80211_PROT_CTSONLY);

	txd->txdw1 |= htole32(SM(R12A_TXDW1_MACID, URTWM_MACID_BC));
	txd->txdw1 |= htole32(SM(R12A_TXDW1_QSEL, R12A_TXDW1_QSEL_MGNT));
	txd->txdw1 |= htole32(SM(R12A_TXDW1_CIPHER, cipher));

	/* Choose a TX rate index. */
	ridx = rate2ridx(params->ibp_rate0);
	txd->txdw4 |= htole32(SM(R12A_TXDW4_DATARATE, ridx));
	txd->txdw4 |= htole32(SM(R12A_TXDW4_DATARATE_FB_LMT, 0x1f));
	txd->txdw3 |= htole32(R12A_TXDW3_DRVRATE);
	urtwm_tx_raid(sc, txd, ni, ismcast);

	if (!IEEE80211_QOS_HAS_SEQ(wh)) {
		/* Use HW sequence numbering for non-QoS frames. */
		txd->txdw8 |= htole32(R12A_TXDW8_HWSEQ_EN);
	} else {
		/* Set sequence number. */
		txd->txdw9 |= htole32(SM(R12A_TXDW9_SEQ,
		    M_SEQNO_GET(m) % IEEE80211_SEQ_RANGE));
	}

	if (ieee80211_radiotap_active_vap(vap)) {
		struct urtwm_tx_radiotap_header *tap = &sc->sc_txtap;

		tap->wt_flags = 0;
		if (k != NULL)
			tap->wt_flags |= IEEE80211_RADIOTAP_F_WEP;
		ieee80211_radiotap_tx(vap, m);
	}

	data->ni = ni;

	urtwm_tx_start(sc, m, type, data);

	return (0);
}

static void
urtwm_tx_start(struct urtwm_softc *sc, struct mbuf *m, uint8_t type,
    struct urtwm_data *data)
{
	struct usb_xfer *xfer;
	struct r12a_tx_desc *txd;
	uint16_t ac;
	int xferlen;

	URTWM_ASSERT_LOCKED(sc);

	ac = M_WME_GETAC(m);

	switch (type) {
	case IEEE80211_FC0_TYPE_CTL:
	case IEEE80211_FC0_TYPE_MGT:
		xfer = sc->sc_xfer[URTWM_BULK_TX_VO];
		break;
	default:
		xfer = sc->sc_xfer[wme2queue[ac].qid];
		break;
	}

	txd = (struct r12a_tx_desc *)data->buf;
	txd->pktlen = htole16(m->m_pkthdr.len);

	/* Compute Tx descriptor checksum. */
	urtwm_tx_checksum(txd);

	xferlen = sizeof(*txd) + m->m_pkthdr.len;
	m_copydata(m, 0, m->m_pkthdr.len, (caddr_t)&txd[1]);

	data->buflen = xferlen;
	if (data->ni != NULL)
		data->m = m;

	STAILQ_INSERT_TAIL(&sc->sc_tx_pending, data, next);
	usbd_transfer_start(xfer);
}

static void
urtwm_tx_checksum(struct r12a_tx_desc *txd)
{
	uint16_t sum = 0;
	int i;

	/* NB: checksum calculation takes into account only first 32 bytes. */
	for (i = 0; i < 32 / 2; i++)
		sum ^= ((uint16_t *)txd)[i];
	txd->txdsum = sum;	/* NB: already little endian. */
}

static int
urtwm_transmit(struct ieee80211com *ic, struct mbuf *m)
{
	struct urtwm_softc *sc = ic->ic_softc;
	int error;

	URTWM_LOCK(sc);
	if ((sc->sc_flags & URTWM_RUNNING) == 0) {
		URTWM_UNLOCK(sc);
		return (ENXIO);
	}
	error = mbufq_enqueue(&sc->sc_snd, m);
	if (error) {
		URTWM_UNLOCK(sc);
		return (error);
	}
	urtwm_start(sc);
	URTWM_UNLOCK(sc);

	return (0);
}

static void
urtwm_start(struct urtwm_softc *sc)
{
	struct ieee80211_node *ni;
	struct mbuf *m;
	struct urtwm_data *bf;

	URTWM_ASSERT_LOCKED(sc);
	while ((m = mbufq_dequeue(&sc->sc_snd)) != NULL) {
		bf = urtwm_getbuf(sc);
		if (bf == NULL) {
			mbufq_prepend(&sc->sc_snd, m);
			break;
		}
		ni = (struct ieee80211_node *)m->m_pkthdr.rcvif;
		m->m_pkthdr.rcvif = NULL;

		URTWM_DPRINTF(sc, URTWM_DEBUG_XMIT,
		    "%s: called; m %p, ni %p\n", __func__, m, ni);

		if (urtwm_tx_data(sc, ni, m, bf) != 0) {
			if_inc_counter(ni->ni_vap->iv_ifp,
			    IFCOUNTER_OERRORS, 1);
			STAILQ_INSERT_HEAD(&sc->sc_tx_inactive, bf, next);
			m_freem(m);
#ifdef D4054
			ieee80211_tx_watchdog_refresh(ni->ni_ic, -1, 0);
#endif
			ieee80211_free_node(ni);
			break;
		}
	}
}

static void
urtwm_parent(struct ieee80211com *ic)
{
	struct urtwm_softc *sc = ic->ic_softc;

	URTWM_LOCK(sc);
	if (sc->sc_flags & URTWM_DETACHED) {
		URTWM_UNLOCK(sc);
		return;
	}
	URTWM_UNLOCK(sc);

	if (ic->ic_nrunning > 0) {
		if (urtwm_init(sc) != 0) {
			struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);
			if (vap != NULL)
				ieee80211_stop(vap);
		} else
			ieee80211_start_all(ic);
	} else
		urtwm_stop(sc);
}

static int
urtwm_ioctl_net(struct ieee80211com *ic, u_long cmd, void *data)
{
	struct urtwm_softc *sc = ic->ic_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int error;

	error = 0;
	switch (cmd) {
	case SIOCSIFCAP:
	{
		struct ieee80211vap *vap;
		int changed, rxmask;

		rxmask = ifr->ifr_reqcap & (IFCAP_RXCSUM | IFCAP_RXCSUM_IPV6);

		URTWM_LOCK(sc);
		changed = 0;
		if (!(sc->sc_flags & URTWM_RXCKSUM_EN) ^
		    !(ifr->ifr_reqcap & IFCAP_RXCSUM)) {
			sc->sc_flags ^= URTWM_RXCKSUM_EN;
			changed = 1;
		}
		if (!(sc->sc_flags & URTWM_RXCKSUM6_EN) ^
		    !(ifr->ifr_reqcap & IFCAP_RXCSUM_IPV6)) {
			sc->sc_flags ^= URTWM_RXCKSUM6_EN;
			changed = 1;
		}
		if (changed) {
			uint32_t rcr;

			rcr = urtwm_read_4(sc, R92C_RCR);
			if (rxmask == 0)
				rcr &= ~R12A_RCR_TCP_OFFLD_EN;
			else
				rcr |= R12A_RCR_TCP_OFFLD_EN;
			urtwm_write_4(sc, R92C_RCR, rcr);
		}
		URTWM_UNLOCK(sc);

		IEEE80211_LOCK(ic);	/* XXX */
		TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
			struct ifnet *ifp = vap->iv_ifp;

			ifp->if_capenable &=
			    ~(IFCAP_RXCSUM | IFCAP_RXCSUM_IPV6);
			ifp->if_capenable |= rxmask;
		}
		IEEE80211_UNLOCK(ic);
		break;
	}
	default:
		error = ENOTTY;		/* for net80211 */
		break;
	}

	return (error);
}

static int
urtwm_r12a_power_on(struct urtwm_softc *sc)
{
#define URTWM_CHK(res) do {			\
	if (res != USB_ERR_NORMAL_COMPLETION)	\
		return (EIO);			\
} while(0)
	int ntries;

	/* Force PWM mode. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_SPS0_CTRL + 1, 0, 0x01));

	/* Turn off ZCD. */
	URTWM_CHK(urtwm_setbits_2(sc, 0x014, 0x0180, 0));

	/* Enable LDO normal mode. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_LPLDO_CTRL, R92C_LPLDO_CTRL_SLEEP,
	    0));

	/* GPIO 0...7 input mode. */
	URTWM_CHK(urtwm_write_1(sc, R92C_GPIO_IOSEL, 0));

	/* GPIO 11...8 input mode. */
	URTWM_CHK(urtwm_write_1(sc, R92C_MAC_PINMUX_CFG, 0));

	/* Enable WL suspend. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_AFSM_HSUS, 0, 1));

	/* Enable 8051. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_SYS_FUNC_EN,
	    0, R92C_SYS_FUNC_EN_CPUEN, 1));

	/* Disable SW LPS. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_APFM_RSM, 0, 1));

	/* Wait for power ready bit. */
	for (ntries = 0; ntries < 5000; ntries++) {
		if (urtwm_read_4(sc, R92C_APS_FSMCO) & R92C_APS_FSMCO_SUS_HOST)
			break;
		urtwm_delay(sc, 10);
	}
	if (ntries == 5000) {
		device_printf(sc->sc_dev,
		    "timeout waiting for chip power up\n");
		return (ETIMEDOUT);
	}

	/* Disable WL suspend. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_AFSM_HSUS, 0, 1));

	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0,
	    R92C_APS_FSMCO_APFM_ONMAC, 1));
	for (ntries = 0; ntries < 5000; ntries++) {
		if (!(urtwm_read_2(sc, R92C_APS_FSMCO) &
		    R92C_APS_FSMCO_APFM_ONMAC))
			break;
		urtwm_delay(sc, 10);
	}
	if (ntries == 5000)
		return (ETIMEDOUT);

	/* Enable MAC DMA/WMAC/SCHEDULE/SEC blocks. */
	URTWM_CHK(urtwm_write_2(sc, R92C_CR, 0x0000));
	URTWM_CHK(urtwm_setbits_2(sc, R92C_CR, 0,
	    R92C_CR_HCI_TXDMA_EN | R92C_CR_TXDMA_EN |
	    R92C_CR_HCI_RXDMA_EN | R92C_CR_RXDMA_EN |
	    R92C_CR_PROTOCOL_EN | R92C_CR_SCHEDULE_EN |
	    R92C_CR_ENSEC | R92C_CR_CALTMR_EN));

	return (0);
}

static int
urtwm_r21a_power_on(struct urtwm_softc *sc)
{
	int ntries;

	/* Clear suspend and power down bits.*/
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_AFSM_HSUS | R92C_APS_FSMCO_APDM_HPDN, 0, 1));

	/* Disable GPIO9 as EXT WAKEUP. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_GPIO_INTM + 2, 0x01, 0));

	/* Enable WL suspend. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_AFSM_HSUS | R92C_APS_FSMCO_AFSM_PCIE, 0, 1));

	/* Enable LDOA12 MACRO block for all interfaces. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_LDOA15_CTRL, 0, R92C_LDOA15_CTRL_EN));

	/* Disable BT_GPS_SEL pins. */
	URTWM_CHK(urtwm_setbits_1(sc, 0x067, 0x10, 0));

	/* 1 ms delay. */
	urtwm_delay(sc, 1000);

	/* Release analog Ips to digital isolation. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_SYS_ISO_CTRL,
	    R92C_SYS_ISO_CTRL_IP2MAC, 0));

	/* Disable SW LPS and WL suspend. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_APFM_RSM |
	    R92C_APS_FSMCO_AFSM_HSUS |
	    R92C_APS_FSMCO_AFSM_PCIE, 0, 1));

	/* Wait for power ready bit. */
	for (ntries = 0; ntries < 5000; ntries++) {
		if (urtwm_read_4(sc, R92C_APS_FSMCO) & R92C_APS_FSMCO_SUS_HOST)
			break;
		urtwm_delay(sc, 10);
	}
	if (ntries == 5000) {
		device_printf(sc->sc_dev,
		    "timeout waiting for chip power up\n");
		return (ETIMEDOUT);
	}

	/* Release WLON reset. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0,
	    R92C_APS_FSMCO_RDY_MACON, 2));

	/* Disable HWPDN. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_APDM_HPDN, 0, 1));

	/* Disable WL suspend. */
	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO,
	    R92C_APS_FSMCO_AFSM_HSUS | R92C_APS_FSMCO_AFSM_PCIE, 0, 1));

	URTWM_CHK(urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0,
	    R92C_APS_FSMCO_APFM_ONMAC, 1));
	for (ntries = 0; ntries < 5000; ntries++) {
		if (!(urtwm_read_2(sc, R92C_APS_FSMCO) &
		    R92C_APS_FSMCO_APFM_ONMAC))
			break;
		urtwm_delay(sc, 10);
	}
	if (ntries == 5000)
		return (ETIMEDOUT);

	/* Switch DPDT_SEL_P output from WL BB. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_LEDCFG3, 0, 0x01));

	/* switch for PAPE_G/PAPE_A from WL BB; switch LNAON from WL BB. */
	URTWM_CHK(urtwm_setbits_1(sc, 0x067, 0, 0x30));

	URTWM_CHK(urtwm_setbits_1(sc, 0x025, 0x40, 0));

	/* Enable falling edge triggering interrupt. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_GPIO_INTM + 1, 0, 0x02));

	/* Enable GPIO9 interrupt mode. */
	URTWM_CHK(urtwm_setbits_1(sc, 0x063, 0, 0x02));

	/* Enable GPIO9 input mode. */
	URTWM_CHK(urtwm_setbits_1(sc, 0x062, 0x02, 0));

	/* Enable HSISR GPIO interrupt. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_HSIMR, 0, 0x01));

	/* Enable HSISR GPIO9 interrupt. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_HSIMR + 2, 0, 0x02));

	/* XTAL trim. */
	URTWM_CHK(urtwm_setbits_1(sc, R92C_APE_PLL_CTRL_EXT + 2, 0xFF, 0x82));

	URTWM_CHK(urtwm_setbits_1(sc, R92C_AFE_MISC, 0, 0x40));

	/* Enable MAC DMA/WMAC/SCHEDULE/SEC blocks. */
	URTWM_CHK(urtwm_write_2(sc, R92C_CR, 0x0000));
	URTWM_CHK(urtwm_setbits_2(sc, R92C_CR, 0,
	    R92C_CR_HCI_TXDMA_EN | R92C_CR_TXDMA_EN |
	    R92C_CR_HCI_RXDMA_EN | R92C_CR_RXDMA_EN |
	    R92C_CR_PROTOCOL_EN | R92C_CR_SCHEDULE_EN |
	    R92C_CR_ENSEC | R92C_CR_CALTMR_EN));

	if (urtwm_read_4(sc, R92C_SYS_CFG) & R92C_SYS_CFG_TRP_BT_EN)
		URTWM_CHK(urtwm_setbits_1(sc, 0x07C, 0, 0x40));

	return (0);
#undef URTWM_CHK
}

static void
urtwm_r12a_power_off(struct urtwm_softc *sc)
{
	int ntries;

	/* Stop Rx. */
	urtwm_write_1(sc, R92C_CR, 0);

	/* Move card to Low Power state. */
	/* Block all Tx queues. */
	urtwm_write_1(sc, R92C_TXPAUSE, R92C_TX_QUEUE_ALL);

	for (ntries = 0; ntries < 5000; ntries++) {
		/* Should be zero if no packet is transmitting. */
		if (urtwm_read_4(sc, R88E_SCH_TXCMD) == 0)
			break;

		urtwm_delay(sc, 10);
	}
	if (ntries == 5000) {
		device_printf(sc->sc_dev, "%s: failed to block Tx queues\n",
		    __func__);
		return;
	}

	/* Turn off 3-wire. */
	urtwm_write_1(sc, R12A_HSSI_PARAM1(0), 0x04);
	urtwm_write_1(sc, R12A_HSSI_PARAM1(1), 0x04);

	/* CCK and OFDM are disabled, and clock are gated. */
	urtwm_setbits_1(sc, R92C_SYS_FUNC_EN, R92C_SYS_FUNC_EN_BBRSTB, 0);

	urtwm_delay(sc, 1);

	/* Reset whole BB. */
	urtwm_setbits_1(sc, R92C_SYS_FUNC_EN, R92C_SYS_FUNC_EN_BB_GLB_RST, 0);

	/* Reset MAC TRX. */
	urtwm_write_1(sc, R92C_CR,
	    R92C_CR_HCI_TXDMA_EN | R92C_CR_HCI_RXDMA_EN);

	/* check if removed later. (?) */
	urtwm_setbits_1_shift(sc, R92C_CR, R92C_CR_ENSEC, 0, 1);

	/* Respond TxOK to scheduler */
	urtwm_setbits_1(sc, R92C_DUAL_TSF_RST, 0, R92C_DUAL_TSF_RST_TXOK);

	/* If firmware in ram code, do reset. */
#ifndef URTWM_WITHOUT_UCODE
	if (urtwm_read_1(sc, R92C_MCUFWDL) & R92C_MCUFWDL_RAM_DL_SEL)
		urtwm_fw_reset(sc);
#endif

	/* Reset MCU. */
	urtwm_setbits_1_shift(sc, R92C_SYS_FUNC_EN, R92C_SYS_FUNC_EN_CPUEN,
	    0, 1);
	urtwm_write_1(sc, R92C_MCUFWDL, 0);

	/* Move card to Disabled state. */
	/* Turn off 3-wire. */
	urtwm_write_1(sc, R12A_HSSI_PARAM1(0), 0x04);
	urtwm_write_1(sc, R12A_HSSI_PARAM1(1), 0x04);

	/* Reset BB, close RF. */
	urtwm_setbits_1(sc, R92C_SYS_FUNC_EN, R92C_SYS_FUNC_EN_BB_GLB_RST, 0);

	urtwm_delay(sc, 1);

	/* SPS PWM mode. */
	urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0xff,
	    R92C_APS_FSMCO_SOP_RCK | R92C_APS_FSMCO_SOP_ABG, 3);

	/* ANA clock = 500k. */
	urtwm_setbits_1(sc, R92C_SYS_CLKR, R92C_SYS_CLKR_ANA8M, 0);

	/* Turn off MAC by HW state machine */
	urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0, R92C_APS_FSMCO_APFM_OFF,
	    1);
	for (ntries = 0; ntries < 5000; ntries++) {
		/* Wait until it will be disabled. */
		if ((urtwm_read_2(sc, R92C_APS_FSMCO) &
		    R92C_APS_FSMCO_APFM_OFF) == 0)
			break;

		urtwm_delay(sc, 10);
	}
	if (ntries == 5000) {
		device_printf(sc->sc_dev, "%s: could not turn off MAC\n",
		    __func__);
		return;
	}

	/* Reset 8051. */
	urtwm_setbits_1_shift(sc, R92C_SYS_FUNC_EN, R92C_SYS_FUNC_EN_CPUEN,
	    0, 1);

	/* Fill the default value of host_CPU handshake field. */
	urtwm_write_1(sc, R92C_MCUFWDL,
	    R92C_MCUFWDL_EN | R92C_MCUFWDL_CHKSUM_RPT);

	urtwm_setbits_1(sc, R92C_GPIO_IO_SEL, 0xf0, 0xc0);

	/* GPIO 11 input mode, 10...8 output mode. */
	urtwm_write_1(sc, R92C_MAC_PINMUX_CFG, 0x07);

	/* GPIO 7...0, output = input */
	urtwm_write_1(sc, R92C_GPIO_OUT, 0);

	/* GPIO 7...0 output mode. */
	urtwm_write_1(sc, R92C_GPIO_IOSEL, 0xff);

	urtwm_write_1(sc, R92C_GPIO_MOD, 0);

	/* Turn on ZCD. */
	urtwm_setbits_2(sc, 0x014, 0, 0x0180);

	/* Force PFM mode. */
	urtwm_setbits_1(sc, R92C_SPS0_CTRL + 1, 0x01, 0);

	/* LDO sleep mode. */
	urtwm_setbits_1(sc, R92C_LPLDO_CTRL, 0, R92C_LPLDO_CTRL_SLEEP);

	/* ANA clock = 500k. */
	urtwm_setbits_1(sc, R92C_SYS_CLKR, R92C_SYS_CLKR_ANA8M, 0);

	/* SOP option to disable BG/MB. */
	urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0xff,
	    R92C_APS_FSMCO_SOP_RCK, 3);

	/* Disable RFC_0. */
	urtwm_setbits_1(sc, R92C_RF_CTRL, R92C_RF_CTRL_RSTB, 0);

	/* Disable RFC_1. */
	urtwm_setbits_1(sc, R12A_RF_B_CTRL, R92C_RF_CTRL_RSTB, 0);

	/* Enable WL suspend. */
	urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0, R92C_APS_FSMCO_AFSM_HSUS,
	    1);
}

static void
urtwm_r21a_power_off(struct urtwm_softc *sc)
{
	int ntries;

	/* Stop Rx. */
	urtwm_write_1(sc, R92C_CR, 0);

	/* Move card to Low Power state. */
	/* Block all Tx queues. */
	urtwm_write_1(sc, R92C_TXPAUSE, R92C_TX_QUEUE_ALL);

	for (ntries = 0; ntries < 5000; ntries++) {
		/* Should be zero if no packet is transmitting. */
		if (urtwm_read_4(sc, R88E_SCH_TXCMD) == 0)
			break;

		urtwm_delay(sc, 10);
	}
	if (ntries == 5000) {
		device_printf(sc->sc_dev, "%s: failed to block Tx queues\n",
		    __func__);
		return;
	}

	/* CCK and OFDM are disabled, and clock are gated. */
	urtwm_setbits_1(sc, R92C_SYS_FUNC_EN, R92C_SYS_FUNC_EN_BBRSTB, 0);

	urtwm_delay(sc, 1);

	/* Reset whole BB. */
	urtwm_setbits_1(sc, R92C_SYS_FUNC_EN, R92C_SYS_FUNC_EN_BB_GLB_RST, 0);

	/* Reset MAC TRX. */
	urtwm_write_1(sc, R92C_CR,
	    R92C_CR_HCI_TXDMA_EN | R92C_CR_HCI_RXDMA_EN);

	/* check if removed later. (?) */
	urtwm_setbits_1_shift(sc, R92C_CR, R92C_CR_ENSEC, 0, 1);

	/* Respond TxOK to scheduler */
	urtwm_setbits_1(sc, R92C_DUAL_TSF_RST, 0, R92C_DUAL_TSF_RST_TXOK);

	/* If firmware in ram code, do reset. */
#ifndef URTWM_WITHOUT_UCODE
	if (urtwm_read_1(sc, R92C_MCUFWDL) & R92C_MCUFWDL_RAM_DL_SEL)
		urtwm_fw_reset(sc);
#endif

	/* Reset MCU. */
	urtwm_setbits_1_shift(sc, R92C_SYS_FUNC_EN, R92C_SYS_FUNC_EN_CPUEN,
	    0, 1);
	urtwm_write_1(sc, R92C_MCUFWDL, 0);

	/* Move card to Disabled state. */
	/* Turn off RF. */
	urtwm_write_1(sc, R92C_RF_CTRL, 0);

	urtwm_setbits_1(sc, R92C_LEDCFG3, 0x01, 0);

	/* Enable rising edge triggering interrupt. */
	urtwm_setbits_1(sc, R92C_GPIO_INTM + 1, 0x02, 0);

	/* Release WLON reset. */
	urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0,
	    R92C_APS_FSMCO_RDY_MACON, 2);

	/* Turn off MAC by HW state machine */
	urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, 0, R92C_APS_FSMCO_APFM_OFF,
	    1);
	for (ntries = 0; ntries < 5000; ntries++) {
		/* Wait until it will be disabled. */
		if ((urtwm_read_2(sc, R92C_APS_FSMCO) &
		    R92C_APS_FSMCO_APFM_OFF) == 0)
			break;

		urtwm_delay(sc, 10);
	}
	if (ntries == 5000) {
		device_printf(sc->sc_dev, "%s: could not turn off MAC\n",
		    __func__);
		return;
	}

	/* Analog Ips to digital isolation. */
        urtwm_setbits_1(sc, R92C_SYS_ISO_CTRL, 0, R92C_SYS_ISO_CTRL_IP2MAC);

	/* Disable LDOA12 MACRO block. */
	urtwm_setbits_1(sc, R92C_LDOA15_CTRL, R92C_LDOA15_CTRL_EN, 0);

	/* Enable WL suspend. */
	urtwm_setbits_1_shift(sc, R92C_APS_FSMCO, R92C_APS_FSMCO_AFSM_PCIE,
	    R92C_APS_FSMCO_AFSM_HSUS, 1);

	/* Enable GPIO9 as EXT WAKEUP. */
	urtwm_setbits_1(sc, R92C_GPIO_INTM + 2, 0, 0x01);
}

static int
urtwm_llt_init(struct urtwm_softc *sc)
{
	int i, error;

	/* Reserve pages [0; page_count]. */
	for (i = 0; i < sc->page_count; i++) {
		if ((error = urtwm_llt_write(sc, i, i + 1)) != 0)
			return (error);
	}
	/* NB: 0xff indicates end-of-list. */
	if ((error = urtwm_llt_write(sc, i, 0xff)) != 0)
		return (error);
	/*
	 * Use pages [page_count + 1; pktbuf_count - 1]
	 * as ring buffer.
	 */
	for (++i; i < sc->pktbuf_count - 1; i++) {
		if ((error = urtwm_llt_write(sc, i, i + 1)) != 0)
			return (error);
	}
	/* Make the last page point to the beginning of the ring buffer. */
	error = urtwm_llt_write(sc, i, sc->page_count + 1);
	return (error);
}

#ifndef URTWM_WITHOUT_UCODE
static void
urtwm_r12a_fw_reset(struct urtwm_softc *sc)
{
	/* Reset MCU IO wrapper. */
	urtwm_setbits_1(sc, R92C_RSV_CTRL, 0x02, 0);
	urtwm_setbits_1(sc, R92C_RSV_CTRL + 1, 0x08, 0);

	urtwm_setbits_1_shift(sc, R92C_SYS_FUNC_EN,
	    R92C_SYS_FUNC_EN_CPUEN, 0, 1);

	/* Enable MCU IO wrapper. */
	urtwm_setbits_1(sc, R92C_RSV_CTRL, 0x02, 0);
	urtwm_setbits_1(sc, R92C_RSV_CTRL + 1, 0, 0x08);

	urtwm_setbits_1_shift(sc, R92C_SYS_FUNC_EN,
	    0, R92C_SYS_FUNC_EN_CPUEN, 1);
}

static void
urtwm_r21a_fw_reset(struct urtwm_softc *sc)
{

	/* Reset MCU IO wrapper. */
	urtwm_setbits_1(sc, R92C_RSV_CTRL, 0x02, 0);
	urtwm_setbits_1(sc, R92C_RSV_CTRL + 1, 0x01, 0);

	urtwm_setbits_1_shift(sc, R92C_SYS_FUNC_EN,
	    R92C_SYS_FUNC_EN_CPUEN, 0, 1);

	/* Enable MCU IO wrapper. */
	urtwm_setbits_1(sc, R92C_RSV_CTRL, 0x02, 0);
	urtwm_setbits_1(sc, R92C_RSV_CTRL + 1, 0, 0x01);

	urtwm_setbits_1_shift(sc, R92C_SYS_FUNC_EN,
	    0, R92C_SYS_FUNC_EN_CPUEN, 1);
}

static usb_error_t
urtwm_fw_loadpage(struct urtwm_softc *sc, int page, const uint8_t *buf, int len)
{
	uint32_t reg;
	usb_error_t error = USB_ERR_NORMAL_COMPLETION;
	int off, mlen;

	reg = urtwm_read_4(sc, R92C_MCUFWDL);
	reg = RW(reg, R92C_MCUFWDL_PAGE, page);
	urtwm_write_4(sc, R92C_MCUFWDL, reg);

	off = R92C_FW_START_ADDR;
	while (len > 0) {
		if (len > R92C_FW_MAX_BLOCK_SIZE_USB)
			mlen = R92C_FW_MAX_BLOCK_SIZE_USB;
		else if (len > 4)
			mlen = 4;
		else
			mlen = 1;
		/* XXX fix this deconst */
		error = urtwm_write_region_1(sc, off,
		    __DECONST(uint8_t *, buf), mlen);
		if (error != USB_ERR_NORMAL_COMPLETION)
			break;
		off += mlen;
		buf += mlen;
		len -= mlen;
	}
	return (error);
}

static int
urtwm_fw_checksum_report(struct urtwm_softc *sc)
{
	int ntries;

	for (ntries = 0; ntries < 25; ntries++) {
		if (urtwm_read_4(sc, R92C_MCUFWDL) & R92C_MCUFWDL_CHKSUM_RPT)
			break;
		urtwm_delay(sc, 10000);
	}
	if (ntries == 25) {
		URTWM_DPRINTF(sc, URTWM_DEBUG_FIRMWARE,
		    "timeout waiting for checksum report\n");
		return (ETIMEDOUT);
	}

	return (0);
}

static int
urtwm_load_firmware(struct urtwm_softc *sc)
{
	const struct firmware *fw;
	const struct r92c_fw_hdr *hdr;
	const u_char *ptr, *ptr2;
	size_t len, len2;
	int mlen, ntries, page, error;

	/* Read firmware image from the filesystem. */
	URTWM_UNLOCK(sc);
	fw = firmware_get(sc->fwname);
	URTWM_LOCK(sc);
	if (fw == NULL) {
		device_printf(sc->sc_dev,
		    "failed loadfirmware of file %s\n", sc->fwname);
		return (ENOENT);
	}

	len = fw->datasize;
	if (len < sizeof(*hdr) || len > R12A_MAX_FW_SIZE) {
		device_printf(sc->sc_dev, "wrong firmware size (%d)\n", len);
		error = EINVAL;
		goto fail;
	}
	ptr = fw->data;
	hdr = (const struct r92c_fw_hdr *)ptr;
	/* Check if there is a valid FW header and skip it. */
	if ((le16toh(hdr->signature) >> 4) == sc->fwsig) {
		sc->fwver = le16toh(hdr->version);

		URTWM_DPRINTF(sc, URTWM_DEBUG_FIRMWARE,
		    "FW V%d.%d %02d-%02d %02d:%02d\n",
		    le16toh(hdr->version), le16toh(hdr->subversion),
		    hdr->month, hdr->date, hdr->hour, hdr->minute);
		ptr += sizeof(*hdr);
		len -= sizeof(*hdr);
	}

	if (urtwm_read_1(sc, R92C_MCUFWDL) & R92C_MCUFWDL_RAM_DL_SEL) {
		urtwm_write_1(sc, R92C_MCUFWDL, 0);
		urtwm_fw_reset(sc);
	}

	/* MCU firmware download enable. */
	urtwm_setbits_1(sc, R92C_MCUFWDL, 0, R92C_MCUFWDL_EN);
	/* 8051 reset. */
	urtwm_setbits_1_shift(sc, R92C_MCUFWDL, R92C_MCUFWDL_ROM_DLEN, 0, 2);

	for (ntries = 0; ntries < 3; ntries++) {
		ptr2 = ptr;
		len2 = len;	/* XXX optimize */

		/* Reset the FWDL checksum. */
		urtwm_setbits_1(sc, R92C_MCUFWDL, 0, R92C_MCUFWDL_CHKSUM_RPT);

		for (page = 0; len2 > 0; page++) {
			mlen = min(len2, R92C_FW_PAGE_SIZE);
			error = urtwm_fw_loadpage(sc, page, ptr2, mlen);
			if (error != 0) {
				URTWM_DPRINTF(sc, URTWM_DEBUG_FIRMWARE,
				    "could not load firmware page (try %d)\n",
				    ntries);
				continue;
			}
			ptr2 += mlen;
			len2 -= mlen;
		}

		/* Wait for checksum report. */
		if (urtwm_fw_checksum_report(sc) == 0)
			break;
	}

	/* MCU download disable. */
	urtwm_setbits_1(sc, R92C_MCUFWDL, R92C_MCUFWDL_EN, 0);

	urtwm_setbits_4(sc, R92C_MCUFWDL, R92C_MCUFWDL_WINTINI_RDY,
	    R92C_MCUFWDL_RDY);

	urtwm_fw_reset(sc);

	/* Wait for firmware readiness. */
	for (ntries = 0; ntries < 20; ntries++) {
		if (urtwm_read_4(sc, R92C_MCUFWDL) & R92C_MCUFWDL_WINTINI_RDY)
			break;
		urtwm_delay(sc, 10000);
	}
	if (ntries == 20) {
		device_printf(sc->sc_dev,
		    "timeout waiting for firmware readiness\n");
		error = ETIMEDOUT;
		goto fail;
	}
fail:
	firmware_put(fw, FIRMWARE_UNLOAD);
	return (error);
}
#endif

static int
urtwm_r12a_set_page_size(struct urtwm_softc *sc)
{
	return (urtwm_setbits_1(sc, R92C_PBP, R92C_PBP_PSTX_M,
	    R92C_PBP_512 << R92C_PBP_PSTX_S) == USB_ERR_NORMAL_COMPLETION);
}

static int
urtwm_r21a_set_page_size(struct urtwm_softc *sc)
{
	return (0);	/* nothing to do */
}

static int
urtwm_dma_init(struct urtwm_softc *sc)
{
#define URTWM_CHK(res) do {			\
	if (res != USB_ERR_NORMAL_COMPLETION)	\
		return (EIO);			\
} while(0)
	uint16_t reg;
	int hasnq, haslq, nqueues;
	int error, nqpages, nrempages;

	/* Initialize LLT table. */
	error = urtwm_llt_init(sc);
	if (error != 0)
		return (error);

	/* Get Tx queues to USB endpoints mapping. */
	hasnq = haslq = 0;
	switch (sc->ntx) {
	case 4:
	case 3:
		haslq = 1;
		/* FALLTHROUGH */
	case 2:
		hasnq = 1;
		/* FALLTHROUGH */
	default:
		break;
	}
	nqueues = 1 + hasnq + haslq;

	/* Get the number of pages for each queue. */
	nqpages = (sc->page_count - sc->npubqpages) / nqueues;

	/* 
	 * The remaining pages are assigned to the high priority
	 * queue.
	 */
	nrempages = (sc->page_count - sc->npubqpages) % nqueues;

	URTWM_CHK(urtwm_write_1(sc, R92C_RQPN_NPQ, hasnq ? nqpages : 0));
	URTWM_CHK(urtwm_write_4(sc, R92C_RQPN,
	    /* Set number of pages for public queue. */
	    SM(R92C_RQPN_PUBQ, sc->npubqpages) |
	    /* Set number of pages for high priority queue. */
	    SM(R92C_RQPN_HPQ, nqpages + nrempages) |
	    /* Set number of pages for low priority queue. */
	    SM(R92C_RQPN_LPQ, haslq ? nqpages : 0) |
	    /* Load values. */
	    R92C_RQPN_LD));

	/* Initialize TX buffer boundary. */
	URTWM_CHK(urtwm_write_1(sc, R92C_TXPKTBUF_BCNQ_BDNY, sc->tx_boundary));
	URTWM_CHK(urtwm_write_1(sc, R92C_TXPKTBUF_MGQ_BDNY, sc->tx_boundary));
	URTWM_CHK(urtwm_write_1(sc, R92C_TXPKTBUF_WMAC_LBK_BF_HD,
	    sc->tx_boundary));
	URTWM_CHK(urtwm_write_1(sc, R92C_TRXFF_BNDY, sc->tx_boundary));
	URTWM_CHK(urtwm_write_1(sc, R92C_TDECTRL + 1, sc->tx_boundary));

	if (URTWM_CHIP_HAS_BCNQ1(sc)) {
		URTWM_CHK(urtwm_write_1(sc, R88E_TXPKTBUF_BCNQ1_BDNY,
		    sc->tx_boundary + 8));	/* XXX hardcoded */
		URTWM_CHK(urtwm_write_1(sc, R12A_DWBCN1_CTRL + 1,
		    sc->tx_boundary + 8));
		URTWM_CHK(urtwm_setbits_1(sc, R12A_DWBCN1_CTRL + 2, 0,
		    R12A_DWBCN1_CTRL_SEL_EN));
	}

	/* Set queue to USB pipe mapping. */
	switch (nqueues) {
	case 1:
		/* NB: should not happen for RTL881*AU. */
		reg = R92C_TRXDMA_CTRL_QMAP_HQ;
		break;
	case 2:
		reg = R92C_TRXDMA_CTRL_QMAP_HQ_NQ;
		break;
	default:
		reg = R92C_TRXDMA_CTRL_QMAP_3EP;
		break;
	}
	URTWM_CHK(urtwm_setbits_2(sc, R92C_TRXDMA_CTRL,
	    R92C_TRXDMA_CTRL_QMAP_M, reg));

	/* Set Tx/Rx transfer page boundary. */
	URTWM_CHK(urtwm_write_2(sc, R92C_TRXFF_BNDY + 2,
	    sc->rx_dma_size - 1));

	/* Set Tx/Rx transfer page size. */
	urtwm_set_page_size(sc);

	return (0);
}

static int
urtwm_mac_init(struct urtwm_softc *sc)
{
	usb_error_t error;
	int i;

	/* Write MAC initialization values. */
	for (i = 0; i < sc->mac_size; i++) {
		error = urtwm_write_1(sc, sc->mac_prog[i].reg,
		    sc->mac_prog[i].val);
		if (error != USB_ERR_NORMAL_COMPLETION)
			return (EIO);
	}

	return (0);
}

static void
urtwm_bb_init(struct urtwm_softc *sc)
{
	int i, j;

	urtwm_setbits_1(sc, R92C_SYS_FUNC_EN, 0, R92C_SYS_FUNC_EN_USBA);

	/* Enable BB and RF. */
	urtwm_setbits_1(sc, R92C_SYS_FUNC_EN, 0,
	    R92C_SYS_FUNC_EN_BBRSTB | R92C_SYS_FUNC_EN_BB_GLB_RST);

	/* PathA RF Power On. */
	urtwm_write_1(sc, R92C_RF_CTRL,
	    R92C_RF_CTRL_EN | R92C_RF_CTRL_RSTB | R92C_RF_CTRL_SDMRSTB);

	/* PathB RF Power On. */
	urtwm_write_1(sc, R12A_RF_B_CTRL,
	    R92C_RF_CTRL_EN | R92C_RF_CTRL_RSTB | R92C_RF_CTRL_SDMRSTB);

	/* Write BB initialization values. */
	for (i = 0; i < sc->bb_size; i++) {
		const struct urtwm_bb_prog *bb_prog = &sc->bb_prog[i];

		while (!urtwm_check_condition(sc, bb_prog->cond)) {
			KASSERT(bb_prog->next != NULL,
			    ("%s: wrong condition value (i %d)\n",
			    __func__, i));
			bb_prog = bb_prog->next;
		}

		for (j = 0; j < bb_prog->count; j++) {
			URTWM_DPRINTF(sc, URTWM_DEBUG_RESET,
			    "BB: reg 0x%03x, val 0x%08x\n",
			    bb_prog->reg[j], bb_prog->val[j]);

			urtwm_bb_write(sc, bb_prog->reg[j], bb_prog->val[j]);
			urtwm_delay(sc, 1);
		}
	}

	/* XXX meshpoint mode? */

	/* Write AGC values. */
	for (i = 0; i < sc->agc_size; i++) {
		const struct urtwm_agc_prog *agc_prog = &sc->agc_prog[i];

		while (!urtwm_check_condition(sc, agc_prog->cond)) {
			KASSERT(agc_prog->next != NULL,
			    ("%s: wrong condition value (2) (i %d)\n",
			    __func__, i));
			agc_prog = agc_prog->next;
		}

		for (j = 0; j < agc_prog->count; j++) {
			URTWM_DPRINTF(sc, URTWM_DEBUG_RESET,
			    "AGC: val 0x%08x\n", agc_prog->val[j]);

			urtwm_bb_write(sc, 0x81c, agc_prog->val[j]);
			urtwm_delay(sc, 1);
		}
	}

	for (i = 0; i < sc->nrxchains; i++) {
		urtwm_bb_write(sc, R12A_INITIAL_GAIN(i), 0x22);
		urtwm_delay(sc, 1);
		urtwm_bb_write(sc, R12A_INITIAL_GAIN(i), 0x20);
		urtwm_delay(sc, 1);
	}

	urtwm_crystalcap_write(sc);

	if (urtwm_bb_read(sc, R12A_CCK_RPT_FORMAT) & R12A_CCK_RPT_FORMAT_HIPWR)
		sc->sc_flags |= URTWM_FLAG_CCK_HIPWR;
}

static void
urtwm_r12a_crystalcap_write(struct urtwm_softc *sc)
{
	uint32_t reg;
	uint8_t val;

	val = sc->crystalcap & 0x3f;
	reg = urtwm_bb_read(sc, R92C_MAC_PHY_CTRL);
	reg = RW(reg, R12A_MAC_PHY_CRYSTALCAP, val | (val << 6));
	urtwm_bb_write(sc, R92C_MAC_PHY_CTRL, reg);
}

static void
urtwm_r21a_crystalcap_write(struct urtwm_softc *sc)
{
	uint32_t reg;
	uint8_t val;

	val = sc->crystalcap & 0x3f;
	reg = urtwm_bb_read(sc, R92C_MAC_PHY_CTRL);
	reg = RW(reg, R21A_MAC_PHY_CRYSTALCAP, val | (val << 6));
	urtwm_bb_write(sc, R92C_MAC_PHY_CTRL, reg);
}

static void
urtwm_rf_init(struct urtwm_softc *sc)
{
	int chain, i;

	for (chain = 0, i = 0; chain < sc->nrxchains; chain++, i++) {
		/* Write RF initialization values for this chain. */
		i += urtwm_rf_init_chain(sc, &sc->rf_prog[i], chain);
	}
}

static int
urtwm_rf_init_chain(struct urtwm_softc *sc,
    const struct urtwm_rf_prog *rf_prog, int chain)
{
	int i, j;

	URTWM_DPRINTF(sc, URTWM_DEBUG_RESET, "%s: chain %d\n",
	    __func__, chain);

	for (i = 0; rf_prog[i].reg != NULL; i++) {
		const struct urtwm_rf_prog *prog = &rf_prog[i];

		while (!urtwm_check_condition(sc, prog->cond)) {
			KASSERT(prog->next != NULL,
			    ("%s: wrong condition value (i %d)\n",
			    __func__, i));
			prog = prog->next;
		}

		for (j = 0; j < prog->count; j++) {
			URTWM_DPRINTF(sc, URTWM_DEBUG_RESET,
			    "RF: reg 0x%02x, val 0x%05x\n",
			    prog->reg[j], prog->val[j]);

			/*
			 * These are fake RF registers offsets that
			 * indicate a delay is required.
			 */
			/* NB: we are using 'value' to store required delay. */
			if (prog->reg[j] > 0xf8) {
				urtwm_delay(sc, prog->val[j]);
				continue;
			}

			urtwm_rf_write(sc, chain, prog->reg[j], prog->val[j]);
			urtwm_delay(sc, 1);
		}
	}

	return (i);
}

static void
urtwm_arfb_init(struct urtwm_softc *sc)
{
	/* ARFB table 9 for 11ac 5G 2SS. */
	urtwm_write_4(sc, R12A_ARFR_5G(0), 0x00000010);
	urtwm_write_4(sc, R12A_ARFR_5G(0) + 4, 0xfffff000);

	/* ARFB table 10 for 11ac 5G 1SS. */
	urtwm_write_4(sc, R12A_ARFR_5G(1), 0x00000010);
	urtwm_write_4(sc, R12A_ARFR_5G(1) + 4, 0x003ff000);

	/* ARFB table 11 for 11ac 2G 1SS. */
	urtwm_write_4(sc, R12A_ARFR_2G(0), 0x00000015);
	urtwm_write_4(sc, R12A_ARFR_2G(0) + 4, 0x003ff000);

	/* ARFB table 12 for 11ac 2G 2SS. */
	urtwm_write_4(sc, R12A_ARFR_2G(1), 0x00000015);
	urtwm_write_4(sc, R12A_ARFR_2G(1) + 4, 0xffcff000);
}

static void
urtwm_r21a_bypass_ext_lna_2ghz(struct urtwm_softc *sc)
{
	urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x00100000, 0);
	urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x00400000, 0);
	urtwm_bb_setbits(sc, R12A_RFE_PINMUX(0), 0, 0x07);
	urtwm_bb_setbits(sc, R12A_RFE_PINMUX(0), 0, 0x0700);
}

static void
urtwm_r12a_set_band_2ghz(struct urtwm_softc *sc)
{
	/* Enable CCK / OFDM. */
	urtwm_bb_setbits(sc, R12A_OFDMCCK_EN,
	    0, R12A_OFDMCCK_EN_CCK | R12A_OFDMCCK_EN_OFDM);

	urtwm_bb_setbits(sc, R12A_BW_INDICATION, 0x02, 0x01);
	urtwm_bb_setbits(sc, R12A_PWED_TH, 0x3e000, 0x2e000);

	/* Select AGC table. */
	urtwm_bb_setbits(sc, R12A_TXAGC_TABLE_SELECT, 0x03, 0);

	switch (sc->rfe_type) {
	case 0:
	case 1:
	case 2:
		urtwm_bb_write(sc, R12A_RFE_PINMUX(0), 0x77777777);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x77777777);
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x3ff00000, 0);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0);
		break;
	case 3:
		urtwm_bb_write(sc, R12A_RFE_PINMUX(0), 0x54337770);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x54337770);
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x3ff00000, 0x01000000);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0x01000000);
		urtwm_bb_setbits(sc, R12A_ANTSEL_SW, 0x0303, 0x01);
		break;
	case 4:
		urtwm_bb_write(sc, R12A_RFE_PINMUX(0), 0x77777777);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x77777777);
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x3ff00000, 0x00100000);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0x00100000);
		break;
	case 5:
		urtwm_write_1(sc, R12A_RFE_PINMUX(0) + 2, 0x77);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x77777777);
		urtwm_setbits_1(sc, R12A_RFE_INV(0) + 3, 0x01, 0);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0);
		break;
	default:
		break;
	}

	urtwm_bb_setbits(sc, R12A_TX_PATH, 0xf0, 0x10);
	urtwm_bb_setbits(sc, R12A_CCK_RX_PATH, 0x0f000000, 0x01000000);

	/* Write basic rates (1, 2, 5.5, 11, 6, 12, 24). */
	/* XXX check ic_curmode. */
	urtwm_setbits_4(sc, R92C_RRSR, R92C_RRSR_RATE_BITMAP_M,
	   (1 << URTWM_RIDX_CCK1)  | (1 << URTWM_RIDX_CCK2) |
	   (1 << URTWM_RIDX_CCK55) | (1 << URTWM_RIDX_CCK11) |
	   (1 << URTWM_RIDX_OFDM6) | (1 << URTWM_RIDX_OFDM12) |
	   (1 << URTWM_RIDX_OFDM24));

	urtwm_write_1(sc, R12A_CCK_CHECK, 0);
}

static void
urtwm_r21a_set_band_2ghz(struct urtwm_softc *sc)
{
	/* Enable CCK / OFDM. */
	urtwm_bb_setbits(sc, R12A_OFDMCCK_EN,
	    0, R12A_OFDMCCK_EN_CCK | R12A_OFDMCCK_EN_OFDM);

	/* Turn off RF PA and LNA. */
	urtwm_bb_setbits(sc, R12A_RFE_PINMUX(0),
	    R12A_RFE_PINMUX_LNA_MASK, 0x7000);
	urtwm_bb_setbits(sc, R12A_RFE_PINMUX(0),
	    R12A_RFE_PINMUX_PA_A_MASK, 0x70);

	if (sc->ext_lna_2g) {
		/* Turn on 2.4 GHz external LNA. */
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0, 0x00100000);
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x00400000, 0);
		urtwm_bb_setbits(sc, R12A_RFE_PINMUX(0), 0x05, 0x02);
		urtwm_bb_setbits(sc, R12A_RFE_PINMUX(0), 0x0500, 0x0200);
	} else {
		/* Bypass 2.4 GHz external LNA. */
		urtwm_r21a_bypass_ext_lna_2ghz(sc);
	}

	/* Select AGC table. */
	urtwm_bb_setbits(sc, R12A_TX_SCALE(0), 0x0f00, 0);

	urtwm_bb_setbits(sc, R12A_TX_PATH, 0xf0, 0x10);
	urtwm_bb_setbits(sc, R12A_CCK_RX_PATH, 0x0f000000, 0x01000000);

	/* Write basic rates (1, 2, 5.5, 11, 6, 12, 24). */
	/* XXX check ic_curmode. */
	urtwm_setbits_4(sc, R92C_RRSR, R92C_RRSR_RATE_BITMAP_M,
	   (1 << URTWM_RIDX_CCK1)  | (1 << URTWM_RIDX_CCK2) |
	   (1 << URTWM_RIDX_CCK55) | (1 << URTWM_RIDX_CCK11) |
	   (1 << URTWM_RIDX_OFDM6) | (1 << URTWM_RIDX_OFDM12) |
	   (1 << URTWM_RIDX_OFDM24));

	urtwm_write_1(sc, R12A_CCK_CHECK, 0);
}

static void
urtwm_r12a_set_band_5ghz(struct urtwm_softc *sc)
{
	int ntries;

	urtwm_write_1(sc, R12A_CCK_CHECK, 0x80);

	for (ntries = 0; ntries < 100; ntries++) {
		if ((urtwm_read_2(sc, R12A_TXPKT_EMPTY) & 0x30) == 0x30)
			break;

		urtwm_delay(sc, 25);
	}
	if (ntries == 100) {
		device_printf(sc->sc_dev,
		    "%s: TXPKT_EMPTY check failed (%04X)\n",
		    __func__, urtwm_read_2(sc, R12A_TXPKT_EMPTY));
	}

	/* Enable OFDM. */
	urtwm_bb_setbits(sc, R12A_OFDMCCK_EN, R12A_OFDMCCK_EN_CCK,
	    R12A_OFDMCCK_EN_OFDM);

	urtwm_bb_setbits(sc, R12A_BW_INDICATION, 0x01, 0x02);
	urtwm_bb_setbits(sc, R12A_PWED_TH, 0x3e000, 0x2a000);

	/* Select AGC table. */
	urtwm_bb_setbits(sc, R12A_TXAGC_TABLE_SELECT, 0x03, 0x01);

	switch (sc->rfe_type) {
	case 0:
		urtwm_bb_write(sc, R12A_RFE_PINMUX(0), 0x77337717);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x77337717);
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x3ff00000, 0x01000000);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0x01000000);
		break;
	case 1:
		urtwm_bb_write(sc, R12A_RFE_PINMUX(0), 0x77337717);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x77337717);
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x3ff00000, 0);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0);
		break;
	case 2:
	case 4:
		urtwm_bb_write(sc, R12A_RFE_PINMUX(0), 0x77337777);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x77337777);
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x3ff00000, 0x01000000);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0x01000000);
		break;
	case 3:
		urtwm_bb_write(sc, R12A_RFE_PINMUX(0), 0x54337717);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x54337717);
		urtwm_bb_setbits(sc, R12A_RFE_INV(0), 0x3ff00000, 0x01000000);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0x01000000);
		urtwm_bb_setbits(sc, R12A_ANTSEL_SW, 0x0303, 0x01);
		break;
	case 5:
		urtwm_write_1(sc, R12A_RFE_PINMUX(0) + 2, 0x33);
		urtwm_bb_write(sc, R12A_RFE_PINMUX(1), 0x77337777);
		urtwm_setbits_1(sc, R12A_RFE_INV(0) + 3, 0, 0x01);
		urtwm_bb_setbits(sc, R12A_RFE_INV(1), 0x3ff00000, 0x01000000);
		break;
	default:
		break;
	}

	urtwm_bb_setbits(sc, R12A_TX_PATH, 0xf0, 0);
	urtwm_bb_setbits(sc, R12A_CCK_RX_PATH, 0, 0x0f000000);

	/* Write basic rates (6, 12, 24). */
	/* XXX obtain from net80211. */
	urtwm_setbits_4(sc, R92C_RRSR, R92C_RRSR_RATE_BITMAP_M,
	    (1 << URTWM_RIDX_OFDM6) | (1 << URTWM_RIDX_OFDM12) |
	    (1 << URTWM_RIDX_OFDM24));
}

static void
urtwm_r21a_set_band_5ghz(struct urtwm_softc *sc)
{
	int ntries;

	urtwm_bb_setbits(sc, R12A_RFE_PINMUX(0),
	    R12A_RFE_PINMUX_LNA_MASK, 0x5000);
	urtwm_bb_setbits(sc, R12A_RFE_PINMUX(0),
	    R12A_RFE_PINMUX_PA_A_MASK, 0x40);

	if (sc->ext_lna_2g) {
		/* Bypass 2.4 GHz external LNA. */
		urtwm_r21a_bypass_ext_lna_2ghz(sc);
	}

	urtwm_write_1(sc, R12A_CCK_CHECK, 0x80);

	for (ntries = 0; ntries < 100; ntries++) {
		if ((urtwm_read_2(sc, R12A_TXPKT_EMPTY) & 0x30) == 0x30)
			break;

		urtwm_delay(sc, 25);
	}
	if (ntries == 100) {
		device_printf(sc->sc_dev,
		    "%s: TXPKT_EMPTY check failed (%04X)\n",
		    __func__, urtwm_read_2(sc, R12A_TXPKT_EMPTY));
	}

	/* Enable OFDM. */
	urtwm_bb_setbits(sc, R12A_OFDMCCK_EN, R12A_OFDMCCK_EN_CCK,
	    R12A_OFDMCCK_EN_OFDM);

	/* Select AGC table. */
	urtwm_bb_setbits(sc, R12A_TX_SCALE(0), 0x0f00, 0x0100);

	urtwm_bb_setbits(sc, R12A_TX_PATH, 0xf0, 0);
	urtwm_bb_setbits(sc, R12A_CCK_RX_PATH, 0, 0x0f000000);

	/* Write basic rates (6, 12, 24). */
	/* XXX obtain from net80211. */
	urtwm_setbits_4(sc, R92C_RRSR, R92C_RRSR_RATE_BITMAP_M,
	    (1 << URTWM_RIDX_OFDM6) | (1 << URTWM_RIDX_OFDM12) |
	    (1 << URTWM_RIDX_OFDM24));
}

static void
urtwm_set_band(struct urtwm_softc *sc, struct ieee80211_channel *c, int force)
{
	uint8_t swing;
	int i;

	/* Check if band was changed. */
	if (!force && IEEE80211_IS_CHAN_5GHZ(c) ^
	    !(urtwm_read_1(sc, R12A_CCK_CHECK) & R12A_CCK_CHECK_5GHZ))
		return;

	if (IEEE80211_IS_CHAN_2GHZ(c)) {
		urtwm_set_band_2ghz(sc);
		swing = sc->tx_bbswing_2g;
	} else if (IEEE80211_IS_CHAN_5GHZ(c)) {
		urtwm_set_band_5ghz(sc);
		swing = sc->tx_bbswing_5g;
	} else {
		KASSERT(0, ("wrong channel flags %08X\n", c->ic_flags));
		return;
	}

	/* XXX PATH_B is set by vendor driver. */
	for (i = 0; i < 2; i++) {
		uint16_t val;

		switch ((swing >> i * 2) & 0x3) {
		case 0:
			val = 0x200;	/* 0 dB	*/
			break;
		case 1:
			val = 0x16a;	/* -3 dB */
			break;
		case 2:
			val = 0x101;	/* -6 dB */
			break;
		case 3:
			val = 0xb6;	/* -9 dB */
			break;
		}

		urtwm_bb_setbits(sc, R12A_TX_SCALE(i), R12A_TX_SCALE_SWING_M,
		    val << R12A_TX_SCALE_SWING_S);
	}
}

static void
urtwm_cam_init(struct urtwm_softc *sc)
{
	/* Invalidate all CAM entries. */
	urtwm_write_4(sc, R92C_CAMCMD,
	    R92C_CAMCMD_POLLING | R92C_CAMCMD_CLR);
}

static int
urtwm_cam_write(struct urtwm_softc *sc, uint32_t addr, uint32_t data)
{
	usb_error_t error;

	error = urtwm_write_4(sc, R92C_CAMWRITE, data);
	if (error != USB_ERR_NORMAL_COMPLETION)
		return (EIO);
	error = urtwm_write_4(sc, R92C_CAMCMD,
	    R92C_CAMCMD_POLLING | R92C_CAMCMD_WRITE |
	    SM(R92C_CAMCMD_ADDR, addr));
	if (error != USB_ERR_NORMAL_COMPLETION)
		return (EIO);

	return (0);
}

static void
urtwm_rxfilter_init(struct urtwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);
	uint32_t rcr;
	uint16_t filter;

	URTWM_ASSERT_LOCKED(sc);

	/* Setup multicast filter. */
	urtwm_set_multi(sc);

	/* Filter for management frames. */
	filter = 0x7f3f;
	switch (vap->iv_opmode) {
	case IEEE80211_M_STA:
		filter &= ~(
		    R92C_RXFLTMAP_SUBTYPE(IEEE80211_FC0_SUBTYPE_ASSOC_REQ) |
		    R92C_RXFLTMAP_SUBTYPE(IEEE80211_FC0_SUBTYPE_REASSOC_REQ) |
		    R92C_RXFLTMAP_SUBTYPE(IEEE80211_FC0_SUBTYPE_PROBE_REQ));
		break;
	case IEEE80211_M_HOSTAP:
		filter &= ~(
		    R92C_RXFLTMAP_SUBTYPE(IEEE80211_FC0_SUBTYPE_ASSOC_RESP) |
		    R92C_RXFLTMAP_SUBTYPE(IEEE80211_FC0_SUBTYPE_REASSOC_RESP));
		break;
	case IEEE80211_M_MONITOR:
	case IEEE80211_M_IBSS:
		break;
	default:
		device_printf(sc->sc_dev, "%s: undefined opmode %d\n",
		    __func__, vap->iv_opmode);
		break;
	}
	urtwm_write_2(sc, R92C_RXFLTMAP0, filter);

	/* Reject all control frames. */
	urtwm_write_2(sc, R92C_RXFLTMAP1, 0x0000);

	/* Reject all data frames. */
	urtwm_write_2(sc, R92C_RXFLTMAP2, 0x0000);

	rcr = R92C_RCR_AM | R92C_RCR_AB | R92C_RCR_APM |
	      R92C_RCR_HTC_LOC_CTRL | R92C_RCR_APP_PHYSTS |
	      R92C_RCR_APP_ICV | R92C_RCR_APP_MIC |
	      R12A_RCR_DIS_CHK_14 | R12A_RCR_VHT_ACK;

	if (sc->sc_flags & (URTWM_RXCKSUM_EN | URTWM_RXCKSUM6_EN))
		rcr |= R12A_RCR_TCP_OFFLD_EN;

	if (vap->iv_opmode == IEEE80211_M_MONITOR) {
		/* Accept all frames. */
		rcr |= R92C_RCR_ACF | R92C_RCR_ADF | R92C_RCR_AMF |
		       R92C_RCR_AAP;
	}

	/* Set Rx filter. */
	urtwm_write_4(sc, R92C_RCR, rcr);

	if (ic->ic_promisc != 0) {
		/* Update Rx filter. */
		urtwm_set_promisc(sc);
	}
}

static void
urtwm_edca_init(struct urtwm_softc *sc)
{
	/* SIFS */
	urtwm_write_2(sc, R92C_SPEC_SIFS, 0x100a);
	urtwm_write_2(sc, R92C_MAC_SPEC_SIFS, 0x100a);
	urtwm_write_2(sc, R92C_SIFS_CCK, 0x100a);
	urtwm_write_2(sc, R92C_SIFS_OFDM, 0x100a);
	/* TXOP */
	urtwm_write_4(sc, R92C_EDCA_BE_PARAM, 0x005ea42b);
	urtwm_write_4(sc, R92C_EDCA_BK_PARAM, 0x0000a44f);
	urtwm_write_4(sc, R92C_EDCA_VI_PARAM, 0x005ea324);
	urtwm_write_4(sc, R92C_EDCA_VO_PARAM, 0x002fa226);
	/* 80 MHz clock */
	urtwm_write_1(sc, R92C_USTIME_TSF, 0x50);
	urtwm_write_1(sc, R92C_USTIME_EDCA, 0x50);
}

static void
urtwm_mrr_init(struct urtwm_softc *sc)
{
	int i;

	/* Drop rate index by 1 per retry. */
	for (i = 0; i < R12A_MRR_SIZE; i++)
		urtwm_write_1(sc, R92C_DARFRC + i, i + 1);
}

static void
urtwm_write_txpower(struct urtwm_softc *sc, int chain,
    struct ieee80211_channel *c, uint16_t power[URTWM_RIDX_COUNT])
{

	if (IEEE80211_IS_CHAN_2GHZ(c)) {
		/* Write per-CCK rate Tx power. */
		urtwm_bb_write(sc, R12A_TXAGC_CCK11_1(chain),
		    SM(R12A_TXAGC_CCK1,  power[URTWM_RIDX_CCK1]) |
		    SM(R12A_TXAGC_CCK2,  power[URTWM_RIDX_CCK2]) |
		    SM(R12A_TXAGC_CCK55, power[URTWM_RIDX_CCK55]) |
		    SM(R12A_TXAGC_CCK11, power[URTWM_RIDX_CCK11]));
	}

	/* Write per-OFDM rate Tx power. */
	urtwm_bb_write(sc, R12A_TXAGC_OFDM18_6(chain),
	    SM(R12A_TXAGC_OFDM06, power[URTWM_RIDX_OFDM6]) |
	    SM(R12A_TXAGC_OFDM09, power[URTWM_RIDX_OFDM9]) |
	    SM(R12A_TXAGC_OFDM12, power[URTWM_RIDX_OFDM12]) |
	    SM(R12A_TXAGC_OFDM18, power[URTWM_RIDX_OFDM18]));
	urtwm_bb_write(sc, R12A_TXAGC_OFDM54_24(chain),
	    SM(R12A_TXAGC_OFDM24, power[URTWM_RIDX_OFDM24]) |
	    SM(R12A_TXAGC_OFDM36, power[URTWM_RIDX_OFDM36]) |
	    SM(R12A_TXAGC_OFDM48, power[URTWM_RIDX_OFDM48]) |
	    SM(R12A_TXAGC_OFDM54, power[URTWM_RIDX_OFDM54]));

	/* Write per-MCS Tx power. */
	urtwm_bb_write(sc, R12A_TXAGC_MCS3_0(chain),
	    SM(R12A_TXAGC_MCS0, power[URTWM_RIDX_MCS(0)]) |
	    SM(R12A_TXAGC_MCS1, power[URTWM_RIDX_MCS(1)]) |
	    SM(R12A_TXAGC_MCS2, power[URTWM_RIDX_MCS(2)]) |
	    SM(R12A_TXAGC_MCS3, power[URTWM_RIDX_MCS(3)]));
	urtwm_bb_write(sc, R12A_TXAGC_MCS7_4(chain),
	    SM(R12A_TXAGC_MCS4, power[URTWM_RIDX_MCS(4)]) |
	    SM(R12A_TXAGC_MCS5, power[URTWM_RIDX_MCS(5)]) |
	    SM(R12A_TXAGC_MCS6, power[URTWM_RIDX_MCS(6)]) |
	    SM(R12A_TXAGC_MCS7, power[URTWM_RIDX_MCS(7)]));
	urtwm_bb_write(sc, R12A_TXAGC_MCS11_8(chain),
	    SM(R12A_TXAGC_MCS8,  power[URTWM_RIDX_MCS(8)]) |
	    SM(R12A_TXAGC_MCS9,  power[URTWM_RIDX_MCS(9)]) |
	    SM(R12A_TXAGC_MCS10, power[URTWM_RIDX_MCS(10)]) |
	    SM(R12A_TXAGC_MCS11, power[URTWM_RIDX_MCS(11)]));
	urtwm_bb_write(sc, R12A_TXAGC_MCS15_12(chain),
	    SM(R12A_TXAGC_MCS12, power[URTWM_RIDX_MCS(12)]) |
	    SM(R12A_TXAGC_MCS13, power[URTWM_RIDX_MCS(13)]) |
	    SM(R12A_TXAGC_MCS14, power[URTWM_RIDX_MCS(14)]) |
	    SM(R12A_TXAGC_MCS15, power[URTWM_RIDX_MCS(15)]));

	/* TODO: VHT rates */
}

static int
urtwm_get_power_group(struct urtwm_softc *sc, struct ieee80211_channel *c)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint8_t chan;
	int group;

	chan = ieee80211_chan2ieee(ic, c);

	if (IEEE80211_IS_CHAN_2GHZ(c)) {
		if (chan <= 2)			group = 0;
		else if (chan <= 5)		group = 1;
		else if (chan <= 8)		group = 2;
		else if (chan <= 11)		group = 3;
		else if (chan <= 14)		group = 4;
		else {
			KASSERT(0, ("wrong 2GHz channel %d!\n", chan));
			return (-1);
		}
	} else if (IEEE80211_IS_CHAN_5GHZ(c)) {
		if (chan < 36)
			return (-1);

		if (chan <= 42)			group = 0;
		else if (chan <= 48)		group = 1;
		else if (chan <= 58)		group = 2;
		else if (chan <= 64)		group = 3;
		else if (chan <= 106)		group = 4;
		else if (chan <= 114)		group = 5;
		else if (chan <= 122)		group = 6;
		else if (chan <= 130)		group = 7;
		else if (chan <= 138)		group = 8;
		else if (chan <= 144)		group = 9;
		else if (chan <= 155)		group = 10;
		else if (chan <= 161)		group = 11;
		else if (chan <= 171)		group = 12;
		else if (chan <= 177)		group = 13;
		else {
			KASSERT(0, ("wrong 5GHz channel %d!\n", chan));
			return (-1);
		}
	} else {
		KASSERT(0, ("wrong channel band (flags %08X)\n", c->ic_flags));
		return (-1);
	}

	return (group);
}

static void
urtwm_get_txpower(struct urtwm_softc *sc, int chain,
    struct ieee80211_channel *c, uint16_t power[URTWM_RIDX_COUNT])
{
	int i, ridx, group, max_mcs;

	/* Determine channel group. */
	group = urtwm_get_power_group(sc, c);
	if (group == -1) {	/* shouldn't happen */
		device_printf(sc->sc_dev, "%s: incorrect channel\n", __func__);
		return;
	}

	/* TODO: VHT rates. */
	max_mcs = URTWM_RIDX_MCS(sc->ntxchains * 8 - 1);

	/* XXX regulatory */
	/* XXX net80211 regulatory */

	if (IEEE80211_IS_CHAN_2GHZ(c)) {
		for (ridx = URTWM_RIDX_CCK1; ridx <= URTWM_RIDX_CCK11; ridx++)
			power[ridx] = sc->cck_tx_pwr[chain][group];
		for (ridx = URTWM_RIDX_OFDM6; ridx <= max_mcs; ridx++)
			power[ridx] = sc->ht40_tx_pwr_2g[chain][group];

		if (URTWM_RATE_IS_OFDM(ridx)) {
			uint8_t pwr_diff = sc->ofdm_tx_pwr_diff_2g[chain][0];
			for (ridx = URTWM_RIDX_CCK1; ridx <= max_mcs; ridx++)
				power[ridx] += pwr_diff;
		}

		for (i = 0; i < sc->ntxchains; i++) {
			uint8_t min_mcs;
			uint8_t pwr_diff;

			if (IEEE80211_IS_CHAN_HT20(c))
				pwr_diff = sc->bw20_tx_pwr_diff_2g[chain][i];
			else if (IEEE80211_IS_CHAN_HT40(c))
				pwr_diff = sc->bw40_tx_pwr_diff_2g[chain][i];
#ifdef notyet
			else if (IEEE80211_IS_CHAN_HT80(c)) {
				/* Vendor driver uses HT40 values here. */
				pwr_diff = sc->bw40_tx_pwr_diff_2g[chain][i];
			}
#endif

			min_mcs = URTWM_RIDX_MCS(i * 8 + 7);
			for (ridx = min_mcs; ridx <= max_mcs; ridx++)
				power[ridx] += pwr_diff;
		}
	} else {	/* 5GHz */
		for (ridx = URTWM_RIDX_OFDM6; ridx <= max_mcs; ridx++)
			power[ridx] = sc->ht40_tx_pwr_5g[chain][group];

		for (i = 0; i < sc->ntxchains; i++) {
			uint8_t min_mcs;
			uint8_t pwr_diff;

			if (IEEE80211_IS_CHAN_HT20(c))
				pwr_diff = sc->bw20_tx_pwr_diff_5g[chain][i];
			else if (IEEE80211_IS_CHAN_HT40(c))
				pwr_diff = sc->bw40_tx_pwr_diff_5g[chain][i];
#ifdef notyet
			else if (IEEE80211_IS_CHAN_HT80(c)) {
				/* TODO: calculate base value. */
				pwr_diff = sc->bw80_tx_pwr_diff_5g[chain][i];
			}
#endif

			min_mcs = URTWM_RIDX_MCS(i * 8 + 7);
			for (ridx = min_mcs; ridx <= max_mcs; ridx++)
				power[ridx] += pwr_diff;
		}
	}

	/* Apply max limit. */
	for (ridx = URTWM_RIDX_CCK1; ridx <= max_mcs; ridx++) {
		if (power[ridx] > R92C_MAX_TX_PWR)
			power[ridx] = R92C_MAX_TX_PWR;
	}

#ifdef USB_DEBUG
	if (sc->sc_debug & URTWM_DEBUG_TXPWR) {
		/* Dump per-rate Tx power values. */
		printf("Tx power for chain %d:\n", chain);
		for (ridx = URTWM_RIDX_CCK1; ridx < URTWM_RIDX_COUNT; ridx++)
			printf("Rate %d = %u\n", ridx, power[ridx]);
	}
#endif
}

static void
urtwm_set_txpower(struct urtwm_softc *sc, struct ieee80211_channel *c)
{
	uint16_t power[URTWM_RIDX_COUNT];
	int i;

	for (i = 0; i < sc->ntxchains; i++) {
		memset(power, 0, sizeof(power));
		/* Compute per-rate Tx power values. */
		urtwm_get_txpower(sc, i, c, power);
		/* Write per-rate Tx power values to hardware. */
		urtwm_write_txpower(sc, i, c, power);
	}
}

static void
urtwm_set_rx_bssid_all(struct urtwm_softc *sc, int enable)
{
	if (enable)
		urtwm_setbits_4(sc, R92C_RCR, R92C_RCR_CBSSID_BCN, 0);
	else
		urtwm_setbits_4(sc, R92C_RCR, 0, R92C_RCR_CBSSID_BCN);
}

static void
urtwm_scan_start(struct ieee80211com *ic)
{
	struct urtwm_softc *sc = ic->ic_softc;

	URTWM_LOCK(sc);
	/* Receive beacons / probe responses from any BSSID. */
	if (ic->ic_opmode != IEEE80211_M_IBSS &&
	    ic->ic_opmode != IEEE80211_M_HOSTAP)
		urtwm_set_rx_bssid_all(sc, 1);
	URTWM_UNLOCK(sc);
}

static void
urtwm_scan_curchan(struct ieee80211_scan_state *ss, unsigned long maxdwell)
{
	struct urtwm_softc *sc = ss->ss_ic->ic_softc;

	/* Make link LED blink during scan. */
	URTWM_LOCK(sc);
	urtwm_set_led(sc, URTWM_LED_LINK, !sc->ledlink);
	URTWM_UNLOCK(sc);

	sc->sc_scan_curchan(ss, maxdwell);
}

static void
urtwm_scan_end(struct ieee80211com *ic)
{
	struct urtwm_softc *sc = ic->ic_softc;
	struct ieee80211vap *vap;

	URTWM_LOCK(sc);
	/* Restore limitations. */
	if (ic->ic_promisc == 0 &&
	    ic->ic_opmode != IEEE80211_M_IBSS &&
	    ic->ic_opmode != IEEE80211_M_HOSTAP)
		urtwm_set_rx_bssid_all(sc, 0);

	vap = TAILQ_FIRST(&ic->ic_vaps);
	if (vap->iv_state == IEEE80211_S_RUN)
		urtwm_set_led(sc, URTWM_LED_LINK, 1);
	else
		urtwm_set_led(sc, URTWM_LED_LINK, 0);
	URTWM_UNLOCK(sc);
}

static void
urtwm_getradiocaps(struct ieee80211com *ic,
    int maxchans, int *nchans, struct ieee80211_channel chans[])
{
	uint8_t bands[IEEE80211_MODE_BYTES];

	memset(bands, 0, sizeof(bands));
	setbit(bands, IEEE80211_MODE_11B);
	setbit(bands, IEEE80211_MODE_11G);
	setbit(bands, IEEE80211_MODE_11NG);
	ieee80211_add_channel_list_2ghz(chans, maxchans, nchans,
	    urtwm_chan_2ghz, nitems(urtwm_chan_2ghz), bands, 0);

	setbit(bands, IEEE80211_MODE_11A);
	setbit(bands, IEEE80211_MODE_11NA);
	ieee80211_add_channel_list_5ghz(chans, maxchans, nchans,
	    urtwm_chan_5ghz, nitems(urtwm_chan_5ghz), bands, 0);
}

static void
urtwm_update_chw(struct ieee80211com *ic)
{
}

static void
urtwm_set_channel(struct ieee80211com *ic)
{
	struct urtwm_softc *sc = ic->ic_softc;
	struct ieee80211_channel *c = ic->ic_curchan;

	URTWM_LOCK(sc);
	urtwm_set_chan(sc, c);
	sc->sc_rxtap.wr_chan_freq = htole16(c->ic_freq);
	sc->sc_rxtap.wr_chan_flags = htole16(c->ic_flags);
	sc->sc_txtap.wt_chan_freq = htole16(c->ic_freq);
	sc->sc_txtap.wt_chan_flags = htole16(c->ic_flags);
	URTWM_UNLOCK(sc);
}

static int
urtwm_wme_update(struct ieee80211com *ic)
{
	struct ieee80211_channel *c = ic->ic_curchan;
	struct urtwm_softc *sc = ic->ic_softc;
	struct wmeParams *wmep = sc->cap_wmeParams;
	uint8_t aifs, acm, slottime;
	int ac;

	/* Prevent possible races. */
	IEEE80211_LOCK(ic);	/* XXX */
	URTWM_LOCK(sc);
	memcpy(wmep, ic->ic_wme.wme_chanParams.cap_wmeParams,
	    sizeof(sc->cap_wmeParams));
	URTWM_UNLOCK(sc);
	IEEE80211_UNLOCK(ic);

	acm = 0;
	slottime = IEEE80211_GET_SLOTTIME(ic);

	URTWM_LOCK(sc);
	for (ac = WME_AC_BE; ac < WME_NUM_AC; ac++) {
		/* AIFS[AC] = AIFSN[AC] * aSlotTime + aSIFSTime. */
		aifs = wmep[ac].wmep_aifsn * slottime +
		    (IEEE80211_IS_CHAN_5GHZ(c) ?
			IEEE80211_DUR_OFDM_SIFS : IEEE80211_DUR_SIFS);
		urtwm_write_4(sc, wme2queue[ac].reg,
		    SM(R92C_EDCA_PARAM_TXOP, wmep[ac].wmep_txopLimit) |
		    SM(R92C_EDCA_PARAM_ECWMIN, wmep[ac].wmep_logcwmin) |
		    SM(R92C_EDCA_PARAM_ECWMAX, wmep[ac].wmep_logcwmax) |
		    SM(R92C_EDCA_PARAM_AIFS, aifs));
		if (ac != WME_AC_BE)
			acm |= wmep[ac].wmep_acm << ac;
	}

	if (acm != 0)
		acm |= R92C_ACMHWCTRL_EN;
	urtwm_setbits_1(sc, R92C_ACMHWCTRL, R92C_ACMHWCTRL_ACM_MASK, acm);
	URTWM_UNLOCK(sc);

	return 0;
}

static void
urtwm_update_slot(struct ieee80211com *ic)
{
	urtwm_cmd_sleepable(ic->ic_softc, NULL, 0, urtwm_update_slot_cb);
}

static void
urtwm_update_slot_cb(struct urtwm_softc *sc, union sec_param *data)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint8_t slottime;

	slottime = IEEE80211_GET_SLOTTIME(ic);

	URTWM_DPRINTF(sc, URTWM_DEBUG_STATE, "%s: setting slot time to %uus\n",
	    __func__, slottime);

	urtwm_write_1(sc, R92C_SLOT, slottime);
	urtwm_update_aifs(sc, slottime);
}

static void
urtwm_update_aifs(struct urtwm_softc *sc, uint8_t slottime)
{
	struct ieee80211_channel *c = sc->sc_ic.ic_curchan;
	const struct wmeParams *wmep = sc->cap_wmeParams;
	uint8_t aifs, ac;

	for (ac = WME_AC_BE; ac < WME_NUM_AC; ac++) {
		/* AIFS[AC] = AIFSN[AC] * aSlotTime + aSIFSTime. */
		aifs = wmep[ac].wmep_aifsn * slottime +
		    (IEEE80211_IS_CHAN_5GHZ(c) ?
			IEEE80211_DUR_OFDM_SIFS : IEEE80211_DUR_SIFS);
		urtwm_write_1(sc, wme2queue[ac].reg, aifs);
	}
}

static uint8_t
urtwm_get_multi_pos(const uint8_t maddr[])
{
	uint64_t mask = 0x00004d101df481b4;
	uint8_t pos = 0x27;	/* initial value */
	int i, j;

	for (i = 0; i < IEEE80211_ADDR_LEN; i++)
		for (j = (i == 0) ? 1 : 0; j < 8; j++)
			if ((maddr[i] >> j) & 1)
				pos ^= (mask >> (i * 8 + j - 1));

	pos &= 0x3f;

	return (pos);
}

static void
urtwm_set_multi(struct urtwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t mfilt[2];

	URTWM_ASSERT_LOCKED(sc);

	/* general structure was copied from ath(4). */
	if (ic->ic_allmulti == 0) {
		struct ieee80211vap *vap;
		struct ifnet *ifp;
		struct ifmultiaddr *ifma;

		/*
		 * Merge multicast addresses to form the hardware filter.
		 */
		mfilt[0] = mfilt[1] = 0;
		TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
			ifp = vap->iv_ifp;
			if_maddr_rlock(ifp);
			TAILQ_FOREACH(ifma, &ifp->if_multiaddrs, ifma_link) {
				caddr_t dl;
				uint8_t pos;

				dl = LLADDR((struct sockaddr_dl *)
				    ifma->ifma_addr);
				pos = urtwm_get_multi_pos(dl);

				mfilt[pos / 32] |= (1 << (pos % 32));
			}
			if_maddr_runlock(ifp);
		}
	} else
		mfilt[0] = mfilt[1] = ~0;


	urtwm_write_4(sc, R92C_MAR + 0, mfilt[0]);
	urtwm_write_4(sc, R92C_MAR + 4, mfilt[1]);

	URTWM_DPRINTF(sc, URTWM_DEBUG_STATE, "%s: MC filter %08x:%08x\n",
	     __func__, mfilt[0], mfilt[1]);
}

static void
urtwm_set_promisc(struct urtwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);
	uint32_t mask1, mask2;

	URTWM_ASSERT_LOCKED(sc);

	if (vap->iv_opmode == IEEE80211_M_MONITOR)
		return;

	mask1 = R92C_RCR_ACF | R92C_RCR_ADF | R92C_RCR_AMF | R92C_RCR_AAP;
	mask2 = R92C_RCR_APM;

	if (vap->iv_state == IEEE80211_S_RUN) {
		switch (vap->iv_opmode) {
		case IEEE80211_M_STA:
			mask2 |= R92C_RCR_CBSSID_BCN;
			/* FALLTHROUGH */
		case IEEE80211_M_IBSS:
			mask2 |= R92C_RCR_CBSSID_DATA;
			break;
		case IEEE80211_M_HOSTAP:
			break;
		default:
			device_printf(sc->sc_dev, "%s: undefined opmode %d\n",
			    __func__, vap->iv_opmode);
			return;
		}
	}

	if (ic->ic_promisc == 0)
		urtwm_setbits_4(sc, R92C_RCR, mask1, mask2);
	else
		urtwm_setbits_4(sc, R92C_RCR, mask2, mask1);
}

static void
urtwm_update_promisc(struct ieee80211com *ic)
{
	struct urtwm_softc *sc = ic->ic_softc;

	URTWM_LOCK(sc);
	if (sc->sc_flags & URTWM_RUNNING)
		urtwm_set_promisc(sc);
	URTWM_UNLOCK(sc);
}

static void
urtwm_update_mcast(struct ieee80211com *ic)
{
	struct urtwm_softc *sc = ic->ic_softc;

	URTWM_LOCK(sc);
	if (sc->sc_flags & URTWM_RUNNING)
		urtwm_set_multi(sc);
	URTWM_UNLOCK(sc);
}

static struct ieee80211_node *
urtwm_node_alloc(struct ieee80211vap *vap,
    const uint8_t mac[IEEE80211_ADDR_LEN])
{
	struct urtwm_node *un;

	un = malloc(sizeof (struct urtwm_node), M_80211_NODE,
	    M_NOWAIT | M_ZERO);

	if (un == NULL)
		return NULL;

	un->id = URTWM_MACID_UNDEFINED;

	return &un->ni;
}

static void
urtwm_newassoc(struct ieee80211_node *ni, int isnew)
{
	struct urtwm_softc *sc = ni->ni_ic->ic_softc;
	struct urtwm_node *un = URTWM_NODE(ni);
	uint8_t id;

	if (!isnew)
		return;

	URTWM_NT_LOCK(sc);
	for (id = 0; id <= URTWM_MACID_MAX(sc); id++) {
		if (id != URTWM_MACID_BC && sc->node_list[id] == NULL) {
			un->id = id;
			sc->node_list[id] = ni;
			break;
		}
	}
	URTWM_NT_UNLOCK(sc);

	if (id > URTWM_MACID_MAX(sc)) {
		device_printf(sc->sc_dev, "%s: node table is full\n",
		    __func__);
		return;
	}

#ifndef URTWM_WITHOUT_UCODE
	/* Notify firmware. */
	id |= URTWM_MACID_VALID;
	urtwm_cmd_sleepable(sc, &id, sizeof(id), urtwm_set_media_status);
#endif
}

static void
urtwm_node_free(struct ieee80211_node *ni)
{
	struct urtwm_softc *sc = ni->ni_ic->ic_softc;
	struct urtwm_node *un = URTWM_NODE(ni);

	URTWM_NT_LOCK(sc);
	if (un->id != URTWM_MACID_UNDEFINED) {
		sc->node_list[un->id] = NULL;
#ifndef URTWM_WITHOUT_UCODE
		urtwm_cmd_sleepable(sc, &un->id, sizeof(un->id),
		    urtwm_set_media_status);
#endif
	}
	URTWM_NT_UNLOCK(sc);

	sc->sc_node_free(ni);
}

static void
urtwm_fix_spur(struct urtwm_softc *sc, struct ieee80211_channel *c)
{
	uint16_t chan = IEEE80211_CHAN2IEEE(c);

	if (!URTWM_CHIP_IS_12A(sc))
		return;

	if (sc->chip & URTWM_CHIP_12A_C_CUT) {
		if (IEEE80211_IS_CHAN_HT40(c) && chan == 11) {
			urtwm_bb_setbits(sc, R12A_RFMOD, 0, 0xc00);
			urtwm_bb_setbits(sc, R12A_ADC_BUF_CLK, 0, 0x40000000);
		} else {
			urtwm_bb_setbits(sc, R12A_RFMOD, 0x400, 0x800);

			if (!IEEE80211_IS_CHAN_HT40(c) &&	/* 20 MHz */
			    (chan == 13 || chan == 14)) {
				urtwm_bb_setbits(sc, R12A_RFMOD, 0, 0x300);
				urtwm_bb_setbits(sc, R12A_ADC_BUF_CLK,
				    0, 0x40000000);
			} else {	/* !80 Mhz */
				urtwm_bb_setbits(sc, R12A_RFMOD, 0x100, 0x200);
				urtwm_bb_setbits(sc, R12A_ADC_BUF_CLK,
				    0x40000000, 0);
			}
		}
	} else {
		/* Set ADC clock to 160M to resolve 2480 MHz spur. */
		if (!IEEE80211_IS_CHAN_HT40(c) &&	/* 20 MHz */
		    (chan == 13 || chan == 14))
			urtwm_bb_setbits(sc, R12A_RFMOD, 0, 0x300);
		else if (IEEE80211_IS_CHAN_2GHZ(c))
			urtwm_bb_setbits(sc, R12A_RFMOD, 0x100, 0x200);
	}
}

static void
urtwm_set_chan(struct urtwm_softc *sc, struct ieee80211_channel *c)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t val;
	uint16_t chan;
	int i;

	urtwm_set_band(sc, c, 0);

	chan = ieee80211_chan2ieee(ic, c);	/* XXX center freq! */
	KASSERT(chan != 0 && chan != IEEE80211_CHAN_ANY,
	    ("invalid channel %x\n", chan));

	if (36 <= chan && chan <= 48)
		val = 0x09280000;
	else if (50 <= chan && chan <= 64)
		val = 0x08a60000;
	else if (100 <= chan && chan <= 116)
		val = 0x08a40000;
	else if (118 <= chan)
		val = 0x08240000;
	else
		val = 0x12d40000;

	urtwm_bb_setbits(sc, R12A_FC_AREA, 0x1ffe0000, val);

	for (i = 0; i < sc->nrxchains; i++) {
		if (36 <= chan && chan <= 64)
			val = 0x10100;
		else if (100 <= chan && chan <= 140)
			val = 0x30100;
		else if (140 < chan)
			val = 0x50100;
		else
			val = 0x00000;

		urtwm_rf_setbits(sc, i, R92C_RF_CHNLBW, 0x70300, val);

		if (URTWM_CHIP_IS_12A(sc))
			urtwm_fix_spur(sc, c);

		KASSERT(chan <= 0xff, ("%s: chan %d\n", __func__, chan));
		urtwm_rf_setbits(sc, i, R92C_RF_CHNLBW, 0xff, chan);
	}

#ifdef notyet
	if (IEEE80211_IS_CHAN_HT80(c)) {	/* 80 MHz */
		urtwm_setbits_2(sc, R12A_WMAC_TRXPTCL_CTL, 0x80, 0x100);

		/* TODO */

		val = 0x0;
	} else
#endif
	if (IEEE80211_IS_CHAN_HT40(c)) {	/* 40 MHz */
		uint8_t ext_chan;

		if (IEEE80211_IS_CHAN_HT40U(c))
			ext_chan = R12A_DATA_SEC_PRIM_DOWN_20;
		else
			ext_chan = R12A_DATA_SEC_PRIM_UP_20;

		urtwm_setbits_2(sc, R12A_WMAC_TRXPTCL_CTL, 0x100, 0x80);
		urtwm_write_1(sc, R12A_DATA_SEC, ext_chan);

		urtwm_bb_setbits(sc, R12A_RFMOD, 0x003003c3, 0x00300201);
		urtwm_bb_setbits(sc, R12A_ADC_BUF_CLK, 0x40000000, 0);

		/* discard high 4 bits */
		val = urtwm_bb_read(sc, R12A_RFMOD);
		val = RW(val, R12A_RFMOD_EXT_CHAN, ext_chan);
		urtwm_bb_write(sc, R12A_RFMOD, val);

		val = urtwm_bb_read(sc, R12A_CCA_ON_SEC);
		val = RW(val, R12A_CCA_ON_SEC_EXT_CHAN, ext_chan);
		urtwm_bb_write(sc, R12A_CCA_ON_SEC, val);

		if (urtwm_read_1(sc, 0x837) & 0x04)
			val = 0x01800000;
		else if (sc->nrxchains == 2 && sc->ntxchains == 2)
			val = 0x01c00000;
		else
			val = 0x02000000;

		urtwm_bb_setbits(sc, R12A_L1_PEAK_TH, 0x03c00000, val);

		if (IEEE80211_IS_CHAN_HT40U(c))
			urtwm_bb_setbits(sc, R92C_CCK0_SYSTEM, 0x10, 0);
		else
			urtwm_bb_setbits(sc, R92C_CCK0_SYSTEM, 0, 0x10);

		val = 0x400;
	} else {	/* 20 MHz */
		urtwm_setbits_2(sc, R12A_WMAC_TRXPTCL_CTL, 0x180, 0);
		urtwm_write_1(sc, R12A_DATA_SEC, R12A_DATA_SEC_NO_EXT);

		urtwm_bb_setbits(sc, R12A_RFMOD, 0x003003c3, 0x00300200);
		urtwm_bb_setbits(sc, R12A_ADC_BUF_CLK, 0x40000000, 0);

		if (sc->nrxchains == 2 && sc->ntxchains == 2)
			val = 0x01c00000;
		else
			val = 0x02000000;

		urtwm_bb_setbits(sc, R12A_L1_PEAK_TH, 0x03c00000, val);

		val = 0xc00;
	}

	if (URTWM_CHIP_IS_12A(sc))
		urtwm_fix_spur(sc, c);

	for (i = 0; i < 2; i++)
		urtwm_rf_setbits(sc, i, R92C_RF_CHNLBW, 0xc00, val);

	/* Set Tx power for this new channel. */
	urtwm_set_txpower(sc, c);
}

static void
urtwm_antsel_init(struct urtwm_softc *sc)
{
	uint32_t reg;

	urtwm_write_1(sc, R92C_LEDCFG2, 0x82);
	urtwm_bb_setbits(sc, R92C_FPGA0_RFPARAM(0), 0, 0x2000);
	reg = urtwm_bb_read(sc, R92C_FPGA0_RFIFACEOE(0));
	sc->sc_ant = MS(reg, R12A_FPGA0_RFIFACEOE0_ANT);
}

static void
urtwm_iq_calib_sw(struct urtwm_softc *sc)
{
	/* TODO */
	URTWM_DPRINTF(sc, URTWM_DEBUG_CALIB, "%s: SW IQ calibration: TODO\n",
	    __func__);
}

#ifndef URTWM_WITHOUT_UCODE
static int
urtwm_iq_calib_fw_supported(struct urtwm_softc *sc)
{
	if (!(sc->sc_flags & URTWM_FW_LOADED))
		return (0);

	if (URTWM_CHIP_IS_12A(sc) && sc->fwver == 0x19)
		return (1);

	if (URTWM_CHIP_IS_21A(sc) && sc->fwver == 0x16)
		return (1);

	return (0);
}

static void
urtwm_iq_calib_fw(struct urtwm_softc *sc)
{
	struct ieee80211_channel *c = sc->sc_ic.ic_curchan;
	struct r12a_fw_cmd_iq_calib cmd;

	if (sc->sc_flags & URTWM_IQK_RUNNING)
		return;

	URTWM_DPRINTF(sc, URTWM_DEBUG_CALIB, "Starting IQ calibration (FW)\n");

	cmd.chan = IEEE80211_CHAN2IEEE(c);

	if (IEEE80211_IS_CHAN_5GHZ(c))
		cmd.band_bw = URTWM_CMD_IQ_BAND_5GHZ;
	else
		cmd.band_bw = URTWM_CMD_IQ_BAND_2GHZ;

	/* TODO: 80/160 MHz. */
	if (IEEE80211_IS_CHAN_HT40(c))
		cmd.band_bw |= URTWM_CMD_IQ_CHAN_WIDTH_40;
	else
		cmd.band_bw |= URTWM_CMD_IQ_CHAN_WIDTH_20;

	cmd.ext_5g_pa_lna = URTWM_CMD_IQ_EXT_PA_5G(sc->ext_pa_5g);
	cmd.ext_5g_pa_lna |= URTWM_CMD_IQ_EXT_LNA_5G(sc->ext_lna_5g);

	if (urtwm_fw_cmd(sc, R12A_CMD_IQ_CALIBRATE, &cmd, sizeof(cmd)) != 0) {
		URTWM_DPRINTF(sc, URTWM_DEBUG_CALIB,
		    "error while sending IQ calibration command to FW!\n");
		return;
	}

	sc->sc_flags |= URTWM_IQK_RUNNING;
}
#endif

static void
urtwm_iq_calib(struct urtwm_softc *sc)
{
#ifndef URTWM_WITHOUT_UCODE
	if (urtwm_iq_calib_fw_supported(sc))
		urtwm_iq_calib_fw(sc);
	else
#endif
		urtwm_iq_calib_sw(sc);
}

static void
urtwm_lc_calib(struct urtwm_softc *sc)
{
	uint32_t chnlbw;
	uint8_t txmode;

	URTWM_DPRINTF(sc, URTWM_DEBUG_CALIB, "%s: LC calibration started\n",
	    __func__);

	txmode = urtwm_read_1(sc, R12A_SINGLETONE_CONT_TX + 2);

	if ((txmode & 0x07) != 0) {
		/* Disable all continuous Tx. */
		/*
		 * Skipped because BB turns off continuous Tx until
		 * next packet comes in.
		 */
	} else {
		/* Block all Tx queues. */
		urtwm_write_1(sc, R92C_TXPAUSE, R92C_TX_QUEUE_ALL);
	}

	/* Enter LCK mode. */
	urtwm_rf_setbits(sc, 0, R12A_RF_LCK, 0, 0x4000);

	/* Start calibration. */
	chnlbw = urtwm_rf_read(sc, 0, R92C_RF_CHNLBW);
	urtwm_rf_write(sc, 0, R92C_RF_CHNLBW, chnlbw | R92C_RF_CHNLBW_LCSTART);

	/* Give calibration the time to complete. */
	urtwm_delay(sc, 150000);	/* 150 ms */

	/* Leave LCK mode. */
	urtwm_rf_setbits(sc, 0, R12A_RF_LCK, 0x4000, 0);

	/* Restore configuration. */
	if ((txmode & 0x07) != 0) {
		/* Continuous Tx case. */
		/*
		 * Skipped because BB turns off continuous Tx until
		 * next packet comes in.
		 */
	} else {
		/* Unblock all Tx queues. */
		urtwm_write_1(sc, R92C_TXPAUSE, 0);
	}

	/* Recover channel number. */
	urtwm_rf_write(sc, 0, R92C_RF_CHNLBW, chnlbw);

	URTWM_DPRINTF(sc, URTWM_DEBUG_CALIB, "%s: LC calibration finished\n",
	    __func__);
}

static void
urtwm_temp_calib(struct urtwm_softc *sc)
{
	uint8_t temp;

	URTWM_ASSERT_LOCKED(sc);

	if (!(sc->sc_flags & URTWM_TEMP_MEASURED)) {
		/* Start measuring temperature. */
		URTWM_DPRINTF(sc, URTWM_DEBUG_TEMP,
		    "%s: start measuring temperature\n", __func__);
		urtwm_rf_write(sc, 0, R88E_RF_T_METER, R88E_RF_T_METER_START);
		sc->sc_flags |= URTWM_TEMP_MEASURED;
		return;
	}
	sc->sc_flags &= ~URTWM_TEMP_MEASURED;

	/* Read measured temperature. */
	temp = MS(urtwm_rf_read(sc, 0, R88E_RF_T_METER), R88E_RF_T_METER_VAL);
	if (temp == 0) {	/* Read failed, skip. */
		URTWM_DPRINTF(sc, URTWM_DEBUG_TEMP,
		    "%s: temperature read failed, skipping\n", __func__);
		return;
	}

	URTWM_DPRINTF(sc, URTWM_DEBUG_TEMP,
	    "temperature: previous %u, current %u\n",
	    sc->thcal_temp, temp);

	/*
	 * Redo LC/IQ calibration if temperature changed significantly since
	 * last calibration.
	 */
	if (sc->thcal_temp == 0xff) {
		/* efuse value is absent; do LCK at initial status. */
		if (!URTWM_CHIP_IS_21A(sc))
			urtwm_lc_calib(sc);

		sc->thcal_temp = temp;
	} else if (abs(temp - sc->thcal_temp) > URTWM_CALIB_THRESHOLD) {
		URTWM_DPRINTF(sc, URTWM_DEBUG_TEMP,
		    "%s: LC/IQ calib triggered by temp: %u -> %u\n",
		    __func__, sc->thcal_temp, temp);

		if (!URTWM_CHIP_IS_21A(sc))
			urtwm_lc_calib(sc);
		urtwm_iq_calib(sc);

		/* Record temperature of last calibration. */
		sc->thcal_temp = temp;
	}
}

static int
urtwm_init(struct urtwm_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211vap *vap = TAILQ_FIRST(&ic->ic_vaps);
	struct urtwm_vap *uvp = URTWM_VAP(vap);
	usb_error_t usb_err = USB_ERR_NORMAL_COMPLETION;
	int i, error;

	URTWM_LOCK(sc);
	if (sc->sc_flags & URTWM_RUNNING) {
		URTWM_UNLOCK(sc);
		return (0);
	}

	/* Allocate Tx/Rx buffers. */
	error = urtwm_alloc_rx_list(sc);
	if (error != 0)
		goto fail;

	error = urtwm_alloc_tx_list(sc);
	if (error != 0)
		goto fail;

	if (URTWM_CHIP_IS_12A(sc)) {
		urtwm_write_1(sc, R92C_RF_CTRL,
		    R92C_RF_CTRL_EN |
		    R92C_RF_CTRL_SDMRSTB);
		urtwm_write_1(sc, R92C_RF_CTRL,
		    R92C_RF_CTRL_EN |
		    R92C_RF_CTRL_RSTB |
		    R92C_RF_CTRL_SDMRSTB);
		urtwm_write_1(sc, R12A_RF_B_CTRL,
		    R92C_RF_CTRL_EN |
		    R92C_RF_CTRL_SDMRSTB);
		urtwm_write_1(sc, R12A_RF_B_CTRL,
		    R92C_RF_CTRL_EN |
		    R92C_RF_CTRL_RSTB |
		    R92C_RF_CTRL_SDMRSTB);
	}

	/* Power on adapter. */
	error = urtwm_power_on(sc);
	if (error != 0)
		goto fail;

#ifndef URTWM_WITHOUT_UCODE
	/* Load 8051 microcode. */
	error = urtwm_load_firmware(sc);
	if (error == 0)
		sc->sc_flags |= URTWM_FW_LOADED;

	/* Init firmware commands ring. */
	sc->fwcur = 0;
#endif

	/* Initialize MAC block. */
	error = urtwm_mac_init(sc);
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "%s: error while initializing MAC block\n", __func__);
		goto fail;
	}

	/* Initialize DMA. */
	error = urtwm_dma_init(sc);
	if (error != 0)
		goto fail;

	/* Drop incorrect TX. */
	urtwm_setbits_1_shift(sc, R92C_TXDMA_OFFSET_CHK, 0,
	    R92C_TXDMA_OFFSET_DROP_DATA_EN, 1);

	/* Set info size in Rx descriptors (in 64-bit words). */
	urtwm_write_1(sc, R92C_RX_DRVINFO_SZ, R92C_RX_DRVINFO_SZ_DEF);

	/* Init interrupts. */
	urtwm_write_4(sc, R88E_HIMR, 0);
	urtwm_write_4(sc, R88E_HIMRE, 0);

	/* Set MAC address. */
	usb_err = urtwm_write_region_1(sc, R92C_MACID, vap->iv_myaddr,
	    IEEE80211_ADDR_LEN);
	if (usb_err != USB_ERR_NORMAL_COMPLETION)
		goto fail;

	/* Set initial network type. */
	urtwm_set_mode(sc, R92C_MSR_NOLINK, 0);

	/* Initialize Rx filter. */
	urtwm_rxfilter_init(sc);

	/* Set response rate. */
	urtwm_setbits_4(sc, R92C_RRSR, R92C_RRSR_RATE_BITMAP_M,
	    R92C_RRSR_RATE_CCK_ONLY_1M);

	/* Set short/long retry limits. */
	urtwm_write_2(sc, R92C_RL,
	    SM(R92C_RL_SRL, 0x30) | SM(R92C_RL_LRL, 0x30));

	/* Initialize EDCA parameters. */
	urtwm_edca_init(sc);

	urtwm_setbits_1(sc, R92C_FWHW_TXQ_CTRL, 0,
	    R92C_FWHW_TXQ_CTRL_AMPDU_RTY_NEW);
	/* Set ACK timeout. */
	urtwm_write_1(sc, R92C_ACKTO, 0x80);

	/* Setup USB aggregation. */
	/* Tx aggregation. */
	urtwm_setbits_4(sc, R92C_TDECTRL,
	    R92C_TDECTRL_BLK_DESC_NUM_M, sc->tx_agg_desc_num);
	if (URTWM_CHIP_HAS_BCNQ1(sc))
		urtwm_write_1(sc, R12A_DWBCN1_CTRL, sc->tx_agg_desc_num << 1);

	/* Rx aggregation (USB). */
	urtwm_write_2(sc, R92C_RXDMA_AGG_PG_TH,
	    sc->ac_usb_dma_size | (sc->ac_usb_dma_time << 8));
	urtwm_setbits_1(sc, R92C_TRXDMA_CTRL, 0,
	    R92C_TRXDMA_CTRL_RXDMA_AGG_EN);

	/* Initialize beacon parameters. */
	urtwm_write_2(sc, R92C_BCN_CTRL, 0x1010);
	urtwm_write_2(sc, R92C_TBTT_PROHIBIT, 0x6404);
	urtwm_write_1(sc, R92C_DRVERLYINT, 0x05);
	urtwm_write_1(sc, R92C_BCNDMATIM, 0x02);
	urtwm_write_2(sc, R92C_BCNTCFG, 0x660f);

	/* Rx interval (USB3). */
	urtwm_write_1(sc, 0xf050, 0x01);

	/* burst length = 4 */
	urtwm_write_2(sc, R92C_RXDMA_STATUS, 0x7400);

	urtwm_write_1(sc, R92C_RXDMA_STATUS + 1, 0xf5);

	/* Setup AMPDU aggregation. */
	if (URTWM_CHIP_IS_12A(sc))
		urtwm_write_1(sc, R12A_AMPDU_MAX_TIME, 0x70);
	else
		urtwm_write_1(sc, R12A_AMPDU_MAX_TIME, 0x5e);
	urtwm_write_4(sc, R12A_AMPDU_MAX_LENGTH, 0xffffffff);

	/* 80 MHz clock (again?) */
	urtwm_write_1(sc, R92C_USTIME_TSF, 0x50);
	urtwm_write_1(sc, R92C_USTIME_EDCA, 0x50);

	if (URTWM_CHIP_IS_21A(sc) ||
	    urtwm_read_1(sc, R92C_TYPE_ID + 3) & 0x80)	{
		if ((urtwm_read_1(sc, R92C_USB_INFO) & 0x30) == 0) {
			/* Set burst packet length to 512 B. */
			urtwm_setbits_1(sc, R12A_RXDMA_PRO, 0x20, 0x1e);
		} else {
			/* Set burst packet length to 64 B. */
			urtwm_setbits_1(sc, R12A_RXDMA_PRO, 0x10, 0x2e);
		}
	} else {	/* USB 3.0 */
		/* Set burst packet length to 1 KB. */
		urtwm_setbits_1(sc, R12A_RXDMA_PRO, 0x30, 0x0e);

		urtwm_setbits_1(sc, 0xf008, 0x18, 0);
	}

	/* Enable single packet AMPDU. */
	urtwm_setbits_1(sc, R12A_HT_SINGLE_AMPDU, 0,
	    R12A_HT_SINGLE_AMPDU_PKT_ENA);

	/* 11K packet length for VHT. */
	urtwm_write_1(sc, R92C_RX_PKT_LIMIT, 0x18);

	urtwm_write_1(sc, R92C_PIFS, 0);

	urtwm_write_2(sc, R92C_MAX_AGGR_NUM, 0x1f1f);

	if (URTWM_CHIP_IS_12A(sc)) {
		urtwm_setbits_1(sc, R92C_FWHW_TXQ_CTRL,
		    R92C_FWHW_TXQ_CTRL_AMPDU_RTY_NEW, 0);
	} else {
		urtwm_write_1(sc, R92C_FWHW_TXQ_CTRL,
		    R92C_FWHW_TXQ_CTRL_AMPDU_RTY_NEW);
		urtwm_write_4(sc, R92C_FAST_EDCA_CTRL, 0x03087777);
	}

	/* Do not reset MAC. */
	urtwm_setbits_1(sc, R92C_RSV_CTRL, 0, 0x60);

	urtwm_arfb_init(sc);

	/* Init MACTXEN / MACRXEN after setting RxFF boundary. */
	urtwm_setbits_1(sc, R92C_CR, 0, R92C_CR_MACTXEN | R92C_CR_MACRXEN);

	/* Initialize BB/RF blocks. */
	urtwm_bb_init(sc);
	urtwm_rf_init(sc);

	/* Initialize wireless band. */
	urtwm_set_band(sc, ic->ic_curchan, 1);

	/* Clear per-station keys table. */
	urtwm_cam_init(sc);

	/* Enable decryption / encryption. */
	urtwm_write_2(sc, R92C_SECCFG,
	    R92C_SECCFG_TXUCKEY_DEF | R92C_SECCFG_RXUCKEY_DEF |
	    R92C_SECCFG_TXENC_ENA | R92C_SECCFG_RXDEC_ENA |
	    R92C_SECCFG_TXBCKEY_DEF | R92C_SECCFG_RXBCKEY_DEF);

	/* Initialize antenna selection. */
	urtwm_antsel_init(sc);

	/* Enable hardware sequence numbering. */
	urtwm_write_1(sc, R92C_HWSEQ_CTRL, R92C_TX_QUEUE_ALL);

	/* Disable BAR. */
	urtwm_write_4(sc, R92C_BAR_MODE_CTRL, 0x0201ffff);

	/* NAV limit. */
	urtwm_write_1(sc, R92C_NAV_UPPER, 0);

	/* Initialize GPIO setting. */
	urtwm_setbits_1(sc, R92C_GPIO_MUXCFG, R92C_GPIO_MUXCFG_ENBT, 0);

	/* Setup RTS BW (equal to data BW). */
	urtwm_setbits_1(sc, R92C_QUEUE_CTRL, 0x08, 0);

	urtwm_write_1(sc, R12A_EARLY_MODE_CONTROL + 3, 0x01);

	/* Initialize MRR. */
	urtwm_mrr_init(sc);

	/* Reset USB mode switch setting. */
	urtwm_write_1(sc, R12A_SDIO_CTRL, 0);
	urtwm_write_1(sc, R92C_ACLK_MON, 0);

	urtwm_write_1(sc, R92C_USB_HRPWM, 0);

	usbd_transfer_start(sc->sc_xfer[URTWM_BULK_RX]);

	sc->sc_flags |= URTWM_RUNNING;

	/*
	 * Install static keys (if any).
	 * Must be called after urtwm_cam_init().
	 */
	for (i = 0; i < IEEE80211_WEP_NKID; i++) {
		const struct ieee80211_key *k = uvp->keys[i];
		if (k != NULL) {
			urtwm_cmd_sleepable(sc, k, sizeof(*k),
			    urtwm_key_set_cb);
		}
	}
fail:
	if (usb_err != USB_ERR_NORMAL_COMPLETION)
		error = EIO;

	URTWM_UNLOCK(sc);

	return (error);
}

static void
urtwm_stop(struct urtwm_softc *sc)
{

	URTWM_LOCK(sc);
	if (!(sc->sc_flags & URTWM_RUNNING)) {
		URTWM_UNLOCK(sc);
		return;
	}

	sc->sc_flags &= ~(URTWM_RUNNING | URTWM_FW_LOADED);
	sc->sc_flags &= ~(URTWM_TEMP_MEASURED | URTWM_IQK_RUNNING);
	sc->fwver = 0;
	sc->thcal_temp = 0;

	urtwm_abort_xfers(sc);
	urtwm_drain_mbufq(sc);
	urtwm_free_tx_list(sc);
	urtwm_free_rx_list(sc);
	urtwm_power_off(sc);
	URTWM_UNLOCK(sc);
}

static void
urtwm_abort_xfers(struct urtwm_softc *sc)
{
	int i;

	URTWM_ASSERT_LOCKED(sc);

	/* abort any pending transfers */
	for (i = 0; i < URTWM_N_TRANSFER; i++)
		usbd_transfer_stop(sc->sc_xfer[i]);
}

static int
urtwm_raw_xmit(struct ieee80211_node *ni, struct mbuf *m,
    const struct ieee80211_bpf_params *params)
{
	struct ieee80211com *ic = ni->ni_ic;
	struct urtwm_softc *sc = ic->ic_softc;
	struct urtwm_data *bf;
	int error;

	URTWM_DPRINTF(sc, URTWM_DEBUG_XMIT, "%s: called; m %p, ni %p\n",
	    __func__, m, ni);

	/* prevent management frames from being sent if we're not ready */
	URTWM_LOCK(sc);
	if (!(sc->sc_flags & URTWM_RUNNING)) {
		error = ENETDOWN;
		goto end;
	}

	bf = urtwm_getbuf(sc);
	if (bf == NULL) {
		error = ENOBUFS;
		goto end;
	}

	if (params == NULL) {
		/*
		 * Legacy path; interpret frame contents to decide
		 * precisely how to send the frame.
		 */
		error = urtwm_tx_data(sc, ni, m, bf);
	} else {
		/*
		 * Caller supplied explicit parameters to use in
		 * sending the frame.
		 */
		error = urtwm_tx_raw(sc, ni, m, bf, params);
	}
	if (error != 0) {
		STAILQ_INSERT_HEAD(&sc->sc_tx_inactive, bf, next);
		goto end;
	}

end:
	if (error != 0)
		m_freem(m);

	URTWM_UNLOCK(sc);
	
	return (error);
}

static void
urtwm_delay(struct urtwm_softc *sc, int usec)
{
	/* No, 1ms delay is too big. */
	if (usec < 1000)
		DELAY(usec);
	else
		usb_pause_mtx(&sc->sc_mtx, USB_MS_TO_TICKS(usec / 1000 + 1));
}

static device_method_t urtwm_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		urtwm_match),
	DEVMETHOD(device_attach,	urtwm_attach),
	DEVMETHOD(device_detach,	urtwm_detach),

	DEVMETHOD_END
};

static driver_t urtwm_driver = {
	"urtwm",
	urtwm_methods,
	sizeof(struct urtwm_softc)
};

static devclass_t urtwm_devclass;

DRIVER_MODULE(urtwm, uhub, urtwm_driver, urtwm_devclass, NULL, NULL);
MODULE_DEPEND(urtwm, usb, 1, 1, 1);
MODULE_DEPEND(urtwm, wlan, 1, 1, 1);
#ifndef URTWM_WITHOUT_UCODE
MODULE_DEPEND(urtwm, firmware, 1, 1, 1);
#endif
MODULE_VERSION(urtwm, 1);
USB_PNP_HOST_INFO(urtwm_devs);
