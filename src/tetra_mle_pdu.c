/* Implementation of TETRA MLE PDU parsing */

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

#include <unistd.h>
#include <osmocom/core/utils.h>

#include "tetra_mle_pdu.h"

static const struct value_string mle_pdisc_names[] = {
	{ TMLE_PDISC_MM,	"MM" },
	{ TMLE_PDISC_CMCE,	"CMCE" },
	{ TMLE_PDISC_SNDCP,	"SNDCP" },
	{ TMLE_PDUSC_MLE,	"MLE" },
	{ TMLE_PDISC_MGMT,	"MGMT" },
	{ TMLE_PDISC_TEST,	"TEST" },
	{ 0, NULL }
};
const char *tetra_get_mle_pdisc_name(uint8_t pdisc)
{
	return get_value_string(mle_pdisc_names, pdisc);
}
