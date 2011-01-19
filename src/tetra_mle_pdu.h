#ifndef TETRA_MLE_PDU_H
#define TETRA_MLE_PDU_H

#include <stdint.h>

/* 18.5.21 */
enum tetra_mle_pdisc {
	TMLE_PDISC_MM		= 1,
	TMLE_PDISC_CMCE		= 2,
	TMLE_PDISC_SNDCP	= 4,
	TMLE_PDUSC_MLE		= 5,
	TMLE_PDISC_MGMT		= 6,
	TMLE_PDISC_TEST		= 7,
};
const char *tetra_get_mle_pdisc_name(uint8_t pdisc);

#endif
