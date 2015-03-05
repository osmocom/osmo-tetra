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



#include <stdint.h>
#include "tetra_sds.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <osmocom/core/utils.h>

#include "tetra_common.h"
#include "tetra_mac_pdu.h"
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
		longtitude=(((1<<24)-lip_longtitude)*180.0)/(1.0*(1<<24)); londir='W';
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
				longtitude=(((1<<24)-lip_longtitude)*180.0)/(1.0*(1<<24)); londir='W';
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
							sprintf(tmpstr2,"Location Data [%i]");
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
	datalen=datalen-16; n=n+16; /* skip the sds-tl 2-byte header */
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
	m=8; locsystem_coding_scheme=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
	switch (locsystem_coding_scheme) {
		case 0: /* NMEA */
			snprintf(out,outlen,"NMEA: "); outlen=outlen-7;
			dumpascii=1;
			break;
		case 1: /* RTCM DC-104 */
			snprintf(out,outlen,"RTCM SC-104 (not implemented)"); outlen=outlen-30;
			break;

		case 0x80: /* some proprietary system seen in the wild in Spain and other countries */
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
			snprintf(out,outlen,"PROPRIETARY_0x80 unknown_flags:0x%4.4x lat:%.6f%c lon:%.6f%c",unk_flags,lattitude,latdir,longtitude,londir);
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

