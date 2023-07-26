/* Cryptography related helper, key management and wrapper functions */
/*
 * Copyright (C) 2023 Midnight Blue B.V.
 *
 * Author: Wouter Bokslag <w.bokslag [ ] midnightblue [ ] nl>
 *
 * SPDX-License-Identifier: AGPL-3.0+
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * See the COPYING file in the main directory for details.
 */


#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <osmocom/core/utils.h>
#include <tetra_mac_pdu.h>
#include <phy/tetra_burst.h>

#include "tetra_crypto.h"

struct tetra_crypto_database _tcdb, *tcdb = &_tcdb;

static const struct value_string tetra_key_types[] = {
	{ KEYTYPE_UNDEFINED,		"UNDEFINED" },
	{ KEYTYPE_DCK,			"DCK" },
	{ KEYTYPE_MGCK,			"MGCK" },
	{ KEYTYPE_CCK_SCK,		"CCK/SCK" },
	{ KEYTYPE_GCK,			"GCK" },
	{ 0, NULL }
};

const char *tetra_get_key_type_name(enum tetra_key_type key_type)
{
	return get_value_string(tetra_key_types, key_type);
}

static const struct value_string tetra_tea_types[] = {
	{ UNKNOWN,			"UNKNOWN" },
	{ KSG_TEA1,			"TEA1" },
	{ KSG_TEA2,			"TEA2" },
	{ KSG_TEA3,			"TEA3" },
	{ KSG_TEA4,			"TEA4" },
	{ KSG_TEA5,			"TEA5" },
	{ KSG_TEA6,			"TEA6" },
	{ KSG_TEA7,			"TEA7" },
	{ KSG_PROPRIETARY,		"PROPRIETARY" },
	{ 0, NULL }
};

const char *tetra_get_ksg_type_name(enum tetra_ksg_type ksg_type)
{
	if (ksg_type >= KSG_PROPRIETARY)
		return tetra_tea_types[KSG_PROPRIETARY].str;
	else
		return get_value_string(tetra_tea_types, ksg_type);
}

static const struct value_string tetra_security_classes[] = {
	{ NETWORK_CLASS_UNDEFINED,	"CLASS_UNDEFINED" },
	{ NETWORK_CLASS_1,		"CLASS_1" },
	{ NETWORK_CLASS_2,		"CLASS_2" },
	{ NETWORK_CLASS_3,		"CLASS_3" },
	{ 0, NULL }
};

const char *tetra_get_security_class_name(uint8_t pdut)
{
	return get_value_string(tetra_security_classes, pdut);
}

void tetra_crypto_state_init(struct tetra_crypto_state *tcs)
{
	/* Initialize tetra_crypto_state */
	tcs = talloc_zero(NULL, struct tetra_crypto_state);
	tcs->mnc = -1;
	tcs->mcc = -1;
	tcs->cck_id = -1;
	tcs->hn =  -1;
	tcs->la =  -1;
	tcs->cc =  -1;
	tcs->cck = 0;
	tcs->network = 0;
}

void tetra_crypto_db_init(void)
{
	/* Initialize tetra_crypto_database */
	tcdb->num_keys = 0;
	tcdb->num_nets = 0;
	tcdb->keys = talloc_zero_array(NULL, struct tetra_key, TCDB_ALLOC_BLOCK_SIZE);
	tcdb->nets = talloc_zero_array(NULL, struct tetra_netinfo, TCDB_ALLOC_BLOCK_SIZE);

	if (!tcdb->keys || !tcdb->nets) {
		fprintf(stderr, "couldn't allocate memory for tetra_crypto_database\n");
		exit(1);
	}
}

char *dump_key(struct tetra_key *k)
{
	static char pbuf[1024];

	int c = snprintf(pbuf, sizeof(pbuf), "MCC %4d MNC %4d key_type %02X",
		k->mcc, k->mnc, k->key_type);

	if (k->key_type & (KEYTYPE_DCK | KEYTYPE_MGCK))
		c += snprintf(pbuf + c, sizeof(pbuf) - c, " addr: %8d", k->addr);

	if (k->key_type & (KEYTYPE_CCK_SCK))
		c += snprintf(pbuf + c, sizeof(pbuf) - c, " key_num: %4d", k->key_num);

	c += snprintf(pbuf + c, sizeof(pbuf) - c, ": ");
	for (int i = 0; i < 10; i++)
		snprintf(pbuf + c + 2*i, sizeof(pbuf)-c-2*i, "%02X", k->key[i]);

	return pbuf;
}

