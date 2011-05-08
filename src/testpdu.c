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


#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <osmocom/core/bitvec.h>
#include <osmocom/core/utils.h>

#include "testpdu.h"

uint8_t pdu_sync[8];		/* 60 bits */
uint8_t pdu_sysinfo[16];	/* 124 bits */
uint8_t pdu_acc_ass[2];
uint8_t pdu_schf[268];

void testpdu_init()
{
	struct bitvec bv;

	memset(&bv, 0, sizeof(bv));
	bv.data = pdu_sync;
	bv.data_len = sizeof(pdu_sync);

	//bitvec_set_uint(&bv, 0, 4);	/* alignment */
	/* According to Table 21.73: SYNC PDU Contents */
	bitvec_set_uint(&bv, 0, 4);	/* System Code: ETS 300 392-2 ed. 1 */
	bitvec_set_uint(&bv, 0, 6);	/* Colour Code: Predefined Scrambling */
	bitvec_set_uint(&bv, 0, 2);	/* Timeslot number: TN 1 */
	bitvec_set_uint(&bv, 1, 5);	/* Frame number: FN 1 */
	bitvec_set_uint(&bv, 1, 6);	/* Multiframe number: MN 1 */
	bitvec_set_uint(&bv, 0, 2);	/* Sharing mode: continuous transmission */
	bitvec_set_uint(&bv, 0, 3);	/* TS reserved frames: 1 frame per 2 mfrm */
	bitvec_set_bit(&bv, 0);		/* No DTX */
	bitvec_set_bit(&bv, 0);		/* No Frame 18 extension */
	bitvec_set_bit(&bv, 0);		/* Reserved */
	/* As defined in Table 18.4.2.1: D-MLE-SYNC */
	bitvec_set_uint(&bv, 262, 10);	/* MCC */
	bitvec_set_uint(&bv, 42, 14);	/* MNC */
	bitvec_set_uint(&bv, 0, 2);	/* Neighbor cell boradcast: not supported */
	bitvec_set_uint(&bv, 0, 2);	/* Cell service level: unknown */	
	bitvec_set_bit(&bv, 0);		/* Late entry information */
	printf("SYNC PDU: %s\n", osmo_hexdump(pdu_sync, sizeof(pdu_sync)));

	memset(&bv, 0, sizeof(bv));
	bv.data = pdu_sysinfo;
	bv.data_len = sizeof(pdu_sysinfo);

	//bitvec_set_uint(&bv, 0, 4);	/* alignment */
	/* According to Table 21.4.4.1: SYSINFO PDU contents */
	bitvec_set_uint(&bv, 2, 2);	/* MAC PDU type: Broadcast */
	bitvec_set_uint(&bv, 0, 2);	/* SYSINVO PDU */
	bitvec_set_uint(&bv, ((392775-300000)/25), 12);	/* Main carier */
	bitvec_set_uint(&bv, 3, 4);	/* Frequency band: 390/400 */
	bitvec_set_uint(&bv, 0, 2);	/* Offset: No offset */
	bitvec_set_uint(&bv, 0, 3);	/* Duplex Spacing: */
	bitvec_set_bit(&bv, 0);		/* Normal operation */
	bitvec_set_uint(&bv, 0, 2);	/* Number of CSCH: none */
	bitvec_set_uint(&bv, 1, 3);	/* MS_TXPWR_MAX_CELL: 15 dBm */
	bitvec_set_uint(&bv, 0, 4);	/* RXLEV_ACCESS_MIN: -125dBm */
	bitvec_set_uint(&bv, 0, 4);	/* ACCESS_PARAMETER: -53 dBm */
	bitvec_set_uint(&bv, 0, 4);	/* RADIO_DOWNLINK_TIMEOUT: Disable */
	bitvec_set_bit(&bv, 0);		/* Hyperframe number follows */
	bitvec_set_uint(&bv, 0, 16);	/* Hyperframe number */
	bitvec_set_uint(&bv, 0, 2);	/* Optional field: Even multiframe */
	bitvec_set_uint(&bv, 0, 20);	/* TS_COMMON_FRAMES for even mframe */
	/* TM-SDU (42 bit), Section 18.4.2.2, Table 18.15 */
	bitvec_set_uint(&bv, 0, 14);	/* Location Area (18.5.9) */
	bitvec_set_uint(&bv, 0xFFFF, 16);	/* Subscriber Class (18.5.22) */
	bitvec_set_uint(&bv, 0, 12);	/* BS service details (18.5.2) */
	printf("SYSINFO PDU: %s\n", osmo_hexdump(pdu_sysinfo, sizeof(pdu_sysinfo)));

	memset(&bv, 0, sizeof(bv));
	bv.data = pdu_acc_ass;
	bv.data_len = sizeof(pdu_acc_ass);

	bitvec_set_uint(&bv, 0, 2);	/* alignment */
	/* According to Table 21.27: ACCESS-ASSIGN PDU */
	bitvec_set_uint(&bv, 0, 2);	/* DL/UL: common only */
	bitvec_set_uint(&bv, 0, 6);
	bitvec_set_uint(&bv, 0, 6);
	printf("ACCESS-ASSIGN PDU: %s\n", osmo_hexdump(pdu_acc_ass, sizeof(pdu_acc_ass)));
}
