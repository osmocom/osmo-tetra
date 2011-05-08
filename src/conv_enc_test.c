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
#include <time.h>

#include <arpa/inet.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/bits.h>


#include "tetra_common.h"
#include <lower_mac/crc_simple.h>
#include <lower_mac/tetra_conv_enc.h>
#include <lower_mac/tetra_interleave.h>
#include <lower_mac/tetra_scramb.h>
#include <lower_mac/tetra_rm3014.h>
#include <lower_mac/viterbi.h>
#include <phy/tetra_burst.h>
#include "testpdu.h"


#define swap16(x) ((x)<<8)|((x)>>8)

static unsigned int num_crc_err;

/* incoming TP-SAP UNITDATA.ind  from PHY into lower MAC */
void tp_sap_udata_ind(enum tp_sap_data_type type, const uint8_t *bits, unsigned int len, void *priv)
{
}

static void decode_schf(const uint8_t *bits)
{
	uint8_t type4[1024];
	uint8_t type3dp[1024*4];
	uint8_t type3[1024];
	uint8_t type2[1024];

	printf("SCH/f type5: %s\n", osmo_ubit_dump(bits, 432));
	memcpy(type4, bits, 432);
	tetra_scramb_bits(SCRAMB_INIT, type4, 432);
	printf("SCH/F type4: %s\n", osmo_ubit_dump(type4, 432));
	/* Run (120,11) block deinterleaving: type-3 bits */
	block_deinterleave(432, 103, type4, type3);
	printf("SCH/F type3: %s\n", osmo_ubit_dump(type3, 432));
	/* De-puncture */
	memset(type3dp, 0xff, sizeof(type3dp));
	tetra_rcpc_depunct(TETRA_RCPC_PUNCT_2_3, type3, 432, type3dp);
	printf("SCH/F type3dp: %s\n", osmo_ubit_dump(type3dp, 288*4));
	viterbi_dec_sb1_wrapper(type3dp, type2, 288);
	printf("SCH/F type2: %s\n", osmo_ubit_dump(type2, 288));

	{
		uint16_t crc;
		crc = crc16_ccitt_bits(type2, 288-4);
		printf("CRC COMP: 0x%04x ", crc);
		if (crc == 0x1d0f)
			printf("OK\n");
		else {
			printf("WRONG\n");
			num_crc_err++;
		}
	}
	printf("SCH/F type1: %s\n", osmo_ubit_dump(type2, 268));
}

/* Build a full 'downlink continuous SYNC burst' from SYSINFO-PDU and SYNC-PDU */
int build_ndb_schf()
{
	/* input: 268 type-1 bits */
	uint8_t type2[284];
	uint8_t master[284*4];
	uint8_t type3[432];
	uint8_t type4[432];
	uint8_t type5[432];
	uint8_t bb_type5[30];
	uint8_t burst[255*2];
	uint16_t crc;
	uint8_t *cur;
	uint32_t bb_rm3014, bb_rm3014_be;

	memset(type2, 0, sizeof(type2));
	cur = type2;

	/* Use pdu_sync from testpdu.c */
	cur += osmo_pbit2ubit(type2, pdu_schf, 268);

	crc = ~crc16_ccitt_bits(type2, 268);
	crc = swap16(crc);
	cur += osmo_pbit2ubit(cur, (uint8_t *) &crc, 16);

	/* Append 4 tail bits: type-2 bits */
	cur += 4;

	printf("SCH/F type2: %s\n", osmo_ubit_dump(type2, 288));

	/* Run rate 2/3 RCPC code: type-3 bits*/
	{
		struct conv_enc_state *ces = calloc(1, sizeof(*ces));
		conv_enc_init(ces);
		conv_enc_input(ces, type2, 288, master);
		get_punctured_rate(TETRA_RCPC_PUNCT_2_3, master, 432, type3);
		free(ces);
	}
	printf("SCH/F type3: %s\n", osmo_ubit_dump(type3, 432));

	/* Run (432,103) block interleaving: type-4 bits */
	block_interleave(432, 103, type3, type4);
	printf("SCH/F type4: %s\n", osmo_ubit_dump(type4, 432));

	/* Run scrambling (all-zero): type-5 bits */
	memcpy(type5, type4, 432);
	tetra_scramb_bits(SCRAMB_INIT, type5, 432);
	printf("SCH/F type5: %s\n", osmo_ubit_dump(type5, 432));

	decode_schf(type5);

	/* Use pdu_acc_ass from testpdu.c */
	/* Run it through (30,14) RM code: type-2=3=4 bits */
	printf("AACH type-1: %s\n", osmo_ubit_dump(pdu_acc_ass, 2));
	bb_rm3014 = tetra_rm3014_compute(*(uint16_t *)pdu_acc_ass);
	printf("AACH RM3014: 0x0%x\n", bb_rm3014);
		/* convert to big endian */
		bb_rm3014_be = htonl(bb_rm3014);
		/* shift two bits left as it is only a 30 bit value */
		bb_rm3014_be <<= 2;
	osmo_pbit2ubit(bb_type5, (uint8_t *) &bb_rm3014_be, 30);
	/* Run scrambling (all-zero): type-5 bits */
	printf("AACH type-5: %s\n", osmo_ubit_dump(bb_type5, 30));

	/* Finally, hand it into the physical layer */
	build_norm_c_d_burst(burst, type5, bb_type5, type5+216, 0);
	printf("cont norm DL burst: %s\n", osmo_ubit_dump(burst, 255*2));

	return 0;
}


