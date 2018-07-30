
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/gsmtap_util.h>
#include <osmocom/core/bits.h>
#include <netinet/in.h>

#include "tetra_common.h"
#include "tetra_tdma.h"

static struct gsmtap_inst *g_gti = NULL;

static const uint8_t lchan2gsmtap[] = {
	[TETRA_LC_SCH_F]	= GSMTAP_TETRA_SCH_F,
	[TETRA_LC_SCH_HD]	= GSMTAP_TETRA_SCH_HD,
	[TETRA_LC_SCH_HU]	= GSMTAP_TETRA_SCH_HU,
	[TETRA_LC_STCH]		= GSMTAP_TETRA_STCH,
	[TETRA_LC_AACH]		= GSMTAP_TETRA_AACH,
	[TETRA_LC_TCH]		= GSMTAP_TETRA_TCH_F,
	[TETRA_LC_BSCH]		= GSMTAP_TETRA_BSCH,
	[TETRA_LC_BNCH]		= GSMTAP_TETRA_BNCH,
};


struct msgb *tetra_gsmtap_makemsg(struct tetra_tdma_time *tm, enum tetra_log_chan lchan, uint8_t ts, uint8_t ss,
				  int8_t signal_dbm, uint8_t snr, const ubit_t *bitdata, unsigned int bitlen,
				  struct tetra_mac_state *tms)
{
	struct msgb *msg;
	struct gsmtap_hdr *gh;
	uint32_t fn = tetra_tdma_time2fn(tm);
	unsigned int packed_len = osmo_pbit_bytesize(bitlen);
	uint8_t *dst;

	msg = msgb_alloc(sizeof(*gh) + packed_len, "tetra_gsmtap_tx");
	if (!msg)
		return NULL;

	gh = (struct gsmtap_hdr *) msgb_put(msg, sizeof(*gh));
	gh->version = GSMTAP_VERSION;
	gh->hdr_len = sizeof(*gh)/4;
	gh->type = GSMTAP_TYPE_TETRA_I1;
	gh->timeslot = ts;
	tms->tsn = ts;
	gh->sub_slot = ss;
	gh->snr_db = snr;
	gh->signal_dbm = signal_dbm;
	gh->frame_number = htonl(fn);
	gh->sub_type = lchan2gsmtap[lchan];
	gh->antenna_nr = 0;

	/* convert from 1bit-per-byte to compressed bits!!! */
	dst = msgb_put(msg, packed_len);
	osmo_ubit2pbit(dst, bitdata, bitlen);

	return msg;
}

int tetra_gsmtap_sendmsg(struct msgb *msg)
{
	if (g_gti)
		return gsmtap_sendmsg(g_gti, msg);
	else
		return 0;
}

int tetra_gsmtap_init(const char *host, uint16_t port)
{
	g_gti = gsmtap_source_init(host, port, 0);
	if (!g_gti)
		return -EINVAL;
	gsmtap_source_add_sink(g_gti);

	return 0;
}
