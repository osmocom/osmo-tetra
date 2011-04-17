
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <osmocom/core/msgb.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/bits.h>
#include <netinet/in.h>

#include "tetra_common.h"
#include "tetra_tdma.h"

static int gsmtap_fd = -1;

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


struct msgb *tetra_gsmtap_makemsg(struct tetra_tdma_time *tm, enum tetra_log_chan lchan,
				  uint8_t ts, uint8_t ss, int8_t signal_dbm,
				  uint8_t snr, const ubit_t *bitdata, unsigned int bitlen)
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
	if (gsmtap_fd != -1)
		return write(gsmtap_fd, msg->data, msg->len);
	else
		return 0;
}

/* this block should move to libosmocore */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

int gsmtap_init(const char *host, uint16_t port)
{
	struct addrinfo hints, *result, *rp;
	int sfd, rc;
	char portbuf[16];

	if (port == 0)
		port = GSMTAP_UDP_PORT;
	if (host == NULL)
		host = "localhost";

	sprintf(portbuf, "%u", port);
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = IPPROTO_UDP;

	rc = getaddrinfo(host, portbuf, &hints, &result);
	if (rc != 0) {
		perror("getaddrinfo returned NULL");
		return -EINVAL;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		printf("creating socket %u, %u, %u\n", rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;
		close(sfd);
	}
	freeaddrinfo(result);

	if (rp == NULL) {
		perror("unable to bind to socket");
		return -ENODEV;
	}

	/* FIXME: if host == localhost, bind to another socket and discard data */

	return sfd;
}
/* end block for libosmocore */

int tetra_gsmtap_init(const char *host, uint16_t port)
{
	int fd;

	fd = gsmtap_init(host, port);
	if (fd < 0)
		return fd;

	gsmtap_fd = fd;

	return 0;
}
