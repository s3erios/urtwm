/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
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

#define URTWM_RX_LIST_COUNT		1
#define URTWM_TX_LIST_COUNT		16
#define URTWM_HOST_CMD_RING_COUNT	32

#define URTWM_RXBUFSZ	(8 * 1024)
#define URTWM_TXBUFSZ	(sizeof(struct r88a_tx_desc) + IEEE80211_MAX_LEN)

#define URTWM_TX_TIMEOUT	5000	/* ms */

#define URTWM_LED_LINK	0
#define URTWM_LED_DATA	1

struct urtwm_rx_radiotap_header {
	struct ieee80211_radiotap_header wr_ihdr;
	uint64_t	wr_tsft;
	uint8_t		wr_flags;
	uint8_t		wr_rate;
	uint16_t	wr_chan_freq;
	uint16_t	wr_chan_flags;
	int8_t		wr_dbm_antsignal;
	int8_t		wr_dbm_antnoise;
} __packed __aligned(8);

#define URTWM_RX_RADIOTAP_PRESENT			\
	(1 << IEEE80211_RADIOTAP_TSFT |			\
	 1 << IEEE80211_RADIOTAP_FLAGS |		\
	 1 << IEEE80211_RADIOTAP_RATE |			\
	 1 << IEEE80211_RADIOTAP_CHANNEL |		\
	 1 << IEEE80211_RADIOTAP_DBM_ANTSIGNAL |	\
	 1 << IEEE80211_RADIOTAP_DBM_ANTNOISE)

struct urtwm_tx_radiotap_header {
	struct ieee80211_radiotap_header wt_ihdr;
	uint8_t		wt_flags;
	uint16_t	wt_chan_freq;
	uint16_t	wt_chan_flags;
} __packed __aligned(8);

#define URTWM_TX_RADIOTAP_PRESENT			\
	(1 << IEEE80211_RADIOTAP_FLAGS |		\
	 1 << IEEE80211_RADIOTAP_CHANNEL)

struct urtwm_data {
	uint8_t				*buf;
	uint16_t			buflen;
	struct mbuf			*m;
	struct ieee80211_node		*ni;
	STAILQ_ENTRY(urtwm_data)	next;
};
typedef STAILQ_HEAD(, urtwm_data) urtwm_datahead;

struct urtwm_softc;

union sec_param {
	struct ieee80211_key		key;
};

#define CMD_FUNC_PROTO			void (*func)(struct urtwm_softc *, \
					    union sec_param *)

struct urtwm_cmdq {
	union sec_param			data;
	CMD_FUNC_PROTO;
};
#define URTWM_CMDQ_SIZE			16

struct urtwm_node {
	struct ieee80211_node	ni;	/* must be the first */
	uint8_t			id;
	int			last_rssi;
};
#define URTWM_NODE(ni)	((struct urtwm_node *)(ni))

struct urtwm_vap {
	struct ieee80211vap	vap;

	struct r88a_tx_desc	bcn_desc;
	struct mbuf		*bcn_mbuf;

	struct callout		tsf_sync_adhoc;
	struct task		tsf_sync_adhoc_task;

	const struct ieee80211_key	*keys[IEEE80211_WEP_NKID];

	int			(*newstate)(struct ieee80211vap *,
				    enum ieee80211_state, int);
	void			(*recv_mgmt)(struct ieee80211_node *,
				    struct mbuf *, int,
				    const struct ieee80211_rx_stats *,
				    int, int);
};
#define	URTWM_VAP(vap)	((struct urtwm_vap *)(vap))

enum {
	URTWM_BULK_RX,
	URTWM_BULK_TX_BE,	/* = WME_AC_BE */
	URTWM_BULK_TX_BK,	/* = WME_AC_BK */
	URTWM_BULK_TX_VI,	/* = WME_AC_VI */
	URTWM_BULK_TX_VO,	/* = WME_AC_VO */
	URTWM_N_TRANSFER = 5,
};

#define	URTWM_EP_QUEUES	URTWM_BULK_RX

struct urtwm_softc {
	struct ieee80211com	sc_ic;
	struct mbufq		sc_snd;
	device_t		sc_dev;
	struct usb_device	*sc_udev;

#define URTWM_USE_RATECTL(_sc)	!!((_sc)->sc_flags & URTWM_FW_LOADED)

