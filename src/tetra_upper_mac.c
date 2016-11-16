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

	tetra_hack_freq_band=sid.freq_band;
	tetra_hack_freq_offset=sid.freq_offset;

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

const char *tetra_alloc_dump(const struct tetra_chan_alloc_decoded *cad, struct tetra_mac_state *tms, int send_telive_msg)
{
	static char buf[64];
	char *cur = buf;
	unsigned int freq_band, freq_offset;
	char freqinfo[128];

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
	if (send_telive_msg) {
		switch (cad->ul_dl) {

			case 3: /* uplink + downlink */
				sprintf(freqinfo,"TETMON_begin FUNC:FREQINFO2 DLF:%i RX:%i TETMON_end",tetra_dl_carrier_hz(freq_band, cad->carrier_nr, freq_offset),tetra_hack_rxid);
				sendto(tetra_hack_live_socket, (char *)&freqinfo, 128, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);
				break;

			default:
				break;
		}
	}
	return buf;
}

/* sq5bpf */
int parse_d_release(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len)
{
	uint8_t *bits = msg->l3h+3;
	int n=0;
	int m=0;
	char tmpstr2[1024];
	char *nis;
	int tmpdu_offset;
	struct tetra_resrc_decoded rsd;

	memset(&rsd, 0, sizeof(rsd));
	tmpdu_offset = macpdu_decode_resource(&rsd, msg->l1h);
	/* strona 270 */
	m=5; uint8_t pdu_type=bits_to_uint(bits+n, m); n=n+m;
	m=14; uint16_t callident=bits_to_uint(bits+n, m); n=n+m;
	m=5; uint16_t disccause=bits_to_uint(bits+n, m); n=n+m;
	m=6; uint16_t notifindic=bits_to_uint(bits+n, m); n=n+m;
	nis=(notifindic<28)?notification_indicator_strings[notifindic]:"Reserved";
	printf("\nCall identifier:%i Discconnect cause:%i NotificationID:%i (%s)\n",callident,disccause,notifindic,nis);
	sprintf(tmpstr2,"TETMON_begin FUNC:DRELEASEDEC SSI:%i CID:%i NID:%i [%s] RX:%i TETMON_end",rsd.addr.ssi,callident, notifindic,nis,tetra_hack_rxid);
	sendto(tetra_hack_live_socket, (char *)&tmpstr2, strlen((char *)&tmpstr2)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);

}
/* sq5bpf */
int parse_d_connect(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len) {
	uint8_t *bits = msg->l3h+3;
	int n=0;
	int m=0;
	char *nis;
	int tmpdu_offset;
	struct tetra_resrc_decoded rsd;
	char buf[1024];
	char buf2[128];

	memset(&rsd, 0, sizeof(rsd));
	tmpdu_offset = macpdu_decode_resource(&rsd, msg->l1h);
	/* strona 266 */
	m=5; uint8_t pdu_type=bits_to_uint(bits+n, m); n=n+m;
	m=14; uint16_t callident=bits_to_uint(bits+n, m); n=n+m;
	m=4; uint8_t call_timeout=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t hook_method_sel=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t duplex_sel=bits_to_uint(bits+n, m); n=n+m;
	m=2; uint8_t tx_grant=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t tx_req_permission=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t call_ownership=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t o_bit=bits_to_uint(bits+n, m); n=n+m;
	printf("\nCall Identifier:%i Call timeout:%i hook_method:%i Duplex:%i TX_Grant:%i TX_Request_permission:%i Call ownership:%i\n",callident,call_timeout,hook_method_sel,duplex_sel,tx_grant,tx_req_permission,call_ownership);
	sprintf(buf,"TETMON_begin FUNC:DCONNECTDEC SSI:%i IDX:%i CID:%i CALLOWN:%i",rsd.addr.ssi,rsd.addr.usage_marker,callident,call_ownership);
	if (o_bit) {

		m=1; uint8_t pbit_callpri=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_callpri) {
			m=4; uint8_t callpri=bits_to_uint(bits+n, m); n=n+m;
			printf("Call priority:%i ",callpri);	
		}

		m=1; uint8_t pbit_bsi=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_bsi) {
			m=8; uint8_t basic_service_information=bits_to_uint(bits+n, m); n=n+m;
			printf("Basic service information:%i ", basic_service_information);
		}

		m=1; uint8_t pbit_tmpaddr=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_tmpaddr) {
			m=24; uint32_t temp_addr=bits_to_uint(bits+n, m); n=n+m;
			printf("Temp address:%i ",temp_addr);
			sprintf(buf2," SSI2:%i",temp_addr);
			strcat(buf,buf2);
		}

		m=1; uint8_t pbit_nid=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_nid) {
			m=6; uint8_t notifindic=bits_to_uint(bits+n, m); n=n+m;
			nis=(notifindic<28)?notification_indicator_strings[notifindic]:"Reserved";
			printf("Notification indicator:%i [%s] ",notifindic,nis);
			sprintf(buf2," NID:%i [%s]",notifindic,nis);
			strcat(buf,buf2);

		}
		printf("\n");
	}
	sprintf(buf2," RX:%i TETMON_end",tetra_hack_rxid);
	strcat(buf,buf2);
	sendto(tetra_hack_live_socket, (char *)&buf, strlen((char *)&buf)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);

}

