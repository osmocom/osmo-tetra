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
	{ KEYTYPE_CCK_SCK,		"CCK/SCK" },
	{ KEYTYPE_DCK,			"DCK" },
	{ KEYTYPE_MGCK,			"MGCK" },
	{ KEYTYPE_GCK,			"GCK" },
	{ 0, NULL }
};

const char *tetra_get_key_type_name(enum tetra_key_type key_type)
{
	return get_value_string(tetra_key_types, key_type);
}

static const struct value_string tetra_ksg_types[] = {
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
		return tetra_ksg_types[KSG_PROPRIETARY].str;
	else
		return get_value_string(tetra_ksg_types, ksg_type);
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
	/* Initialize network info fields to -1 to designate unknown */
	tcs->mnc = -1;
	tcs->mcc = -1;
	tcs->cck_id = -1;
	tcs->hn =  -1;
	tcs->la =  -1;
	tcs->cc =  -1;

	/* Initialize database key/network pointers to zero */
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

	int c = snprintf(pbuf, sizeof(pbuf), "MCC %4d MNC %4d key_type %s",
		k->mcc, k->mnc, tetra_get_key_type_name(k->key_type));

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

int decrypt_identity(struct tetra_crypto_state *tcs, struct tetra_addr *addr)
{
	return 0;
}

int decrypt_mac_element(struct tetra_crypto_state *tcs, struct tetra_tmvsap_prim *tmvp, struct tetra_key *key, int l1_len, int tmpdu_offset)
{
	return 0;
}

int decrypt_voice_timeslot(struct tetra_crypto_state *tcs, struct tetra_tdma_time *tdma_time, int16_t *type1_block)
{
	return 0;
}

int load_keystore(char *tetra_keyfile)
{
	/* Keystore file:
	 * Each line contains network or key definition.
	 * Lines starting with # are ignored as comments.
	 *
	 *   network mcc 123 mnc 456 ksg_type 1 security_class 2
	 *   - ksg_type: decimal, see enum tetra_ksg_type
	 *   - security_class: 2 for SCK, 3 for CCK (and DCK per ISSI)
	 *
	 *   key mcc 123 mnc 456 addr 00000000 key_type 1 key_num 002 key 1234deadbeefcafebabe
	 *   - addr: decimal, leave zero for key type 1 (CCK/SCK)
	 *   - key_type: 1 CCK/SCK, 2 DCK, 3 MGCK, 4 GCK
	 *   - key_num: SCK_VN or group key number depending on type
	 *   - key: 80-bit key hex string
	 */

	int i, c;
	char buf[1000]; // max line len
	FILE *fp;

	tetra_crypto_db_init();

	fp = fopen(tetra_keyfile, "r");
	if (!fp) {
		printf("tetra_crypto: cannot read keyfile\n");
		exit(1);
	}

	while (fgets(buf, sizeof(buf), fp)) {

		if (strlen(buf) <= 1 || buf[0] == '#') {
			// Commented/empty line
			continue;

		} else if (!strncmp(buf, "network ", 8)) {

			/* Network definition */
			i = tcdb->num_nets;
			if (i > 0 && (i % TCDB_ALLOC_BLOCK_SIZE == 0))
				tcdb->nets = talloc_realloc(NULL, tcdb->nets, struct tetra_netinfo, i + TCDB_ALLOC_BLOCK_SIZE);

			c = sscanf(buf, "network mcc %d mnc %d ksg_type %d security_class %d\n",
				&tcdb->nets[i].mcc, &tcdb->nets[i].mnc,
				(uint32_t *) &tcdb->nets[i].ksg_type,
				(uint32_t *) &tcdb->nets[i].security_class);

			if (c != 4) {
				printf("tetra_crypto: Failed to parse network info element %d [%s] (%d)\n", i, buf, c);
				exit(1);
			}
			printf("tetra_crypto: Loaded MNC [%s]\n", dump_network_info(&tcdb->nets[i]));
			tcdb->num_nets++;

		} else if (!strncmp(buf, "key ", 4)) {

			/* Key definition */
			i = tcdb->num_keys;
			if (i > 0 && (i % TCDB_ALLOC_BLOCK_SIZE == 0))
				tcdb->keys = talloc_realloc(NULL, tcdb->keys, struct tetra_key, i + TCDB_ALLOC_BLOCK_SIZE);

			c = sscanf(buf, "key mcc %d mnc %d addr %d key_type %d key_num %d key %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
				&tcdb->keys[i].mcc, &tcdb->keys[i].mnc, &tcdb->keys[i].addr,
				(uint32_t *) &tcdb->keys[i].key_type, &tcdb->keys[i].key_num,
				(uint32_t *) &tcdb->keys[i].key[0], (uint32_t *) &tcdb->keys[i].key[1],
				(uint32_t *) &tcdb->keys[i].key[2], (uint32_t *) &tcdb->keys[i].key[3],
				(uint32_t *) &tcdb->keys[i].key[4], (uint32_t *) &tcdb->keys[i].key[5],
				(uint32_t *) &tcdb->keys[i].key[6], (uint32_t *) &tcdb->keys[i].key[7],
				(uint32_t *) &tcdb->keys[i].key[8], (uint32_t *) &tcdb->keys[i].key[9]);
			tcdb->keys[i].index = i;
			if (c != 15) {
				printf("tetra_crypto: Failed to parse key %d [%s] (%d)\n", i, buf, c);
				exit(1);
			}
			printf("tetra_crypto: Loaded key [%s]\n", dump_key(&tcdb->keys[i]));
			tcdb->num_keys++;

		} else {
			printf("tetra_crypto: Could not parse line: %s\n", buf);
			exit(1);
		}
	}

	/* Check network info available for each key and set ptrs for convenience */
	for (i = 0; i < tcdb->num_keys; i++) {
		struct tetra_netinfo *network_info_ptr = get_network_info(tcdb->keys[i].mcc, tcdb->keys[i].mnc);
		if (!network_info_ptr) {
			printf("tetra_crypto: Required network info is missing for %4d", tcdb->keys[i].mnc);
			exit(1);
		}
		tcdb->keys[i].network_info = network_info_ptr;
	}

	fclose(fp);
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
