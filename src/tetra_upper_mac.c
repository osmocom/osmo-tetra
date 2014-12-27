/* TETRA upper MAC layer main routine, above TMV-SAP */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/msgb.h>
#include <osmocom/core/talloc.h>

#include "tetra_common.h"
#include "tetra_prim.h"
#include "tetra_upper_mac.h"
#include "tetra_mac_pdu.h"
#include "tetra_llc_pdu.h"
#include "tetra_mm_pdu.h"
#include "tetra_cmce_pdu.h"
#include "tetra_sndcp_pdu.h"
#include "tetra_mle_pdu.h"
#include "tetra_gsmtap.h"
#include "tetra_sds.h"

static int rx_tm_sdu(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len);

static void rx_bcast(struct tetra_tmvsap_prim *tmvp, struct tetra_mac_state *tms)
{
	struct msgb *msg = tmvp->oph.msg;
	struct tetra_si_decoded sid;
	uint32_t dl_freq, ul_freq;
	int i;

	memset(&sid, 0, sizeof(sid));
	macpdu_decode_sysinfo(&sid, msg->l1h);
	tmvp->u.unitdata.tdma_time.hn = sid.hyperframe_number;

	dl_freq = tetra_dl_carrier_hz(sid.freq_band,
				      sid.main_carrier,
				      sid.freq_offset);

	ul_freq = tetra_ul_carrier_hz(sid.freq_band,
				      sid.main_carrier,
				      sid.freq_offset,
				      sid.duplex_spacing,
				      sid.reverse_operation);

	printf("BNCH SYSINFO (DL %u Hz, UL %u Hz), service_details 0x%04x LA:%u ",
		dl_freq, ul_freq, sid.mle_si.bs_service_details,sid.mle_si.la);
	/* sq5bpf */
	tetra_hack_dl_freq=dl_freq;
	tetra_hack_ul_freq=ul_freq;
	tetra_hack_la=sid.mle_si.la;
	
	if (sid.cck_valid_no_hf)
		printf("CCK ID %u", sid.cck_id);
	else
		printf("Hyperframe %u", sid.hyperframe_number);
	printf("\n");
	for (i = 0; i < 12; i++)
		printf("\t%s: %u\n", tetra_get_bs_serv_det_name(1 << i),
			sid.mle_si.bs_service_details & (1 << i) ? 1 : 0);

	memcpy(&tms->last_sid, &sid, sizeof(sid));
}

const char *tetra_alloc_dump(const struct tetra_chan_alloc_decoded *cad, struct tetra_mac_state *tms)
{
	static char buf[64];
	char *cur = buf;
	unsigned int freq_band, freq_offset;

	if (cad->ext_carr_pres) {
		freq_band = cad->ext_carr.freq_band;
		freq_offset = cad->ext_carr.freq_offset;
	} else {
		freq_band = tms->last_sid.freq_band;
		freq_offset = tms->last_sid.freq_offset;
	}

	cur += sprintf(cur, "%s (TN%u/%s/%uHz)",
		tetra_get_alloc_t_name(cad->type), cad->timeslot,
		tetra_get_ul_dl_name(cad->ul_dl),
		tetra_dl_carrier_hz(freq_band, cad->carrier_nr, freq_offset));

	return buf;
}

/* sq5bpf */
int parse_d_release(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len)
{
	uint8_t *bits = msg->l3h+3;
	int n=0;
	int m=0;

	/* strona 270 */
	m=5; uint8_t pdu_type=bits_to_uint(bits+n, m); n=n+m;
	m=14; uint16_t callident=bits_to_uint(bits+n, m); n=n+m;
	m=4; uint16_t disccause=bits_to_uint(bits+n, m); n=n+m;
	m=6; uint16_t notifindic=bits_to_uint(bits+n, m); n=n+m;
	printf("\nCall identifier:%i Discconnect cause:%i NotificationID:%i\n",callident,disccause,notifindic);

}