int parse_d_txgranted(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len) {
	uint8_t *bits = msg->l3h+3;
	int n=0;
	int m=0;
	char *nis;
	int tmpdu_offset;
	struct tetra_resrc_decoded rsd;
	char buf[1024];
	char buf2[128];

	memset(&rsd, 0, sizeof(rsd));
	tmpdu_offset = macpdu_decode_resource(&rsd, msg->l1h);
	/* strona 271 */
	m=5; uint8_t pdu_type=bits_to_uint(bits+n, m); n=n+m;
	m=14; uint16_t callident=bits_to_uint(bits+n, m); n=n+m;
	m=2; uint8_t tx_grant=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t tx_req_permission=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t enc_control=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t reserved=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t o_bit=bits_to_uint(bits+n, m); n=n+m;
	printf("\nCall Identifier:%i TX_Grant:%i TX_Request_permission:%i Encryption control:%i\n",callident,tx_grant,tx_req_permission,enc_control);
	sprintf(buf,"TETMON_begin FUNC:DTXGRANTDEC SSI:%i IDX:%i CID:%i TXGRANT:%i TXPERM:%i ENCC:%i",rsd.addr.ssi,rsd.addr.usage_marker,callident,tx_grant,tx_req_permission,enc_control);
	if (o_bit) {
		m=1; uint8_t pbit_nid=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_nid) {
			m=6; uint8_t notifindic=bits_to_uint(bits+n, m); n=n+m;
			nis=(notifindic<28)?notification_indicator_strings[notifindic]:"Reserved";
			printf("Notification indicator:%i [%s] ",notifindic,nis);
			sprintf(buf2," NID:%i [%s]",notifindic,nis);
			strcat(buf,buf2);

		}
		m=1; uint8_t pbit_tpti=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_tpti) {
			m=2; uint8_t tpti=bits_to_uint(bits+n, m); n=n+m;
			uint32_t txssi;
			uint32_t txssiext;

			sprintf(buf2," TPTI:%i",tpti);
			strcat(buf,buf2);

			switch(tpti)
			{
				case 0: /* SNA , this isn't defined for D-TX GRANTED */
					m=8; txssi=bits_to_uint(bits+n, m); n=n+m;
					sprintf(buf2," SSI2:%i",txssi);
					strcat(buf,buf2);

					break;
				case 1: /* SSI */
					m=24; txssi=bits_to_uint(bits+n, m); n=n+m;
					sprintf(buf2," SSI2:%i",txssi);
					strcat(buf,buf2);
					break;
				case 2: /* TETRA Subscriber Identity (TSI) */
					m=24; txssi=bits_to_uint(bits+n, m); n=n+m;
					m=24; txssiext=bits_to_uint(bits+n, m); n=n+m;
					sprintf(buf2," SSI2:%i SSIEXT:%i",txssi,txssiext);
					strcat(buf,buf2);
					break;
				case 3: /* reserved ? */
					break;
			}


		}
		/* TODO: type 3/4 elements */
		printf("\n");
	}
	sprintf(buf2," RX:%i TETMON_end",tetra_hack_rxid);
	strcat(buf,buf2);
	sendto(tetra_hack_live_socket, (char *)&buf, strlen((char *)&buf)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);
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
	uint16_t notifindic=0;
	uint32_t tempaddr=0;
	uint16_t cpti=0;

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
	m=1; uint8_t obit=bits_to_uint(bits+n, m); n=n+m;
	if (obit) 
	{
		m=1; uint8_t pbit_notifindic=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_notifindic) {
			m=6;  notifindic=bits_to_uint(bits+n, m); n=n+m;
		}
		m=1; uint8_t pbit_tempaddr=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_tempaddr) {
			m=24;  tempaddr=bits_to_uint(bits+n, m); n=n+m;
		}
		m=1; uint8_t pbit_cpti=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_cpti) {
			m=2;  cpti=bits_to_uint(bits+n, m); n=n+m;
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
		}

	}
	printf ("\nCall identifier:%i  Call timeout:%i  Hookmethod:%i  Duplex:%i\n",callident,calltimeout,hookmethod,duplex);
	printf("Basicinfo:0x%2.2X  Txgrant:%i  TXperm:%i  Callprio:%i\n",basicinfo,txgrant,txperm,callprio);
	printf("NotificationID:%i  Tempaddr:%i CPTI:%i  CallingSSI:%i  CallingExt:%i\n",notifindic,tempaddr,cpti,callingssi,callingext);

	sprintf(tmpstr2,"TETMON_begin FUNC:DSETUPDEC IDX:%i SSI:%i SSI2:%i CID:%i NID:%i RX:%i TETMON_end",rsd.addr.usage_marker,rsd.addr.ssi,callingssi,callident,notifindic,tetra_hack_rxid);
	sendto(tetra_hack_live_socket, (char *)&tmpstr2, strlen((char *)&tmpstr2)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);
}

