/* Implementation of some PDU parsing of the TETRA LLC */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <osmocom/core/utils.h>

#include "tetra_common.h"
#include "tetra_llc_pdu.h"

static const struct value_string tetra_llc_pdut_names[] = {
	{ TLLC_PDUT_BL_ADATA,		"BL-ADATA" },
	{ TLLC_PDUT_BL_DATA,		"BL-DATA" },
	{ TLLC_PDUT_BL_UDATA,		"BL-UDATA" },
	{ TLLC_PDUT_BL_ACK,		"BL-ACK" },
	{ TLLC_PDUT_BL_ADATA_FCS,	"BL-ADATA-FCS" },
	{ TLLC_PDUT_BL_DATA_FCS,	"BL-DATA-FCS" },
	{ TLLC_PDUT_BL_UDATA_FCS,	"BL-UDATA-FCS" },
	{ TLLC_PDUT_BL_ACK_FCS,		"BL-ACK-FCS" },
	{ TLLC_PDUT_AL_SETUP,		"AL-SETUP" },
	{ TLLC_PDUT_AL_DATA_FINAL,	"AL-DATA/FINAL" },
	{ TLLC_PDUT_AL_UDATA_UFINAL,	"AL-UDATA/FINAL" },
	{ TLLC_PDUT_AL_ACK_RNR,		"AL-ACK/AL-RNR" },
	{ TLLC_PDUT_AL_RECONNECT,	"AL-RECONNECT" },
	{ TLLC_PDUT_SUPPL,		"AL-SUPPLEMENTARY" },
	{ TLLC_PDUT_L2SIG,		"AL-L2SIG" },
	{ TLLD_PDUT_AL_DISC,		"AL-DISC" },
	{ 0, NULL }
};
const char *tetra_get_llc_pdut_name(uint8_t pdut)
{
	return get_value_string(tetra_llc_pdut_names, pdut);
}

static const struct value_string pdut_dec_names[] = {
	{ TLLC_PDUT_DEC_BL_ADATA,	"BL-ADATA" },
	{ TLLC_PDUT_DEC_BL_DATA,	"BL-DATA" },
	{ TLLC_PDUT_DEC_BL_UDATA,	"BL-UDATA" },
	{ TLLC_PDUT_DEC_BL_ACK,		"BL-ACK" },
	{ TLLC_PDUT_DEC_AL_SETUP,	"AL-SETUP" },
	{ TLLC_PDUT_DEC_AL_DATA,	"AL-DATA" },
	{ TLLC_PDUT_DEC_AL_FINAL,	"AL-FINAL" },
	{ TLLC_PDUT_DEC_AL_UDATA,	"AL-UDATA" },
	{ TLLC_PDUT_DEC_AL_UFINAL,	"AL-UFINAL" },
	{ TLLC_PDUT_DEC_AL_ACK,		"AL-ACK" },
	{ TLLC_PDUT_DEC_AL_RNR,		"AL-RNR" },
	{ TLLC_PDUT_DEC_AL_RECONNECT,	"AL-RECONNECT" },
	{ TLLC_PDUT_DEC_AL_DISC,	"AL-DISC" },
	{ TLLC_PDUT_DEC_ALX_DATA,	"ALX-DATA" },
	{ TLLC_PDUT_DEC_ALX_FINAL,	"ALX-FINAL" },
	{ TLLC_PDUT_DEC_ALX_UDATA,	"ALX-UDATA" },
	{ TLLC_PDUT_DEC_ALX_UFINAL,	"ALX-UFINAL" },
	{ TLLC_PDUT_DEC_ALX_ACK,	"ALX-ACK" },
	{ TLLC_PDUT_DEC_ALX_RNR,	"ALX-RNR" },
	{ 0, NULL }
};

const char *tetra_get_llc_pdut_dec_name(enum tllc_pdut_dec pdut)
{
	return get_value_string(pdut_dec_names, pdut);
}

int tetra_llc_pdu_parse(struct tetra_llc_pdu *lpp, uint8_t *buf, int len)
{
	uint8_t *cur = buf;
	uint8_t pdu_type;

	pdu_type = bits_to_uint(cur, 4);
	cur += 4;

	switch (pdu_type) {
	case TLLC_PDUT_BL_ADATA_FCS:
		/* FIXME */
		len -= 32;
	case TLLC_PDUT_BL_ADATA:
		lpp->nr = *cur++;
		lpp->ns = *cur++;
		lpp->tl_sdu = cur;
		lpp->tl_sdu_len = len - (cur - buf);
		lpp->pdu_type = TLLC_PDUT_DEC_BL_ADATA;
		break;
	case TLLC_PDUT_BL_DATA_FCS:
		/* FIXME */
		len -= 32;
	case TLLC_PDUT_BL_DATA:
		lpp->ns = *cur++;
		lpp->tl_sdu = cur;
		lpp->tl_sdu_len = len - (cur - buf);
		lpp->pdu_type = TLLC_PDUT_DEC_BL_DATA;
		break;
	case TLLC_PDUT_BL_UDATA_FCS:
		/* FIXME */
		len -= 32;
	case TLLC_PDUT_BL_UDATA:
		lpp->tl_sdu = cur;
		lpp->tl_sdu_len = len - (cur - buf);
		lpp->pdu_type = TLLC_PDUT_DEC_BL_UDATA;
		break;
	case TLLC_PDUT_AL_DATA_FINAL:
		if (*cur++) {
			/* FINAL */
			cur++;
			lpp->ns = bits_to_uint(cur, 3); cur += 3;
			lpp->ss = bits_to_uint(cur, 8); cur += 8;
			if (*cur++) {
				/* FIXME: FCS */
				len -= 32;
			}
			lpp->tl_sdu = cur;
			lpp->tl_sdu_len = len - (cur - buf);
			lpp->pdu_type = TLLC_PDUT_DEC_AL_FINAL;
		} else {
			/* DATA Table 21.19 */
			cur++;
			lpp->ns = bits_to_uint(cur, 3); cur += 3;
			lpp->ss = bits_to_uint(cur, 8); cur += 8;
			lpp->tl_sdu = cur;
			lpp->tl_sdu_len = len - (cur - buf);
			lpp->pdu_type = TLLC_PDUT_DEC_AL_DATA;
		}
		break;
	case TLLC_PDUT_AL_UDATA_UFINAL:
		if (*cur++) {
			/* UFINAL 21.2.3.7 / Table 21.26 */
			lpp->ns = bits_to_uint(cur, 8); cur+= 8;
			lpp->ss = bits_to_uint(cur, 8); cur+= 8;
			lpp->tl_sdu = cur;
			/* FIXME: FCS */
			len -= 32;
			lpp->tl_sdu_len = len - (cur - buf);
			lpp->pdu_type = TLLC_PDUT_DEC_AL_UFINAL;
		} else {
			/* UDATA 21.2.3.6 / Table 21.24 */
			lpp->ns = bits_to_uint(cur, 8); cur+= 8;
			lpp->ss = bits_to_uint(cur, 8); cur+= 8;
			lpp->tl_sdu = cur;
			lpp->tl_sdu_len = len - (cur - buf);
			lpp->pdu_type = TLLC_PDUT_DEC_AL_UDATA;
		}
		break;
	}
	return (cur - buf);
}