uint parse_d_setup(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len)
{
	uint8_t *bits = msg->l3h+3;
	int n=0;
	int m=0;
	uint32_t callingssi=0;
	uint32_t callingext=0;
	char tmpstr2[1024];
	struct tetra_resrc_decoded rsd;
	int tmpdu_offset;
	memset(&rsd, 0, sizeof(rsd));
	tmpdu_offset = macpdu_decode_resource(&rsd, msg->l1h);



	/* strona 270, opisy strona 280 */
	m=5; uint8_t pdu_type=bits_to_uint(bits+n, m); n=n+m;

	m=14; uint16_t callident=bits_to_uint(bits+n, m); n=n+m;
	m=4; uint16_t calltimeout=bits_to_uint(bits+n, m);  n=n+m;
	m=1; uint16_t hookmethod=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint16_t duplex=bits_to_uint(bits+n, m); n=n+m;
	m=8; uint8_t basicinfo=bits_to_uint(bits+n, m); n=n+m;
	m=2; uint16_t txgrant=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint16_t txperm=bits_to_uint(bits+n, m); n=n+m;
	m=4; uint16_t callprio=bits_to_uint(bits+n, m); n=n+m;
	m=6; uint16_t notifindic=bits_to_uint(bits+n, m); n=n+m;
	m=24; uint32_t tempaddr=bits_to_uint(bits+n, m); n=n+m;
	m=2; uint16_t cpti=bits_to_uint(bits+n, m); n=n+m;
	switch(cpti)
	{
		case 0: /* SNA */
			m=8; callingssi=bits_to_uint(bits+n, m); n=n+m;
			break;
		case 1: /* SSI */
			m=24; callingssi=bits_to_uint(bits+n, m); n=n+m;
			break;
		case 2: /* TETRA Subscriber Identity (TSI) */
			m=24; callingssi=bits_to_uint(bits+n, m); n=n+m;
			m=24; callingext=bits_to_uint(bits+n, m); n=n+m;
			break;
		case 3: /* reserved ? */
			break;
	}


	printf ("\nCall identifier:%i  Call timeout:%i  Hookmethod:%i  Duplex:%i\n",callident,calltimeout,hookmethod,duplex);
	printf("Basicinfo:0x%2.2X  Txgrant:%i  TXperm:%i  Callprio:%i\n",basicinfo,txgrant,txperm,callprio);
	printf("NotificationID:%i  Tempaddr:%i CPTI:%i  CallingSSI:%i  CallingExt:%i\n",notifindic,tempaddr,cpti,callingssi,callingext);

	sprintf(tmpstr2,"TETMON_begin FUNC:DSETUPDEC IDX:%i SSI:%i RX:%i TETMON_end",rsd.addr.usage_marker,tempaddr,tetra_hack_rxid);
	sendto(tetra_hack_live_socket, (char *)&tmpstr2, strlen((char *)&tmpstr2)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);

}

