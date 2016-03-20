/* tetra_sds.c - SDS helper functions 
 * 
 *
 * (C) 2014-2015 by Jacek Lipkowski <sq5bpf@lipkowski.org>
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
#include <math.h>


const char oth_reserved[]="Other Reserved";

int decode_pdu(char *dec,unsigned char *enc,int len)
{
	int outlen=0;
	int nbits=0;
	unsigned char carry=0;
	while(len) {
		*dec=carry|((*enc<<(nbits))&0x7f);
		carry=*enc>>(7-nbits);
		nbits++;
		dec++;
		outlen++;
		enc++;
		len--;
		if (nbits==7) {
			*dec=carry;
			dec++;
			nbits=0;
			outlen++;
		}
	}
	*dec=0;
	return(outlen);
}



char *get_sds_type(uint8_t type) {
	struct sds_type* a= (struct sds_type *)&sds_types;	
	while (a->description)
	{
		if (a->type==type) {
			return(a->description);
		}
		a++;
	}
	return((char *)&oth_reserved);

}


/* TODO: move this into tetra_sds.c and break into multiple functions */
unsigned int parse_d_sds_data(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len)
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
	char descr[4096];
	char tmpstr[128];
	char tmpstr2[1024];
	int is_sdstl=0;
	uint8_t  message_type;
	struct tetra_resrc_decoded rsd;
	int tmpdu_offset;
	uint8_t sdstl_delivery_report_request;
	uint8_t sdstl_service_selection;
	uint8_t sdstl_storage_control;
	uint8_t sdstl_message_reference;
	uint8_t sdstl_validity_period;
	uint8_t sdstl_forw_address_type;
	uint32_t sdstl_forw_address;
	uint32_t sdstl_forw_address_ext;
	uint8_t sdstl_extnum_digits;
	uint8_t sdstl_extnum_digit;
	int decode_sds=1;


	memset(&rsd, 0, sizeof(rsd));
	tmpdu_offset = macpdu_decode_resource(&rsd, msg->l1h);



	m=5; pdu_type=bits_to_uint(bits+n, m); n=n+m;
	m=2; cpti=bits_to_uint(bits+n, m); n=n+m;
	switch(cpti) {
		case 0: /* SNA */
			m=8; calling_ssi=bits_to_uint(bits+n, m); n=n+m;
			break;
		case 1: /* SSI */
			m=24; calling_ssi=bits_to_uint(bits+n, m); n=n+m;
			break;
		case 2: /* TETRA Subscriber Identity (TSI) */
			m=24; calling_ssi=bits_to_uint(bits+n, m); n=n+m;
			m=24; calling_ext=bits_to_uint(bits+n, m); n=n+m;
			break;
		default:
			/* hgw co to jest */
			printf("\nparse_d_sds_data: BAD CPTI %i\n",cpti);
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

			/* SDS-TL header */
			if (protoid&0x80) {
				is_sdstl=1;
				m=4; message_type=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
				strcat(descr,"SDS-TL:[ MsgType:");

				switch(message_type) {
					case 0: /* SDS-TRANSFER page 1182 */
						strcat(descr,"SDS-TRANSFER");
						m=2; sdstl_delivery_report_request=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m; /* page 1186 */
						m=1; sdstl_service_selection=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m; /* page 1189 1:to group*/
						m=1; sdstl_storage_control=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m; /* page 1189 */
						m=8; sdstl_message_reference=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;

						sprintf(tmpstr," MSG_REF:%i TO_GROUP:%i",sdstl_message_reference,sdstl_service_selection);
						strcat(descr,tmpstr);
						if (sdstl_storage_control) {
							/* Storage/forward control information is available */
							m=5; sdstl_validity_period=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
							m=3; sdstl_forw_address_type=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;

							sprintf(tmpstr," Validity_period:%i",sdstl_validity_period);
							strcat(descr,tmpstr);

							switch(sdstl_forw_address_type) {
								case 0: /* SNA */
									m=8; sdstl_forw_address=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
									sprintf(tmpstr," Forward_addr_SNA:%i",sdstl_forw_address);
									strcat(descr,tmpstr);
									break;
								case 1: /* SSI */
									m=24; sdstl_forw_address=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
									sprintf(tmpstr," Forward_addr_SSI:%i",sdstl_forw_address);
									strcat(descr,tmpstr);
									break;
								case 2: /* TETRA Subscriber Identity (TSI) */
									m=24; sdstl_forw_address=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
									m=24; sdstl_forw_address_ext=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
									sprintf(tmpstr," Forward_addr_SSI:%i:EXT:%i",sdstl_forw_address,sdstl_forw_address_ext);
									strcat(descr,tmpstr);
									break;
								case 3: /* External subscriber number */
									m=8; sdstl_extnum_digits=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
									strcat(descr," Forward_addr_EXT:");
									l=sdstl_extnum_digits;
									while(l) {
										m=4; sdstl_extnum_digit=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
										sprintf(tmpstr,"%c",'0'+sdstl_extnum_digit);
										strcat(descr,tmpstr);
										l--;
									}
									if (sdstl_extnum_digits&0x1) {
										/* odd number, eat dummy digit */
										n=n+4;
									}
									break;
								case 7: /* no forward address */
									strcat(descr," Forward_addr_none");
									break;
								default:
									/* hgw co to jest */
									sprintf(tmpstr," Forward_addr_BAD:%i",sdstl_forw_address_type);
									strcat(descr,tmpstr);
									return(1);
									break;

							}


						}

						break;
					case 1: /* SDS-REPORT */
						strcat(descr,"SDS-REPORT");
						n=n+12;
						decode_sds=0;
						break;

					case 2: /* SDS-ACK */
						strcat(descr,"SDS-ACK ");
						decode_sds=0;

						n=n+12;
						break;
					default:
						decode_sds=0;
						if (message_type&0x8) {
							sprintf(tmpstr,"Defined_by_application_%i ",message_type);
						} else {
							sprintf(tmpstr,"Reserved_for_add_msg_types_%i ",message_type);
						}
						strcat(descr,tmpstr);
						n=n+12;
						break;
				}

				strcat(descr,"] ");
			}

			if (datalen>2047) {  
				strcat(descr,"ERROR: SDS too long");
				decode_sds=0; 
			}

			uint8_t c;
			if (decode_sds) {
				switch (protoid) {
					case TETRA_SDS_PROTO_SIMPLE_LOC:
						sprintf(tmpstr,"SIMPLE_LOCATION_SYSTEM:[");
						strcat(descr,tmpstr);

						decode_simplelocsystem(tmpstr2, sizeof(tmpstr2),bits+n,datalen);
						strcat(descr,tmpstr2);
						sprintf(tmpstr,"]\n");
						strcat(descr,tmpstr);
						break;

					case TETRA_SDS_PROTO_LOCSYSTEM:
						sprintf(tmpstr,"LOCATION_SYSTEM:[");
						strcat(descr,tmpstr);

						decode_locsystem(tmpstr2, sizeof(tmpstr2),bits+n,datalen);
						strcat(descr,tmpstr2);
						sprintf(tmpstr,"]\n");
						strcat(descr,tmpstr);
						break;

					case TETRA_SDS_PROTO_LIP:
						sprintf(tmpstr,"LIP:[");
						strcat(descr,tmpstr);

						decode_lip(tmpstr2, sizeof(tmpstr2),bits+n,datalen);
						strcat(descr,tmpstr2);
						sprintf(tmpstr,"]");
						strcat(descr,tmpstr);


						break;

					case TETRA_SDS_PROTO_TXTMSG:
					case TETRA_SDS_PROTO_SIMPLE_TXTMSG:
					case TETRA_SDS_PROTO_SIMPLE_ITXTMSG:
					case TETRA_SDS_PROTO_ITXTMSG:
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
								while((datalen>=m)&&(n<=len)) {
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
								strcat(descr,"]");


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
								while((datalen>=m)&&(n<=len)) {
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
								strcat(descr,"]");
								break;
						}
						break;

					default:	
						sprintf(tmpstr,"DATA:[");
						strcat(descr,tmpstr);
						/* other message */
						/* hexdump */
						m=8;
						l=0;
						while((datalen>=m)&&(n<=len)) {
							udata[l]=bits_to_uint(bits+n, m); n=n+m;
							l++;
							datalen=datalen-m;

						}
						/* dump */
						for(a=0;a<l;a++) {
							sprintf(tmpstr,"0x%2.2X ",udata[a]);
							strcat(descr,tmpstr);
						}
						strcat(descr,"]");
						break;

				}	

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


char *get_lip_dirtravel_type(uint8_t type) {
	struct lip_dirtravel_type* a= (struct lip_dirtravel_type *)&lip_dirtravel_types;	
	while (a->description)
	{
		if (a->type==type) {
			return(a->description);
		}
		a++;
	}
	return((char *)&oth_reserved); /* this should be an assert + error */
}

float get_horiz_velocity(uint8_t lip_horiz_velocity) {
	if (lip_horiz_velocity<29) return(lip_horiz_velocity);
	return(16.0*powf(1.038,lip_horiz_velocity-13));
}

/* decode LIP Location reporting enable flags, skip reserved bits */
char *decode_liprepenflags(uint8_t flags)
{
	switch(flags>>6) {
		case 0: return ("[Reporting:disabled Backlog:disabled]"); break;
		case 1: return ("[Reporting:disabled Backlog:enabled]"); break;
		case 2: return ("[Reporting:enabled Backlog:disabled]"); break;
		case 3: return ("[Reporting:enabled Backlog:enabled]"); break;
	}
	return("BUG!"); /* can't happen :) */
}
/* decode LIP Report Type */
char *decode_lipreptype(uint8_t flags)
{
	/* from Table 6.95: Report type information element contents  */
	switch(flags) {
		case 0: return ("[Long location report preferred with no time information]"); break;
		case 1: return ("[Long location report preferred with time type 'Time elapsed']"); break;
		case 2: return ("[Long location report preferred with time type Time of position']"); break;
		case 3: return ("[Short location report preferred]"); break;
	}
	return("BUG!"); /* can't happen :) */
}

char *parse_lip_lattitude(uint32_t lip_lattitude) {
	char latdir;
	static char latstr[16];
	float lattitude;
	if (lip_lattitude&(1<<23)) {
		lattitude=(((1<<24)-lip_lattitude)*180.0)/(1.0*(1<<24)); latdir='S';
	} else
	{
		lattitude=(lip_lattitude*180.0)/(1.0*(1<<24)); latdir='N';
	}
	snprintf(latstr,sizeof(latstr),"%.6f%c",lattitude,latdir);
	return((char *)&latstr);
}

char *parse_lip_longtitude(uint32_t lip_longtitude) {
	char londir;
	float longtitude;
	static char lonstr[16];
	/* note: this should be 1<<25 for the calculations (according to the documentation), but for some reason 1<<24 is correct, maybe i skipped a bit somewhere? */
	if (lip_longtitude&(1<<24)) {
		longtitude=(((1<<25)-lip_longtitude)*180.0)/(1.0*(1<<24)); londir='W';
	} else
	{
		longtitude=(lip_longtitude*180.0)/(1.0*(1<<24)); londir='E';
	}		
	snprintf(lonstr,sizeof(lonstr),"%.6f%c",longtitude,londir);
	return((char *)&lonstr);
}

/* decode Location information protocol */
/* note: this started out as a simple and short function that got longer and longer and longer and .....
 * some day this mess will be broken up into smaller functions */
int decode_lip(char *out, int outlen,uint8_t *bits,int datalen)
{
	int m;
	int n=0;
	uint8_t lip_pdu_type; 
	float lattitude,longtitude;
	char latdir,londir;
	m=2; lip_pdu_type=bits_to_uint(bits+n, m); n=n+m;
	uint8_t lip_time_elapsed; 
	uint8_t lip_pdu_type_extension;
	uint32_t lip_longtitude; 
	uint32_t lip_lattitude; 
	uint8_t lip_pos_error; 
	uint8_t lip_horiz_velocity; 
	uint8_t lip_dir_travel; 
	uint8_t lip_type_adddata; 

	uint8_t lip_pdu_ackrequest;
	uint8_t lip_pdu_locrepflags;
	uint8_t lip_pdu_type_extreqresp;
	uint8_t lip_pdu_type_ackrequest;
	uint8_t lip_pdu_resultcode;
	uint8_t lip_pdu_minrepinterval;
	uint8_t lip_pdu_reporttype;
	char tmpstr[1024];
	char tmpstr2[128];
	uint32_t lip_pdu_timedatae;
	uint8_t lip_pdu_timedata;
	uint8_t lip_pdu_locdata;
	uint8_t lip_horizuncertainity;
	uint8_t lip_halfmajoraxis;
	uint8_t lip_halfminoraxis;
	uint8_t lip_angle;
	uint8_t lip_confidence_level;
	uint16_t  lip_altitude;
	uint8_t  lip_altitudeunc;
	uint16_t lip_inradius;
	uint16_t lip_outradius;
	uint8_t lip_startangle;
	uint8_t lip_stopangle;
	uint8_t lip_velocity_type;
	uint8_t lip_pdu_type_adddata;
	uint8_t lip_pdu_adddata;

	switch  (lip_pdu_type) {

		case 0: /* SHORT LOCATION REPORT PDU */

			m=2; lip_time_elapsed=bits_to_uint(bits+n, m); n=n+m;
			m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
			m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
			m=3; lip_pos_error=bits_to_uint(bits+n, m); n=n+m;
			m=7; lip_horiz_velocity=bits_to_uint(bits+n, m); n=n+m;
			m=4; lip_dir_travel=bits_to_uint(bits+n, m); n=n+m;
			m=1; lip_type_adddata=bits_to_uint(bits+n, m); n=n+m;

			if (lip_lattitude&(1<<23)) {
				lattitude=(((1<<24)-lip_lattitude)*180.0)/(1.0*(1<<24)); latdir='S';
			} else
			{
				lattitude=(lip_lattitude*180.0)/(1.0*(1<<24)); latdir='N';
			}

			/* note: this should be 1<<25 for the calculations (according to the documentation), but for some reason 1<<24 is correct, maybe i skipped a bit somewhere? */
			if (lip_longtitude&(1<<24)) {
				longtitude=(((1<<25)-lip_longtitude)*180.0)/(1.0*(1<<24)); londir='W';
			} else
			{
				longtitude=(lip_longtitude*180.0)/(1.0*(1<<24)); londir='E';
			}
			snprintf(out,outlen,"SHORT LOCATION REPORT: lat:%.6f%c lon:%.6f%c error%s speed:%4.1fkm/h heading:%s",lattitude,latdir,longtitude,londir,lip_position_errors[lip_pos_error],get_horiz_velocity(lip_horiz_velocity),get_lip_dirtravel_type(lip_dir_travel));
			break;

		case 1: /* LONG-type pdus */
			m=4; lip_pdu_type_extension=bits_to_uint(bits+n, m); n=n+m;
			switch(lip_pdu_type_extension)
			{
				case TETRA_LIP_LONGPDUEXT_LONGLOCREP:
					// strona 45
					/* Time data */
					m=2; lip_pdu_timedata=bits_to_uint(bits+n, m); n=n+m;
					switch(lip_pdu_timedata) {
						case 0:
							/* no data */
							sprintf(tmpstr,"no_timedata ");
							break;
						case 1:
							m=2; lip_pdu_timedatae=bits_to_uint(bits+n, m); n=n+m;
							sprintf(tmpstr,"time_elaspsed:%i ",lip_pdu_timedatae);
							break;
						case 2:
							m=22; lip_pdu_timedatae=bits_to_uint(bits+n, m); n=n+m;
							sprintf(tmpstr,"time_of_position:%i ",lip_pdu_timedatae);
							break;
						case 3:
						default:
							sprintf(tmpstr,"timedata_reserved ");
							break;

					}
					/* Location data */
					m=4; lip_pdu_locdata=bits_to_uint(bits+n, m); n=n+m;
					switch(lip_pdu_locdata) {
						case TETRA_LIP_SHAPE_NOSHAPE:
							sprintf(tmpstr2,"no_shape_data ");
							break;
						case TETRA_LIP_SHAPE_POINT:
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							sprintf(tmpstr2,"point lat:%s lon:%s ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude));
							break;
						case TETRA_LIP_SHAPE_CIRCLE:
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							m=m+6; lip_horizuncertainity=bits_to_uint(bits+n, m); n=n+m; /* Horizontal position uncertainty */

							sprintf(tmpstr2,"circle lat:%s lon:%s uncertainity:%i ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude), lip_horizuncertainity);

							break;
						case TETRA_LIP_SHAPE_ELLIPSE:
							/* Location ellipse */
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							m=m+6; lip_halfmajoraxis=bits_to_uint(bits+n, m); n=n+m; /* Half of the major axis */
							m=m+6; lip_halfminoraxis=bits_to_uint(bits+n, m); n=n+m; /* Half of the minor axis */
							m=m+8; lip_angle=bits_to_uint(bits+n, m); n=n+m; /* Angle */
							m=m+3; lip_confidence_level=bits_to_uint(bits+n, m); n=n+m; /* confidence level */
							sprintf(tmpstr2,"ellipse lat:%s lon:%s ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude));


							break;
						case TETRA_LIP_SHAPE_POINT_ALTITUDE:
							/* Location point with altitude */
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							m=m+12; lip_altitude=bits_to_uint(bits+n, m); n=n+m; /* Altitude */
							sprintf(tmpstr2,"point+altitude lat:%s lon:%s altitude:%i ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude),lip_altitude);

							break;
						case TETRA_LIP_SHAPE_CIRCLE_ALTITUDE:
							/* Location circle with altitude */
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							m=m+6; lip_horizuncertainity=bits_to_uint(bits+n, m); n=n+m; /* Horizontal position uncertainty */
							m=m+12; lip_altitude=bits_to_uint(bits+n, m); n=n+m; /* Altitude */

							sprintf(tmpstr2,"circle+altitude lat:%s lon:%s uncertainity:%i altitude:%i ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude), lip_horizuncertainity,lip_altitude);


							break;
						case TETRA_LIP_SHAPE_ELLIPSE_ALTITUDE:
							/* Location ellipse with altitude */
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							m=m+6; lip_halfmajoraxis=bits_to_uint(bits+n, m); n=n+m; /* Half of the major axis */
							m=m+6; lip_halfminoraxis=bits_to_uint(bits+n, m); n=n+m; /* Half of the minor axis */
							m=m+8; lip_angle=bits_to_uint(bits+n, m); n=n+m; /* Angle */
							m=m+12; lip_altitude=bits_to_uint(bits+n, m); n=n+m; /* Altitude */
							m=m+3; lip_confidence_level=bits_to_uint(bits+n, m); n=n+m; /* confidence level */
							sprintf(tmpstr2,"ellipse+altitude lat:%s lon:%s ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude));

							break;
						case TETRA_LIP_SHAPE_CIRCLE_ALTITUDE_ALTUNC:
							/* Location circle with altitude and altitude uncertainity*/
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							m=m+6; lip_horizuncertainity=bits_to_uint(bits+n, m); n=n+m; /* Horizontal position uncertainty */
							m=m+12; lip_altitude=bits_to_uint(bits+n, m); n=n+m; /* Altitude */
							m=m+3; lip_altitudeunc=bits_to_uint(bits+n, m); n=n+m; /* Altitude uncertainty */

							sprintf(tmpstr2,"circle+altitude+uncertainity lat:%s lon:%s uncertainity:%i altitude:%i alt_uncertainity:%i ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude), lip_horizuncertainity,lip_altitude,lip_altitudeunc);


							break;
						case TETRA_LIP_SHAPE_ARC:
							/* Location arc */
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							m=m+16; lip_inradius=bits_to_uint(bits+n, m); n=n+m; /* Inner radius */
							m=m+16; lip_outradius=bits_to_uint(bits+n, m); n=n+m; /* Outer radius */
							m=m+8; lip_startangle=bits_to_uint(bits+n, m); n=n+m; /* Start Angle */
							m=m+8; lip_stopangle=bits_to_uint(bits+n, m); n=n+m; /* Stop Angle */
							m=m+3; lip_confidence_level=bits_to_uint(bits+n, m); n=n+m; /* confidence level */
							sprintf(tmpstr2,"arc lat:%s lon:%s inner_radius:%i outer_radius:%i start_angle:%i stop_angle:%ii confidence_level:%i ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude),lip_inradius,lip_outradius, lip_startangle, lip_stopangle,lip_confidence_level);

							break;
						case TETRA_LIP_SHAPE_POINT_POSERR:
							/* location point and position error */
							m=25; lip_longtitude=bits_to_uint(bits+n, m); n=n+m;
							m=24; lip_lattitude=bits_to_uint(bits+n, m); n=n+m;
							m=3; lip_pos_error=bits_to_uint(bits+n, m); n=n+m;
							sprintf(tmpstr2,"point+error lat:%s lon:%s error:%s ",parse_lip_lattitude(lip_lattitude),parse_lip_longtitude(lip_longtitude),lip_position_errors[lip_pos_error]);

							break;
						case TETRA_LIP_SHAPE_RESERVED11:
						case TETRA_LIP_SHAPE_RESERVED12:
						case TETRA_LIP_SHAPE_RESERVED13:
						case TETRA_LIP_SHAPE_RESERVED15:
						case TETRA_LIP_SHAPE_LOCSHAPE_EXTENSION:
						default:
							sprintf(tmpstr2,"Location Data [%i]",lip_pdu_locdata);
							break;
					}
					strcat(tmpstr,tmpstr2);
					/* Velocity data */
					m=3; lip_velocity_type=bits_to_uint(bits+n, m); n=n+m;

					/* placeholder for now, lengths from Table 6.123: Velocity data information element contents */
					switch(lip_velocity_type)
					{
						case TETRA_LIP_VELOCITY_NONE:
							break;
						case TETRA_LIP_VELOCITY_HORIZ_VELOCITY: 
							m=m+7;
							break;
						case TETRA_LIP_VELOCITY_HORIZ_VELOCITY_UNC: 
							m=m+10;
							break;
						case TETRA_LIP_VELOCITY_HORIZ_VELOCITY_VERT: 
							m=m+15;
							break;
						case TETRA_LIP_VELOCITY_HORIZ_VELOCITY_VERT_UNC: 
							m=m+21;
							break;
						case TETRA_LIP_VELOCITY_HORIZ_VELOCITY_DIRTRAVEL: 
							m=m+15;
							break;
						case TETRA_LIP_VELOCITY_HORIZ_VELOCITY_DIRTRAVEL_UNC: 
							m=m+21;
							break;
						case TETRA_LIP_VELOCITY_HORIZ_VELOCITY_VERT_DIRTRAVEL_UNC: 
							m=m+32;
							break;
					}
					sprintf(tmpstr2,"velocity_type:%i ",lip_velocity_type); 
					strcat(tmpstr,tmpstr2);


					/* Acknowledgement request */
					m=1; lip_pdu_ackrequest=bits_to_uint(bits+n, m); n=n+m;

					/* Type of additional data 0:Reason for sending 1:User defined data */
					m=1; lip_pdu_type_adddata=bits_to_uint(bits+n, m); n=n+m;
					/* Reason for sending */
					m=8; lip_pdu_adddata=bits_to_uint(bits+n, m); n=n+m;
					sprintf(tmpstr2,"ack:%i type_add_data:%i add_data:%i ",lip_pdu_ackrequest,lip_pdu_type_adddata,lip_pdu_adddata); 
					strcat(tmpstr,tmpstr2);



					snprintf(out,outlen,"LONG LOCATION REPORT PDU [%s]",tmpstr);

					break;

				case TETRA_LIP_LONGPDUEXT_BASICLOCREPREQ:
					m=1; lip_pdu_type_extreqresp=bits_to_uint(bits+n, m); n=n+m;
					if (lip_pdu_type_extreqresp)
					{
						/* BASIC LOCATION PARAMETERS RESPONSE PDU */
						m=8; lip_pdu_resultcode=bits_to_uint(bits+n, m); n=n+m;
						m=8; lip_pdu_locrepflags=bits_to_uint(bits+n, m); n=n+m;
						m=7; lip_pdu_minrepinterval=bits_to_uint(bits+n, m); n=n+m;
						m=2; lip_pdu_reporttype=bits_to_uint(bits+n, m); n=n+m;
						snprintf(out,outlen,"BASIC LOCATION PARAMETERS RESPONSE PDU result:%i flags:%2.2x %s min_report_interval:%is report_type:%i %s",lip_pdu_resultcode,lip_pdu_locrepflags,decode_liprepenflags(lip_pdu_locrepflags),lip_pdu_minrepinterval*10,lip_pdu_reporttype,decode_lipreptype(lip_pdu_reporttype));


					} else {
						/* BASIC LOCATION PARAMETERS REQUEST PDU */
						m=1; lip_pdu_ackrequest=bits_to_uint(bits+n, m); n=n+m;
						m=7; lip_pdu_minrepinterval=bits_to_uint(bits+n, m); n=n+m;
						m=2; lip_pdu_reporttype=bits_to_uint(bits+n, m); n=n+m;
						snprintf(out,outlen,"BASIC LOCATION PARAMETERS REQUEST PDU ack:%i min_report_interval:%is report_type:%i %s",lip_pdu_ackrequest,lip_pdu_minrepinterval*10,lip_pdu_reporttype,decode_lipreptype(lip_pdu_reporttype));

					}

					break;
				case TETRA_LIP_LONGPDUEXT_LOCREPORENABLE:
					m=1; lip_pdu_type_extreqresp=bits_to_uint(bits+n, m); n=n+m;
					if (lip_pdu_type_extreqresp) {
						/* LOCATION REPORTING ENABLE/DISABLE RESPONSE PDU */
						m=8; lip_pdu_locrepflags=bits_to_uint(bits+n, m); n=n+m;

						snprintf(out,outlen,"LOCATION REPORTING ENA/DIS RESPONSE flags:%2.2x %s",lip_pdu_locrepflags,decode_liprepenflags(lip_pdu_locrepflags));
					} else {
						/* LOCATION REPORTING ENABLE/DISABLE REQUEST PDU */
						m=1; lip_pdu_ackrequest=bits_to_uint(bits+n, m); n=n+m;
						m=8; lip_pdu_locrepflags=bits_to_uint(bits+n, m); n=n+m;

						snprintf(out,outlen,"LOCATION REPORTING ENA/DIS REQUEST ack:%i flags:%2.2x %s",lip_pdu_ackrequest,lip_pdu_locrepflags,decode_liprepenflags(lip_pdu_locrepflags));

					}

					break;
				default:
					snprintf(out,outlen,"LONG PDU TYPE %i %s (not implemented yet)",lip_pdu_type_extension,lip_longpdu_extensions[lip_pdu_type_extension]);
					break;
			}
			break;
		default:

			snprintf(out,outlen,"UNKNOWN PDU TYPE %i",lip_pdu_type);
			break;

	}
	return(0);
}

/* decode Location System */
int decode_locsystem(char *out, int outlen,uint8_t *bits,int datalen)
{
	uint32_t loc_longtitude; 
	uint32_t loc_lattitude; 
	float lattitude,longtitude;
	char latdir,londir;
	int n=0, m;
	uint8_t locsystem_coding_scheme;
	char c;
	char buf[16];
	int dumpascii=0; /* try to dump ascii, or just do all hex? */
	int dump=1; /* should we dump, or maybe do something smarter? */
	uint8_t rtcm_msgtype;
	uint16_t rtcm_stationid;
	uint8_t rtcm_parity;
	uint16_t rtcm_mod_zcount;
	uint8_t rtcm_seqno;
	uint8_t  rtcm_nowords;
	uint8_t  rtcm_sthealth;
	uint8_t rtcm_parity2;
	buf[0]=0;
/*	datalen=datalen-16; n=n+16;  skip the sds-tl 2-byte header, not needed now */
	m=8; locsystem_coding_scheme=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
	switch (locsystem_coding_scheme) {
		case 0: /* NMEA */
			snprintf(out,outlen,"NMEA: "); outlen=outlen-7;
			dumpascii=1;
			break;
		case 1: /* RTCM DC-104 */
			m=6; rtcm_msgtype=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
			m=10; rtcm_stationid=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
			m=6; rtcm_parity=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
			m=13; rtcm_mod_zcount=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
			m=3; rtcm_seqno=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
			m=5; rtcm_nowords=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
			m=3; rtcm_sthealth=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
			m=6; rtcm_parity2=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;

			snprintf(out,outlen,"RTCM SC-104 *THIS IS BOGUS* word1: msgtype:%i stationid:%i parity:%i word2: mod_zcount:%fs seqno:%i no_datawords:%i stationhealth:%i parity2:%i ",rtcm_msgtype,rtcm_stationid,rtcm_parity, rtcm_mod_zcount*0.6,rtcm_seqno, rtcm_nowords, rtcm_sthealth,rtcm_parity2); outlen=outlen-30; /* the outlen stuff is bogus, lets be men and assume the strings are big enough :) */
			//snprintf(out,outlen,"RTCM SC-104 (not implemented)  msgtype:%i stationid:%i parity:%i ",rtcm_msgtype,rtcm_stationid,rtcm_parity); outlen=outlen-30;

			break;
/* i've seen a type 0x80 proprietary message, but this something different from simple location system 0x80 */ 
		default: /* reserved */
			snprintf(out,outlen,"proprietary coding scheme 0x%2.2x: ",locsystem_coding_scheme); outlen=outlen-33;

			break;
	}
	if (dump)
	{       while ((datalen>0)&&(outlen>0)) {
							m=8; c=bits_to_uint(bits+n, m); n=n+m;
							if ((dumpascii)&&isprint(c)) {
								sprintf(buf,"%c",c);
								outlen--;
							} else {
								sprintf(buf,"\\x%2.2x",(unsigned char) c);
								outlen=outlen-3;
							}
							datalen=datalen-m;
							strcat(out,buf);
						}
	}

	return(0);
}

/* decode Simple Location System */
int decode_simplelocsystem(char *out, int outlen,uint8_t *bits,int datalen)
{
	/* TODO: this should be one common function for decode_simplelocsystem and decode_locsystem,
	 * because in theory they are the same */
	int n=0, m;
	uint8_t locsystem_coding_scheme;
	int dumpascii=0; /* try to dump ascii, or just do all hex? */
	int dump=1; /* should we dump, or maybe do something smarter? */
	uint32_t loc_longtitude; 
	uint32_t loc_lattitude; 
	float lattitude,longtitude;
	char c;
	char latdir,londir;
	uint16_t unk_flags;
	char buf[1024];
	int is_invalid=0;
	m=8; locsystem_coding_scheme=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
	switch (locsystem_coding_scheme) {
		case 0: /* NMEA */
			snprintf(out,outlen,"NMEA: "); outlen=outlen-7;
			dumpascii=1;
			break;
		case 1: /* RTCM DC-104 */
			snprintf(out,outlen,"RTCM SC-104 (not implemented)"); outlen=outlen-30;
			break;

		case 0x80: 
			/* some proprietary system seen in the wild in Spain, Itlay and France
			 * some speculate it's either from DAMM or SEPURA  */
			m=12; unk_flags=bits_to_uint(bits+n,m); n=n+m; /* skip some unknown part */
			m=24; loc_lattitude=bits_to_uint(bits+n,m); n=n+m;
			m=24; loc_longtitude=bits_to_uint(bits+n,m); n=n+m;
			/* i didn't figure out yet what the other bits mean yet */
			if (loc_lattitude&(1<<23)) {
				lattitude=(((1<<24)-loc_lattitude)*180.0)/(1.0*(1<<24)); latdir='S';
			} else
			{
				lattitude=(loc_lattitude*180.0)/(1.0*(1<<24)); latdir='N';
			}

			if (loc_longtitude&(1<<23)) {
				longtitude=(((1<<24)-loc_longtitude)*360.0)/(1.0*(1<<24)); londir='W';
			} else
			{
				longtitude=(loc_longtitude*360.0)/(1.0*(1<<24)); londir='E';
			}
			if ((lattitude==90)&&(longtitude==0)) is_invalid=1;
			if ((lattitude==0)&&(longtitude==0)) is_invalid=1;
			snprintf(out,outlen,"PROPRIETARY_0x80 %sunknown_flags:0x%4.4x lat:%.6f%c lon:%.6f%c",is_invalid?"INVALID_POSITION ":"",unk_flags,lattitude,latdir,longtitude,londir);
			dump=0;
			break;
		default:
			snprintf(out,outlen,"proprietary coding scheme 0x%2.2x: ",locsystem_coding_scheme); outlen=outlen-33;

	}
	if (dump)
	{       while ((datalen>0)&&(outlen>0)) {
							m=8; c=bits_to_uint(bits+n, m); n=n+m;
							if ((dumpascii)&&isprint(c)) {
								sprintf(buf,"%c",c);
								outlen--;
							} else {
								sprintf(buf,"\\x%2.2x",(unsigned char) c);
								outlen=outlen-3;
							}
							datalen=datalen-m;
							strcat(out,buf);
						}
	}

}

