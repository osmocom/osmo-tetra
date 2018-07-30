#ifndef TETRA_GSMTAP_H
#define TETRA_GSMTAP_H
#include "tetra_common.h"

struct msgb *tetra_gsmtap_makemsg(struct tetra_tdma_time *tm, enum tetra_log_chan lchan, uint8_t ts, uint8_t ss,
				  int8_t signal_dbm, uint8_t snr, const uint8_t *bitdata, unsigned int bitlen,
				  struct tetra_mac_state *tms);

int tetra_gsmtap_sendmsg(struct msgb *msg);

int tetra_gsmtap_init(const char *host, uint16_t port);

#endif