uint parse_d_sds_data(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len)
{
	/* strona  269, 297, 1072, 1075 */
	uint8_t *bits = msg->l3h+3;

	int n=0;
	int l,a;
	int m=5;

	uint8_t pdu_type;
	uint8_t cpti;
	uint32_t calling_ssi;
	uint32_t calling_ext=0;
	uint8_t sdti;
	uint8_t udata[512];
	uint16_t datalen=0;
	uint8_t protoid;
	uint8_t reserved1;
	uint8_t coding_scheme;
	char descr[1024];
	char tmpstr[128];
	char tmpstr2[1024];

	struct tetra_resrc_decoded rsd;
	int tmpdu_offset;
	memset(&rsd, 0, sizeof(rsd));
	tmpdu_offset = macpdu_decode_resource(&rsd, msg->l1h);



	m=5; pdu_type=bits_to_uint(bits+n, m); n=n+m;
	m=2; cpti=bits_to_uint(bits+n, m); n=n+m;
	switch(cpti) {
		case 1:
			m=24; calling_ssi=bits_to_uint(bits+n, m); n=n+m;
			break;
		case 2:
			m=24; calling_ssi=bits_to_uint(bits+n, m); n=n+m;
			m=24; calling_ext=bits_to_uint(bits+n, m); n=n+m;
			break;
		default:
			/* hgw co to jest */
			printf("\nparse_d_sds_data: ZLY CPTI %i\n",cpti);
			return(1);
			break;
	}

	sprintf(descr,"CPTI:%i CalledSSI:%i CallingSSI:%i CallingEXT:%i ",cpti,rsd.addr.ssi,calling_ssi,calling_ext);

	m=2; sdti=bits_to_uint(bits+n, m); n=n+m;

	switch(sdti) {
		case 0: /* user defined data-1 16 bit */
			datalen=2;
			m=8;
			for(l=0;l<datalen;l++) {
				udata[l]=bits_to_uint(bits+n, m); n=n+m;
			}
			sprintf(tmpstr," UserData1: 0x%2.2X 0x%2.2X",udata[0],udata[1]);
			strcat(descr,tmpstr);

			break;
		case 1: /* user defined data-2 32 bit */
			datalen=4;
			m=8;
			for(l=0;l<datalen;l++) {
				udata[l]=bits_to_uint(bits+n, m); n=n+m;
			}
			sprintf(tmpstr,"UserData2: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X",udata[0],udata[1],udata[2],udata[3]);
			strcat(descr,tmpstr);
			break;
		case 2: /* user defined data-3 64 bit */
			datalen=8;
			m=8;
			for(l=0;l<datalen;l++) {
				udata[l]=bits_to_uint(bits+n, m); n=n+m;

			}
			sprintf(tmpstr," UserData3: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X",udata[0],udata[1],udata[2],udata[3],udata[4],udata[5],udata[6],udata[7]);
			strcat(descr,tmpstr);
			break;
		case 3: /* length indicator + user defined data-4 bit */
			m=11; datalen=bits_to_uint(bits+n, m); n=n+m;
			m=8; protoid=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
			sprintf(tmpstr," UserData4: len:%i protoid:%2.2X(%s) ",datalen,protoid,get_sds_type(protoid));
			strcat(descr,tmpstr);

			uint8_t c;

			if ((protoid==TETRA_SDS_PROTO_TXTMSG)||(protoid==TETRA_SDS_PROTO_SIMPLE_TXTMSG)||(protoid==TETRA_SDS_PROTO_SIMPLE_ITXTMSG)||(protoid==TETRA_SDS_PROTO_ITXTMSG)) {
				m=1; reserved1=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
				m=7; coding_scheme=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
				sprintf(tmpstr," coding_scheme:%2.2x ",coding_scheme);
				strcat(descr,tmpstr);

				sprintf(tmpstr,"DATA:[");
				strcat(descr,tmpstr);

				/* dump text message */
				switch(coding_scheme) {
					case 0: /* 7-bit gsm encoding */
						sprintf(tmpstr," *7bit* ");
						strcat(descr,tmpstr);
						m=8;
						l=0;
						while(datalen>=m) {
							udata[l]=bits_to_uint(bits+n, m); n=n+m;
							l++;
							datalen=datalen-m;
						}
						/* TODO: maybe skip the first two bytes? i've never seen a 7-bit SDS in the wild --sq5bpf */
						datalen=decode_pdu(tmpstr2,udata,l);
						/* dump */
						for(a=0;a<datalen;a++) {
							if (isprint(tmpstr2[a])) {
								sprintf(tmpstr,"%c",tmpstr2[a]);
							}
							else {
								sprintf(tmpstr,"\\x%2.2X",tmpstr2[a]);
							}
							strcat(descr,tmpstr);


						}
						strcat(descr,"]\n");


						break;
					case 0x1A: /* SO/IEC 10646-1 [22] UCS-2/UTF-16BE (16-bit) alphabet */
						/* TODO: use iconv or whatever else
						 * for now we'll just use the 8-bit decoding function, 
						 * every other bit will be written as \x00. ugly but readable --sq5bpf 
						 */

						sprintf(tmpstr," *UTF16* ");
						strcat(descr,tmpstr);

					default: /* 8-bit */
						m=8;
						l=0;
						while(datalen>=m) {
							udata[l]=bits_to_uint(bits+n, m); n=n+m;
							l++;
							datalen=datalen-m;
						}
						/* TODO: the first two bytes are often garbage. either parse it or skip it 
						 * i guess i'll have to read the etsi specifications better --sq5bpf */

						for(a=0;a<l;a++) {
							if (isprint(udata[a])) {
								sprintf(tmpstr,"%c",udata[a]);
							}
							else {
								sprintf(tmpstr,"\\x%2.2X",udata[a]);
							}
							strcat(descr,tmpstr);


						}
						strcat(descr,"]\n");
						break;
				}
			} 
			else
			{
				sprintf(tmpstr,"DATA:[");
				strcat(descr,tmpstr);
				/* other message */
				/* hexdump */
				m=8;
				l=0;
				while(datalen>=m) {
					udata[l]=bits_to_uint(bits+n, m); n=n+m;
					l++;
					datalen=datalen-m;
				}
				/* dump */
				for(a=0;a<l;a++) {
					sprintf(tmpstr,"0x%2.2X ",udata[a]);
					strcat(descr,tmpstr);
				}
				strcat(descr,"]\n");
			}


	}



	printf("%s\n",descr);
	sprintf(tmpstr2,"TETMON_begin FUNC:SDSDEC [%s] RX:%i TETMON_end",descr,tetra_hack_rxid);
	sendto(tetra_hack_live_socket, (char *)&tmpstr2, strlen((char *)&tmpstr2)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);
	/*  We'll just ignore these for now :)
	 *  External subscriber number
	 *  DM-MS address
	 */

}