/* decode 18.5.17 Neighbour cell information for CA */
/* str 535, przyklad str 1294 */
int parse_nci_ca( uint8_t *bits)
{
	int n,m;
	char buf[1024];
	char buf2[128];
	char freqinfo[128];
	n=0;
	m=5; uint8_t cell_id=bits_to_uint(bits+n, m); n=n+m;
	m=2; uint8_t cell_reselection=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t neig_cell_synced=bits_to_uint(bits+n, m); n=n+m;
	m=2; uint8_t cell_load=bits_to_uint(bits+n, m); n=n+m;
	m=12; uint16_t main_carrier_num=bits_to_uint(bits+n, m); n=n+m;
	/* the band and offset info is from the sysinfo message, not sure if this is correct */
	sprintf(buf," NCI:[cell_id:%i cell_resel:%i neigh_synced:%i cell_load:%i carrier:%i %iHz",cell_id,cell_reselection,neig_cell_synced,cell_load,main_carrier_num,tetra_dl_carrier_hz(tetra_hack_freq_band, main_carrier_num, tetra_hack_freq_offset));

	sprintf(freqinfo,"TETMON_begin FUNC:FREQINFO1 DLF:%i",tetra_dl_carrier_hz(tetra_hack_freq_band, main_carrier_num, tetra_hack_freq_offset));

	m=1; uint8_t obit=bits_to_uint(bits+n, m); n=n+m;
	if (obit) {
		m=1; uint8_t pbit_main_carrier_num_ext=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_main_carrier_num_ext) {
			m=4; uint8_t freq_band=bits_to_uint(bits+n, m); n=n+m;
			m=2; uint8_t freq_offset=bits_to_uint(bits+n, m); n=n+m;
			m=3; uint8_t duplex_spacing=bits_to_uint(bits+n, m); n=n+m;
			m=1; uint8_t reverse=bits_to_uint(bits+n, m); n=n+m;
			uint32_t dlfext=tetra_dl_carrier_hz(freq_band, main_carrier_num, freq_offset);
			uint32_t ulfext=tetra_ul_carrier_hz(freq_band, main_carrier_num, freq_offset,duplex_spacing,reverse);

			sprintf(buf2," band:%i offset:%i freq:%iHz uplink:%iHz (duplex:%i rev:%i)",freq_band,freq_offset,dlfext,ulfext,duplex_spacing,reverse);
			strcat(buf,buf2);
			sprintf(buf2,"TETMON_begin FUNC:FREQINFO1 DLF:%i ULF:%i",dlfext, ulfext); 
			strcat(freqinfo,buf2);
		}
		m=1; uint8_t pbit_mcc=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_mcc) {
			m=10; uint16_t mcc=bits_to_uint(bits+n, m); n=n+m;
			sprintf(buf2," MCC:%i",mcc);
			strcat(buf,buf2);
			sprintf(buf2," MCC:%4.4x",mcc);
			strcat(freqinfo,buf2);
		}

		m=1; uint8_t pbit_mnc=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_mnc) {
			m=14; uint16_t mnc=bits_to_uint(bits+n, m); n=n+m;
			sprintf(buf2," MNC:%i",mnc);
			strcat(buf,buf2);
			sprintf(buf2," MNC:%4.4x",mnc);
			strcat(freqinfo,buf2);
		}

		m=1; uint8_t pbit_la=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_la) {
			m=14; uint16_t la=bits_to_uint(bits+n, m); n=n+m;
			sprintf(buf2," LA:%i",la);
			strcat(buf,buf2);
			strcat(freqinfo,buf2);
		}

		m=1; uint8_t pbit_max_ms_txpower=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_max_ms_txpower) {
			m=3; uint8_t max_ms_txpower=bits_to_uint(bits+n, m); n=n+m;
		}

		m=1; uint8_t pbit_min_rx_level=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_min_rx_level) {
			m=4; uint8_t min_rx_level=bits_to_uint(bits+n, m); n=n+m;
		}

		m=1; uint8_t pbit_subscr_class=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_subscr_class) {
			m=16; uint16_t subscr_class=bits_to_uint(bits+n, m); n=n+m;
		}

		m=1; uint8_t pbit_bs_srv_details=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_bs_srv_details) {
			m=12; uint16_t bs_srv_details=bits_to_uint(bits+n, m); n=n+m;
		}

		m=1; uint8_t pbit_timeshare_info=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_timeshare_info) {
			m=5; uint8_t timeshare_info=bits_to_uint(bits+n, m); n=n+m;
		}

		m=1; uint8_t pbit_tdma_frame_offset=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_tdma_frame_offset) {
			m=6; uint8_t tdma_frame_offset=bits_to_uint(bits+n, m); n=n+m;
		}
	}
	sprintf(buf2,"] ");
	strcat(buf,buf2);
	printf("%s",buf);

	sprintf(buf2," RX:%i TETMON_end",tetra_hack_rxid);
	strcat(freqinfo,buf2);
	sendto(tetra_hack_live_socket, (char *)&freqinfo, 128, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);

	return(n);
}