static void decode_sb1(const uint8_t *bits)
{
	uint8_t type4[1024];
	uint8_t type3dp[1024*4];
	uint8_t type3[1024];
	uint8_t type2[1024];

	printf("SB1 type5: %s\n", osmo_ubit_dump(bits, 120));
	memcpy(type4, bits, 120);
	tetra_scramb_bits(SCRAMB_INIT, type4, 120);
	printf("SB1 type4: %s\n", osmo_ubit_dump(type4, 120));
	/* Run (120,11) block deinterleaving: type-3 bits */
	block_deinterleave(120, 11, type4, type3);
	printf("SB1 type3: %s\n", osmo_ubit_dump(type3, 120));
	/* De-puncture */
	memset(type3dp, 0xff, sizeof(type3dp));
	tetra_rcpc_depunct(TETRA_RCPC_PUNCT_2_3, type3, 120, type3dp);
	printf("SB1 type3dp: %s\n", osmo_ubit_dump(type3dp, 80*4));
	viterbi_dec_sb1_wrapper(type3dp, type2, 80);
	printf("SB1 type2: %s\n", osmo_ubit_dump(type2, 80));

	{
		uint16_t crc;
		crc = crc16_ccitt_bits(type2, 76);
		printf("CRC COMP: 0x%04x ", crc);
		if (crc == 0x1d0f)
			printf("OK\n");
		else {
			printf("WRONG\n");
			num_crc_err++;
		}
	}

	printf("TN %s ", osmo_ubit_dump(type2+10, 2));
	printf("MCC %s ", osmo_ubit_dump(type2+31, 10));
	printf("MNC %s\n", osmo_ubit_dump(type2+41, 14));
}