/* Receive TL-SDU (LLC SDU == MLE PDU) */
static int rx_tl_sdu(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len)
{
	uint8_t *bits = msg->l3h;
	uint8_t mle_pdisc = bits_to_uint(bits, 3);
	char tmpstr[1024];
	printf("TL-SDU(%s): %s", tetra_get_mle_pdisc_name(mle_pdisc),
			osmo_ubit_dump(bits, len));
	switch (mle_pdisc) {
		case TMLE_PDISC_MM:
			printf(" %s", tetra_get_mm_pdut_name(bits_to_uint(bits+3, 4), 0));
			break;
		case TMLE_PDISC_CMCE:
			printf(" %s", tetra_get_cmce_pdut_name(bits_to_uint(bits+3, 5), 0));

			/* sq5bpf */
			switch(bits_to_uint(bits+3, 5)) {
				case TCMCE_PDU_T_D_SETUP:
					parse_d_setup(tms,msg,len);
					break;

				case TCMCE_PDU_T_D_RELEASE:
					parse_d_release(tms,msg,len);
					break;

				case TCMCE_PDU_T_D_SDS_DATA:
					sprintf(tmpstr,"TETMON_begin FUNC:SDS [%s] TETMON_end",osmo_ubit_dump(bits, len));
					sendto(tetra_hack_live_socket, (char *)&tmpstr, strlen((char *)&tmpstr)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);
					parse_d_sds_data(tms,msg,len);
					break;



					/*				case TCMCE_PDU_T_U_SDS_DATA:
									sprintf(tmpstr,"TETMON_begin FUNC:D-SDS [%s] TETMON_end",osmo_ubit_dump(bits, len));
									sendto(tetra_hack_live_socket, (char *)&tmpstr, strlen((char *)&tmpstr)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);
									break;
									*/
			}

			break;
		case TMLE_PDISC_SNDCP:
			printf(" %s", tetra_get_sndcp_pdut_name(bits_to_uint(bits+3, 4), 0));
			printf(" NSAPI=%u PCOMP=%u, DCOMP=%u",
					bits_to_uint(bits+3+4, 4),
					bits_to_uint(bits+3+4+4, 4),
					bits_to_uint(bits+3+4+4+4, 4));
			printf(" V%u, IHL=%u",
					bits_to_uint(bits+3+4+4+4+4, 4),
					4*bits_to_uint(bits+3+4+4+4+4+4, 4));
			printf(" Proto=%u",
					bits_to_uint(bits+3+4+4+4+4+4+4+64, 8));
			break;
		case TMLE_PDISC_MLE:
			printf(" %s", tetra_get_mle_pdut_name(bits_to_uint(bits+3, 3), 0));
			break;
		default:
			break;
	}
	return len;
}

static int rx_tm_sdu(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len)
{
	struct tetra_llc_pdu lpp;
	uint8_t *bits = msg->l2h;

	memset(&lpp, 0, sizeof(lpp));
	tetra_llc_pdu_parse(&lpp, bits, len);

	printf("TM-SDU(%s,%u,%u): ",
			tetra_get_llc_pdut_dec_name(lpp.pdu_type), lpp.ns, lpp.ss);
	if (lpp.tl_sdu && lpp.ss == 0) {
		msg->l3h = lpp.tl_sdu;
		rx_tl_sdu(tms, msg, lpp.tl_sdu_len);
	}
	return len;
}