char *dump_network_info(struct tetra_netinfo *network)
{
	static char pbuf[1024];
	snprintf(pbuf, sizeof(pbuf), "MCC %4d MNC %4d ksg_type %d security_class %d", network->mcc, network->mnc, network->ksg_type, network->security_class);
	return pbuf;
}

uint32_t tea_build_iv(struct tetra_tdma_time *tm, uint16_t hn, uint8_t dir)
{
	assert(1 <= tm->tn  && tm->tn  <= 4);
	assert(1 <= tm->fn  && tm->fn  <= 18);
	assert(1 <= tm->mn  && tm->mn  <= 60);
	assert(0 <= tm->hn  && tm->hn  <= 0xFFFF);
	assert(0 <= dir && dir <= 1); // 0 = downlink, 1 = uplink
	return ((tm->tn - 1) | (tm->fn << 2) | (tm->mn << 7) | ((hn & 0x7FFF) << 13) | (dir << 28));
}

int decrypt_identity(struct tetra_addr *addr)
{
	return 0;
}

int decrypt_mac_element(struct tetra_tmvsap_prim *tmvp, struct tetra_key *key, int l1_len, int tmpdu_offset)
{
	return 0;
}

int decrypt_voice_timeslot(struct tetra_tdma_time *tdma_time, int16_t *type1_block)
{
	return 0;
}

int load_keystore(char *tetra_keyfile)
{
	return 0;
}

struct tetra_key *get_key_by_addr(struct tetra_crypto_state *tcs, int addr, enum tetra_key_type key_type)
{
	for (int i = 0; i < tcdb->num_keys; i++) {
		struct tetra_key *key = &tcdb->keys[i];
		if (key->mnc == tcs->mnc &&
				key->mcc == tcs->mcc &&
				key->addr == addr &&
				(key->key_type & key_type)) {
			return key;
		}
	}
	return 0;
}

struct tetra_key *get_ksg_key(struct tetra_crypto_state *tcs, int addr)
{
	/* TETRA standard part 7 Clause 6.2:
	--------------------------------------------
			Auth	Encr	GCK	DCK
	Class 1:	?	-	-	-
	Class 2:	?	+	?	-
	Class 3:	+	+	?	+
	--------------------------------------------
	*/

	if (!tcs->network)
		/* No tetra_netinfo from the db set for this network */
		return 0;

	/* FIXME: add support for ISSI/GSSI range definitions and GCK bindings */
	/* FIXME: add support for ISSI-bound DCK keys in class 3 networks */

	return tcs->cck;
}

void update_current_network(struct tetra_crypto_state *tcs, int mcc, int mnc)
{
	// Update globals
	tcs->mcc = mcc;
	tcs->mnc = mnc;

	// Network changed, update reference to current network
	tcs->network = 0;
	for (int i = 0; i < tcdb->num_nets; i++) {
		struct tetra_netinfo *network = &tcdb->nets[i];
		if (network->mnc == tcs->mnc && network->mcc == tcs->mcc) {
			tcs->network = network;
			break;
		}
	}

	// (Try to) select new CCK/SCK
	update_current_cck(tcs);
}

void update_current_cck(struct tetra_crypto_state *tcs)
{
	printf("\ntetra_crypto: update_current_cck invoked cck %d mcc %d mnc %d\n", tcs->cck_id, tcs->mcc, tcs->mnc);
	tcs->cck = 0;

	for (int i = 0; i < tcdb->num_keys; i++) {
		struct tetra_key *key = &tcdb->keys[i];
		/* TODO FIXME consider selecting CCK or SCK key type based on network config */
		if (key->mcc == tcs->mcc && key->mnc == tcs->mnc && key->key_num == tcs->cck_id) {
			if (key->key_type == KEYTYPE_CCK_SCK) {
				tcs->cck = key;
				printf("tetra_crypto: Set new current_cck %d (type: full)\n", i);
				break;
			}
		}
	}
}

struct tetra_netinfo *get_network_info(int mcc, int mnc)
{
	for (int i = 0; i < tcdb->num_nets; i++) {
		if (tcdb->nets[i].mcc == mcc && tcdb->nets[i].mnc == mnc)
			return &tcdb->nets[i];
	}
	return 0;
}