/* Build a full 'downlink continuous SYNC burst' from SYSINFO-PDU and SYNC-PDU */
int build_sb()
{
	uint8_t sb_type2[80];
	uint8_t sb_master[80*4];
	uint8_t sb_type3[120];
	uint8_t sb_type4[120];
	uint8_t sb_type5[120];

	uint8_t si_type2[140];
	uint8_t si_master[140*4];
	uint8_t si_type3[216];
	uint8_t si_type4[216];
	uint8_t si_type5[216];

	uint8_t bb_type5[30];
	uint8_t burst[255*2];
	uint16_t crc;
	uint8_t *cur;
	uint32_t bb_rm3014, bb_rm3014_be;

	memset(sb_type2, 0, sizeof(sb_type2));
	cur = sb_type2;

	/* Use pdu_sync from testpdu.c */
	cur += osmo_pbit2ubit(sb_type2, pdu_sync, 60);

	crc = ~crc16_ccitt_bits(sb_type2, 60);
	crc = swap16(crc);
	cur += osmo_pbit2ubit(cur, (uint8_t *) &crc, 16);

	/* Append 4 tail bits: type-2 bits */
	cur += 4;

	printf("SYNC type2: %s\n", osmo_ubit_dump(sb_type2, 80));

	/* Run rate 2/3 RCPC code: type-3 bits*/
	{
		struct conv_enc_state *ces = calloc(1, sizeof(*ces));
		conv_enc_init(ces);
		conv_enc_input(ces, sb_type2, 80, sb_master);
		get_punctured_rate(TETRA_RCPC_PUNCT_2_3, sb_master, 120, sb_type3);
		free(ces);
	}
	printf("SYNC type3: %s\n", osmo_ubit_dump(sb_type3, 120));

	/* Run (120,11) block interleaving: type-4 bits */
	block_interleave(120, 11, sb_type3, sb_type4);
	printf("SYNC type4: %s\n", osmo_ubit_dump(sb_type4, 120));

	/* Run scrambling (all-zero): type-5 bits */
	memcpy(sb_type5, sb_type4, 120);
	tetra_scramb_bits(SCRAMB_INIT, sb_type5, 120);
	printf("SYNC type5: %s\n", osmo_ubit_dump(sb_type5, 120));

	decode_sb1(sb_type5);

	/* Use pdu_sysinfo from testpdu.c */
	memset(si_type2, 0, sizeof(si_type2));
	cur = si_type2;
	cur += osmo_pbit2ubit(si_type2, pdu_sysinfo, 124);

	/* Run it through CRC16-CCITT */
	crc = ~crc16_ccitt_bits(si_type2, 124);
	crc = swap16(crc);
	cur += osmo_pbit2ubit(cur, (uint8_t *) &crc, 16);

	/* Append 4 tail bits: type-2 bits */
	cur += 4;

	printf("SI type2: %s\n", osmo_ubit_dump(si_type2, 140));

	/* Run rate 2/3 RCPC code: type-3 bits */
	{
		struct conv_enc_state *ces = calloc(1, sizeof(*ces));
		conv_enc_init(ces);
		conv_enc_input(ces, sb_type2, 144, si_master);
		get_punctured_rate(TETRA_RCPC_PUNCT_2_3, si_master, 216, si_type3);
		free(ces);
	}
	printf("SI type3: %s\n", osmo_ubit_dump(si_type3, 216));

	/* Run (216,101) block interleaving: type-4 bits */
	block_interleave(216, 101, si_type3, si_type4);
	printf("SI type4: %s\n", osmo_ubit_dump(si_type4, 216));

	/* Run scrambling (all-zero): type-5 bits */
	memcpy(si_type5, si_type4, 216);


	/* Use pdu_acc_ass from testpdu.c */
	/* Run it through (30,14) RM code: type-2=3=4 bits */
	printf("AACH type-1: %s\n", osmo_ubit_dump(pdu_acc_ass, 2));
	bb_rm3014 = tetra_rm3014_compute(*(uint16_t *)pdu_acc_ass);
	printf("AACH RM3014: 0x0%x\n", bb_rm3014);
		/* convert to big endian */
		bb_rm3014_be = htonl(bb_rm3014);
		/* shift two bits left as it is only a 30 bit value */
		bb_rm3014_be <<= 2;
	osmo_pbit2ubit(bb_type5, (uint8_t *) &bb_rm3014_be, 30);
	/* Run scrambling (all-zero): type-5 bits */
	printf("AACH type-5: %s\n", osmo_ubit_dump(bb_type5, 30));

	/* Finally, hand it into the physical layer */
	build_sync_c_d_burst(burst, sb_type5, bb_type5, si_type5);
	printf("cont sync DL burst: %s\n", osmo_ubit_dump(burst, 255*2));

	return 0;
}

int main(int argc, char **argv)
{
	int err, i;
	uint16_t out;
	uint32_t ret;

	/* first: run some subsystem tests */
	ret = tetra_punct_test();
	if (ret < 0)
		exit(1);

	tetra_rm3014_init();
#if 0
	ret = tetra_rm3014_compute(0x1001);
	printf("RM3014: 0x%08x\n", ret);

	err = tetra_rm3014_decode(ret, &out);
	printf("RM3014: 0x%x error: %d\n", out, err);
#endif

	/* finally, build some test PDUs and encocde them */
	testpdu_init();
#if 0
	build_sb();

	build_ndb_schf();
#else
	/* iterate over various random PDUs and throw them throguh the viterbi */
	srand(time(NULL));
	for (i = 0; i < 100; i++) {
		uint32_t r = rand();
		osmo_pbit2ubit(pdu_sync, (uint8_t *) &r, 32);
		osmo_pbit2ubit(pdu_sync+32, (uint8_t *)&r, 60-32);
		//build_sb();

		osmo_pbit2ubit(pdu_schf, (uint8_t *) &r, 32);
		osmo_pbit2ubit(pdu_schf+32, (uint8_t *)&r, 60-32);
		build_ndb_schf();
	}
#endif

	printf("total number of CRC Errors: %u\n", num_crc_err);

	exit(0);
}