static void rx_resrc(struct tetra_tmvsap_prim *tmvp, struct tetra_mac_state *tms)
{
	struct msgb *msg = tmvp->oph.msg;
	struct tetra_resrc_decoded rsd;
	int tmpdu_offset;
	char tmpstr[1380];
	time_t tp=time(0);

	memset(&rsd, 0, sizeof(rsd));
	tmpdu_offset = macpdu_decode_resource(&rsd, msg->l1h);
	msg->l2h = msg->l1h + tmpdu_offset;

	printf("RESOURCE Encr=%u, Length=%d Addr=%s ",
			rsd.encryption_mode, rsd.macpdu_length,
			tetra_addr_dump(&rsd.addr));

	if (rsd.addr.type == ADDR_TYPE_NULL)
		goto out;

	if (rsd.chan_alloc_pres)
		printf("ChanAlloc=%s ", tetra_alloc_dump(&rsd.cad, tms));
	if (rsd.slot_granting.pres)
		printf("SlotGrant=%u/%u ", rsd.slot_granting.nr_slots,
				rsd.slot_granting.delay);

	if (rsd.macpdu_length > 0 && rsd.encryption_mode == 0) {
		int len_bits = rsd.macpdu_length*8;
		if (msg->l2h + len_bits > msg->l1h + msgb_l1len(msg))
			len_bits = msgb_l1len(msg) - tmpdu_offset;
		rx_tm_sdu(tms, msg, len_bits);
	}
out:

	/* sq5bpf */
	//if (rsd.encryption_mode==0) 
	{
		uint8_t *bits = msg->l3h;
		uint8_t mle_pdisc=0;
		uint8_t	req_type=0;
		uint16_t callident=0;
		int i;


		if (bits) {
			mle_pdisc= bits_to_uint(bits, 3);
			req_type=bits_to_uint(bits+3, 5);
			callident=bits_to_uint(bits+8, 14);
		}
		printf("sq5bpf req mle_pdisc=%i req=%i ",mle_pdisc,req_type);

		if (mle_pdisc==TMLE_PDISC_CMCE) {

			sprintf(tmpstr,"TETMON_begin FUNC:%s SSI:%8.8i IDX:%3.3i IDT:%i ENCR:%i RX:%i TETMON_end",tetra_get_cmce_pdut_name(req_type, 0),rsd.addr.ssi,rsd.addr.usage_marker,rsd.addr.type,rsd.encryption_mode,tetra_hack_rxid);
			sendto(tetra_hack_live_socket, (char *)&tmpstr, 128, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);
			//printf("\nSQ5BPF KOMUNIKAT: [%s]\n",tmpstr);
		}



	}


	printf("\n");
}

static void rx_suppl(struct tetra_tmvsap_prim *tmvp, struct tetra_mac_state *tms)
{
	//struct tmv_unitdata_param *tup = &tmvp->u.unitdata;
	struct msgb *msg = tmvp->oph.msg;
	//struct tetra_suppl_decoded sud;
	int tmpdu_offset;

#if 0
	memset(&sud, 0, sizeof(sud));
	tmpdu_offset = macpdu_decode_suppl(&sud, msg->l1h, tup->lchan);
#else
	{
		uint8_t slot_granting = *(msg->l1h + 17);
		if (slot_granting)
			tmpdu_offset = 17+1+8;
		else
			tmpdu_offset = 17+1;
	}
#endif

	printf("SUPPLEMENTARY MAC-D-BLOCK ");

	//if (sud.encryption_mode == 0)
	msg->l2h = msg->l1h + tmpdu_offset;
	rx_tm_sdu(tms, msg, 100);

	printf("\n");
}

static void dump_access(struct tetra_access_field *acc, unsigned int num)
{
	printf("ACCESS%u: %c/%u ", num, 'A'+acc->access_code, acc->base_frame_len);
}