	uint32_t		sc_debug;
	uint8_t			sc_iface_index;
	uint8_t			sc_flags;
#define URTWM_DETACHED	0x01
#define URTWM_RUNNING	0x02
#define URTWM_FW_LOADED	0x08

	uint8_t			pa_type;
	uint8_t			lna_type;
	uint8_t			regulatory;
	uint8_t			tx_bbswing_2g;
	uint8_t			tx_bbswing_5g;

	uint8_t	cck_tx_pwr[URTWM_MAX_RF_PATH][URTWM_MAX_GROUP_2G];
	uint8_t	ht40_tx_pwr_2g[URTWM_MAX_RF_PATH][URTWM_MAX_GROUP_2G];
	uint8_t	ht40_tx_pwr_5g[URTWM_MAX_RF_PATH][URTWM_MAX_GROUP_5G];

	uint8_t cck_tx_pwr_diff_2g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];
	uint8_t	ofdm_tx_pwr_diff_2g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];
	uint8_t	bw20_tx_pwr_diff_2g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];
	uint8_t	bw40_tx_pwr_diff_2g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];

	uint8_t	ofdm_tx_pwr_diff_5g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];
	uint8_t	bw20_tx_pwr_diff_5g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];
	uint8_t	bw40_tx_pwr_diff_5g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];
	uint8_t	bw80_tx_pwr_diff_5g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];
	uint8_t	bw160_tx_pwr_diff_5g[URTWM_MAX_RF_PATH][URTWM_MAX_TX_COUNT];

	int			ntxchains;
        int			nrxchains;
	int			ntx;
	int			ledlink;
	int			sc_ant;

	int			fwcur;
	struct urtwm_data	sc_rx[URTWM_RX_LIST_COUNT];
	urtwm_datahead		sc_rx_active;
	urtwm_datahead		sc_rx_inactive;
	struct urtwm_data	sc_tx[URTWM_TX_LIST_COUNT];
	urtwm_datahead		sc_tx_active;
	int			sc_tx_n_active;
	urtwm_datahead		sc_tx_inactive;
	urtwm_datahead		sc_tx_pending;

	uint16_t		next_rom_addr;
	uint32_t		keys_bmap;

	struct ieee80211_node	*node_list[R8821A_MACID_MAX + 1];
	struct mtx		nt_mtx;

	struct mtx		sc_mtx;

	struct urtwm_cmdq	cmdq[URTWM_CMDQ_SIZE];
        struct mtx		cmdq_mtx;
        struct task		cmdq_task;
        uint8_t			cmdq_first;
        uint8_t			cmdq_last;

	struct usb_xfer		*sc_xfer[URTWM_N_TRANSFER];

	struct wmeParams	cap_wmeParams[WME_NUM_AC];

	struct urtwm_rx_radiotap_header	sc_rxtap;
	struct urtwm_tx_radiotap_header	sc_txtap;

	void		(*sc_node_free)(struct ieee80211_node *);
	void		(*sc_scan_curchan)(struct ieee80211_scan_state *,
			    unsigned long);
};

#define	URTWM_LOCK(sc)			mtx_lock(&(sc)->sc_mtx)
#define	URTWM_UNLOCK(sc)		mtx_unlock(&(sc)->sc_mtx)
#define	URTWM_ASSERT_LOCKED(sc)		mtx_assert(&(sc)->sc_mtx, MA_OWNED)

#define URTWM_CMDQ_LOCK_INIT(sc) \
	mtx_init(&(sc)->cmdq_mtx, "cmdq lock", NULL, MTX_DEF)
#define URTWM_CMDQ_LOCK(sc)		mtx_lock(&(sc)->cmdq_mtx)
#define URTWM_CMDQ_UNLOCK(sc)		mtx_unlock(&(sc)->cmdq_mtx)
#define URTWM_CMDQ_LOCK_DESTROY(sc)	mtx_destroy(&(sc)->cmdq_mtx)

#define URTWM_NT_LOCK_INIT(sc) \
	mtx_init(&(sc)->nt_mtx, "node table lock", NULL, MTX_DEF)
#define URTWM_NT_LOCK(sc)		mtx_lock(&(sc)->nt_mtx)
#define URTWM_NT_UNLOCK(sc)		mtx_unlock(&(sc)->nt_mtx)
#define URTWM_NT_LOCK_DESTROY(sc)	mtx_destroy(&(sc)->nt_mtx)