uint parse_d_nwrk_broadcast(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len)
{
	uint8_t *bits = msg->l3h;
	int n,m,i;

	/* TMLE_PDISC_MLE 3 bits
	 * TMLE_PDUT_D_NWRK_BROADCAST 3 bits */
	n=3+3;

	m=16; uint16_t cell_reselect_parms=bits_to_uint(bits+n, m); n=n+m;
	m=2; uint16_t cell_load=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint16_t optional_elements=bits_to_uint(bits+n, m); n=n+m;
	printf("\nD_NWRK_BROADCAST:[ cell_reselect:0x%4.4x cell_load:%i", cell_reselect_parms,cell_load);
	if (optional_elements) {
		m=1; uint16_t pbit_tetra_time=bits_to_uint(bits+n, m); n=n+m;
		if (pbit_tetra_time) 
		{
			m=24; uint32_t tetra_time_utc=bits_to_uint(bits+n, m); n=n+m;
			m=1; uint8_t tetra_time_offset_sign=bits_to_uint(bits+n, m); n=n+m;
			m=6; uint8_t tetra_time_offset=bits_to_uint(bits+n, m); n=n+m;
			m=6; uint8_t tetra_time_year=bits_to_uint(bits+n, m); n=n+m;
			m=11; uint16_t tetra_time_reserved=bits_to_uint(bits+n, m); n=n+m; /* must be 0x7ff */
			printf(" time[secs:%i offset:%c%imin year:%i reserved:0x%4.4x]",tetra_time_utc,tetra_time_offset_sign?'-':'+',tetra_time_offset*15,2000+tetra_time_year,tetra_time_reserved);
			/* we could decode the time here, but it is not accurate on the networks that i see anyway */
		}


		m=1; uint16_t pbit_neigh_cells=bits_to_uint(bits+n, m); n=n+m;

		//	printf(" pbit_tetra_time:%i pbit_neigh_cells:%i",pbit_tetra_time,pbit_neigh_cells);
		if (pbit_neigh_cells) 
		{
			m=3; uint16_t num_neigh_cells=bits_to_uint(bits+n, m); n=n+m;
			printf(" num_cells:%i",num_neigh_cells);
			for (i=0;i<num_neigh_cells;i++) {
				m=parse_nci_ca(bits+n); n=n+m;
			}

		}


	}
	printf("] RX:%i\n",tetra_hack_rxid);

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

				case TCMCE_PDU_T_D_CONNECT:
					parse_d_connect(tms,msg,len);
					break;

				case TCMCE_PDU_T_D_RELEASE:
					parse_d_release(tms,msg,len);
					break;

				case TCMCE_PDU_T_D_TX_GRANTED:
					parse_d_txgranted(tms,msg,len);
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
			/* parse d-nwrk-broadcast */
			switch(bits_to_uint(bits+3, 3))
			{
				case TMLE_PDUT_D_NWRK_BROADCAST:
					parse_d_nwrk_broadcast(tms,msg,len);
				default:
					break;
			}
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

/* add bits to a fragment. these should really be bit operations and not stuffing one bit per byte */
void append_frag_bits(int slot,uint8_t *bits,int bitlen,int fillbits)
{
	int i=bitlen;
	int l=fragslots[slot].length;
	struct msgb *fragmsgb;
	uint8_t bit;
	int zeroes=0;

	fragmsgb= fragslots[slot].msgb;

	while(i) {
		bit=bits_to_uint(bits, 1);
		msgb_put_u8(fragmsgb,bit);
		if (bit) { zeroes=0; } else { zeroes++; }
		bits++;
		i--;
		l++;
		if (l>4095) { printf("\nFRAG LENGTH ERROR!\n"); return; } /* limit hardcoded for now, the buffer allocated is twice the size just in case */
	}

	fragslots[slot].length=fragslots[slot].length+bitlen;

	if (fillbits) {
		fragslots[slot].length=fragslots[slot].length-zeroes;
		msgb_get(fragmsgb,zeroes);
	}

	fragslots[slot].fragments++;
	fragslots[slot].fragtimer=0;
	/*
	 * printf("\nappend_frag slot=%i len=%i totallen=%i fillbits=%i\n",slot,bitlen,fragslots[slot].length,fillbits);
	 * printf("\nFRAGDUMP: %s\n",osmo_ubit_dump((unsigned char *)fragmsgb->l3h,msgb_l3len(fragmsgb)));
	 */

}

/* MAC-FRAG PDU */
static void rx_macfrag(struct tetra_tmvsap_prim *tmvp, struct tetra_mac_state *tms,int slot)
{
	struct msgb *msg = tmvp->oph.msg;
	struct tetra_resrc_decoded rsd;
	uint8_t *bits = msg->l1h;
	int n=0;
	int m=0;

	memset(&rsd, 0, sizeof(rsd));
	m=2; uint8_t macpdu_type=bits_to_uint(bits+n, m); n=n+m; /*  MAC-FRAG/END */
	m=1; uint8_t macpdu_subtype=bits_to_uint(bits+n, m); n=n+m; /* 0 - MAC-FRAG */
	m=1; uint8_t fillbits_present=bits_to_uint(bits+n, m); n=n+m;
	int len=msgb_l1len(msg) - n;

	if (fragslots[slot].active) {
		append_frag_bits(slot,bits+n,len,fillbits_present);
	} else {
		printf("\nFRAG: got fragment without start packet for slot=%i\n",slot);
	}
}

/* 21.4.3.3 MAC-END PDU page 618 */
static void rx_macend(struct tetra_tmvsap_prim *tmvp, struct tetra_mac_state *tms,int slot)
{
	struct msgb *msg = tmvp->oph.msg;
	struct tetra_resrc_decoded rsd;
	int tmpdu_offset;
	uint8_t *bits = msg->l1h;
	struct msgb *fragmsgb;
	int n=0;
	int m=0;

	memset(&rsd, 0, sizeof(rsd));

	m=2; uint8_t macpdu_type=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t macpdu_subtype=bits_to_uint(bits+n, m); n=n+m;
	m=1; uint8_t fillbits_present=bits_to_uint(bits+n, m); n=n+m;
	m=6; uint8_t length_indicator=bits_to_uint(bits+n, m); n=n+m;
	/* FIXME: we should really look at the modulation and handle d8psk and qam */
	/* m=1; uint8_t napping=bits_to_uint(bits+n, m); n=n+m; // only  in d8psk  and qam */
	m=1; uint8_t slot_granting=bits_to_uint(bits+n, m); n=n+m;
	if (slot_granting) {
		/* m=1; uint8_t multiple=bits_to_uint(bits+n, m); n=n+m; // only  in  qam */
		m=8; /* basic slot granting */ n=n+m;
		/* multiple slot granting in qam */

	}
	m=1; uint8_t chanalloc=bits_to_uint(bits+n, m); n=n+m;

	if (chanalloc) {
		m=decode_chan_alloc(&rsd.cad, bits+n); n=n+m;

	}
	int len=msgb_l1len(msg) - n;

	fragmsgb=fragslots[slot].msgb;

	fragslots[slot].fragments++;
	if (fragslots[slot].active) {
		append_frag_bits(slot,bits+n,len,fillbits_present);


		/* for now filter out just SDS messages to hide the fact that the fragment stuff doesn't work 100% correctly :) */
		uint8_t *b = fragmsgb->l3h;

		if (b) {
			uint8_t mle_pdisc = bits_to_uint(b, 3);
			uint8_t proto=bits_to_uint(b+3, 5);
			if ((mle_pdisc==TMLE_PDISC_CMCE)&&(proto==TCMCE_PDU_T_D_SDS_DATA)) {
				printf("\nFRAGMENT DECODE fragments=%i len=%i slot=%i Encr=%i ",fragslots[slot].fragments,fragslots[slot].length,slot,fragslots[slot].encryption);
				fflush(stdout); /* TODO: remove this in the future, for now leave it so that the printf() is shown if rx_tl_sdu segfaults for somee reason */
				rx_tl_sdu(tms, fragmsgb, fragslots[slot].length);
			}
		}
		else 
		{
			printf("\nFRAG: got end frag without start packet for slot=%i\n",slot);
		}
	} else {
		printf("\nFRAGMENT without l3 header dropped slot=%i\n",slot);

	}

	msgb_reset(fragmsgb);
	fragslots[slot].fragments=0;
	fragslots[slot].active=0;
	fragslots[slot].length=0;
	fragslots[slot].fragtimer=0;
}

void hexdump(unsigned char *c,int i)
{
	printf("\nHEXDUMP_%i: [",i);
	while (i) {
		printf("%2.2x ",(unsigned char)*c);
		c++;
		i--;
		fflush(stdout);
	}
	printf ("]\n");
}


static void rx_resrc(struct tetra_tmvsap_prim *tmvp, struct tetra_mac_state *tms, int slot)
{
	struct msgb *msg = tmvp->oph.msg;
	struct tetra_resrc_decoded rsd;
	int tmpdu_offset;
	struct msgb *fragmsgb;
	int tmplen;
	char tmpstr[1380];

	memset(&rsd, 0, sizeof(rsd));
	tmpdu_offset = macpdu_decode_resource(&rsd, msg->l1h);
	msg->l2h = msg->l1h + tmpdu_offset;

	printf("RESOURCE Encr=%u, Length=%d Addr=%s ",
			rsd.encryption_mode, rsd.macpdu_length,
			tetra_addr_dump(&rsd.addr));

	if (rsd.addr.type == ADDR_TYPE_NULL)
		goto out;
	if (rsd.chan_alloc_pres)
		printf("ChanAlloc=%s ", tetra_alloc_dump(&rsd.cad, tms,(rsd.encryption_mode==0)));
	if (rsd.slot_granting.pres)
		printf("SlotGrant=%u/%u ", rsd.slot_granting.nr_slots,
				rsd.slot_granting.delay);

	if ((tetra_hack_allow_encrypted)||(rsd.encryption_mode == 0)) {
		int len_bits = rsd.macpdu_length*8;
		if (msg->l2h + len_bits > msg->l1h + msgb_l1len(msg))
			len_bits = msgb_l1len(msg) - tmpdu_offset;
		if (rsd.macpdu_length>0) {
			rx_tm_sdu(tms, msg, len_bits);
		} 
		else 
		{
			if ((tetra_hack_reassemble_fragments)&&(rsd.macpdu_length==MACPDU_LEN_START_FRAG)) {
				int len=msgb_l1len(msg) - tmpdu_offset;

				if (fragslots[slot].active) printf("\nWARNING: leftover fragment slot\n");

				fragmsgb=fragslots[slot].msgb;

				/* printf ("\nFRAGMENT START slot=%i msgb=%p\n",slot,fragmsgb); */
				msgb_reset(fragmsgb);

				fragslots[slot].active=1;
				fragslots[slot].fragments=0;
				/* copy the original msgb */
				tmplen=msg->tail - msg->data;
				memcpy(msgb_put(fragmsgb,tmplen),msg->data, tmplen);
				if (msg->l1h) {
					fragmsgb->l1h=((void *)msg->l1h-(void *)msg)+(void *)fragmsgb;
				} else {
					fragmsgb->l1h=0;
				}
				if (msg->l2h) {
					fragmsgb->l2h=((void *)msg->l2h-(void *)msg)+(void *)fragmsgb;
				} else {
					fragmsgb->l2h=0;
				}

				struct tetra_llc_pdu lpp;

				memset(&lpp, 0, sizeof(lpp));
				tetra_llc_pdu_parse(&lpp,  (uint8_t *)fragmsgb->l2h,  msgb_l2len(fragmsgb));

				if (lpp.tl_sdu && lpp.ss == 0) {
					fragmsgb->l3h = lpp.tl_sdu;
				} else {
					fragmsgb->l3h = 0;
				}
				fragslots[slot].length=lpp.tl_sdu_len; /* not sure if this is the correct way to get the accurate length */

				fragslots[slot].encryption=rsd.encryption_mode;

				fragslots[slot].active=1;
				fragslots[slot].fragments=1;

				return;
			}

		}
	}
out:

	/* sq5bpf */
	//if (rsd.encryption_mode==0) 
	{
		uint8_t *bits = msg->l3h;
		uint8_t mle_pdisc=0;
		uint8_t	req_type=0;
		uint16_t callident=0;


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
	uint8_t pdu_frag_subtype;

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

	int slot=tup->tdma_time.tn;

	/* age out old fragments */
	if ((tetra_hack_reassemble_fragments)&&(tup->tdma_time.fn==18)) {
		int i;
		for (i=0;i<FRAGSLOT_NR_SLOTS;i++) {
			if (fragslots[i].active) {
				fragslots[i].fragtimer++;
				if (fragslots[i].fragtimer>N203) {
					printf("\nFRAG: aged out old fragments for slot=%i fragments=%i length=%i timer=%i\n",i,fragslots[i].fragments,fragslots[i].length, fragslots[i].fragtimer);
					msgb_reset(fragslots[i].msgb);
					fragslots[i].fragments=0;
					fragslots[i].active=0;
					fragslots[i].length=0;
					fragslots[i].fragtimer=0;
				}

			}
		}
	}

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
					rx_resrc(tmvp, tms, slot);
					break;
				case TETRA_PDU_T_MAC_SUPPL:
					rx_suppl(tmvp, tms);
					break;
				case TETRA_PDU_T_MAC_FRAG_END:
					pdu_frag_subtype = bits_to_uint(msg->l1h+2, 1);

					if (msg->l1h[3] == TETRA_MAC_FRAGE_FRAG) {
						printf("FRAG/END FRAG: ");
						msg->l2h = msg->l1h+4;
						if (tetra_hack_reassemble_fragments) {
							rx_macfrag(tmvp, tms,slot);
						} else {
							rx_tm_sdu(tms, msg, 100 /*FIXME*/);
						}
						printf("\n");
					} else
						printf("FRAG/END END\n");
					if (tetra_hack_reassemble_fragments) rx_macend(tmvp, tms,slot);
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