static void rx_aach(struct tetra_tmvsap_prim *tmvp, struct tetra_mac_state *tms)
{
	struct tmv_unitdata_param *tup = &tmvp->u.unitdata;
	struct tetra_acc_ass_decoded aad;

	printf("ACCESS-ASSIGN PDU: ");

	memset(&aad, 0, sizeof(aad));
	macpdu_decode_access_assign(&aad, tmvp->oph.msg->l1h,
			tup->tdma_time.fn == 18 ? 1 : 0);

	if (aad.pres & TETRA_ACC_ASS_PRES_ACCESS1)
		dump_access(&aad.access[0], 1);
	if (aad.pres & TETRA_ACC_ASS_PRES_ACCESS2)
		dump_access(&aad.access[1], 2);
	if (aad.pres & TETRA_ACC_ASS_PRES_DL_USAGE)
		printf("DL_USAGE: %s ", tetra_get_dl_usage_name(aad.dl_usage));
	if (aad.pres & TETRA_ACC_ASS_PRES_UL_USAGE)
		printf("UL_USAGE: %s ", tetra_get_ul_usage_name(aad.ul_usage));

	/* save the state whether the current burst is traffic or not */
	if (aad.dl_usage > 3)
		//sq5bpf tms->cur_burst.is_traffic = 1;
		tms->cur_burst.is_traffic = aad.dl_usage;
	else
		tms->cur_burst.is_traffic = 0;

	printf("\n");
}

static int rx_tmv_unitdata_ind(struct tetra_tmvsap_prim *tmvp, struct tetra_mac_state *tms)
{
	struct tmv_unitdata_param *tup = &tmvp->u.unitdata;
	struct msgb *msg = tmvp->oph.msg;
	uint8_t pdu_type = bits_to_uint(msg->l1h, 2);
	const char *pdu_name;
	struct msgb *gsmtap_msg;

	if (tup->lchan == TETRA_LC_BSCH)
		pdu_name = "SYNC";
	else if (tup->lchan == TETRA_LC_AACH)
		pdu_name = "ACCESS-ASSIGN";
	else {
		pdu_type = bits_to_uint(msg->l1h, 2);
		pdu_name = tetra_get_macpdu_name(pdu_type);
	}

	printf("TMV-UNITDATA.ind %s %s CRC=%u %s\n",
			tetra_tdma_time_dump(&tup->tdma_time),
			tetra_get_lchan_name(tup->lchan),
			tup->crc_ok, pdu_name);

	if (!tup->crc_ok)
		return 0;

	gsmtap_msg = tetra_gsmtap_makemsg(&tup->tdma_time, tup->lchan,
			tup->tdma_time.tn,
			/* FIXME: */ 0, 0, 0,
			msg->l1h, msgb_l1len(msg));
	if (gsmtap_msg)
		tetra_gsmtap_sendmsg(gsmtap_msg);

	switch (tup->lchan) {
		case TETRA_LC_AACH:
			rx_aach(tmvp, tms);
			break;
		case TETRA_LC_BNCH:
		case TETRA_LC_UNKNOWN:
		case TETRA_LC_SCH_F:
			switch (pdu_type) {
				case TETRA_PDU_T_BROADCAST:
					rx_bcast(tmvp, tms);
					break;
				case TETRA_PDU_T_MAC_RESOURCE:
					rx_resrc(tmvp, tms);
					break;
				case TETRA_PDU_T_MAC_SUPPL:
					rx_suppl(tmvp, tms);
					break;
				case TETRA_PDU_T_MAC_FRAG_END:
					if (msg->l1h[3] == TETRA_MAC_FRAGE_FRAG) {
						printf("FRAG/END FRAG: ");
						msg->l2h = msg->l1h+4;
						rx_tm_sdu(tms, msg, 100 /*FIXME*/);
						printf("\n");
					} else
						printf("FRAG/END END\n");
					break;
				default:
					printf("STRANGE pdu=%u\n", pdu_type);
					break;
			}
			break;
		case TETRA_LC_BSCH:
			break;
		default:
			printf("STRANGE lchan=%u\n", tup->lchan);
			break;
	}

	return 0;
}

int upper_mac_prim_recv(struct osmo_prim_hdr *op, void *priv)
{
	struct tetra_tmvsap_prim *tmvp;
	struct tetra_mac_state *tms = priv;
	int rc;

	switch (op->sap) {
		case TETRA_SAP_TMV:
			tmvp = (struct tetra_tmvsap_prim *) op;
			rc = rx_tmv_unitdata_ind(tmvp, tms);
			break;
		default:
			printf("primitive on unknown sap\n");
			break;
	}

	talloc_free(op->msg);
	talloc_free(op);

	return rc;
}
