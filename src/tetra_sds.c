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



/* decode Location information protocol */
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

			snprintf(out,outlen,"LONG PDU TYPE ext %i (not implemented yet)",lip_pdu_type_extension);
		default:

			snprintf(out,outlen,"UNKNOWN PDU TYPE %i",lip_pdu_type);
			break;

	}
	return(0);
}

/* decode Location System */
int decode_locsystem(char *out, int outlen,uint8_t *bits,int datalen)
{
	int n=0, m;
	uint8_t locsystem_coding_scheme;
	char c;
	char buf[16];
	int dumpascii=0; /* try to dump ascii, or just do all hex? */
	int dump=1; /* should we dump, or maybe do something smarter? */
	buf[0]=0;
	datalen=datalen-16; n=n+16; /* skip the sds-tl 2-byte header */
	m=8; locsystem_coding_scheme=bits_to_uint(bits+n, m); n=n+m; datalen=datalen-m;
	switch (locsystem_coding_scheme) {
		case 0: /* NMEA */
			snprintf(out,outlen,"NMEA: "); outlen=outlen-7;
			dumpascii=1;
			break;
		case 1: /* RTCM DC-104 */
			snprintf(out,outlen,"RTCM SC-104 (not implemented)"); outlen=outlen-30;
			break;
		default: /* reserved */
			snprintf(out,outlen,"proprietary coding scheme 0x2.2x: ",locsystem_coding_scheme); outlen=outlen-33;

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

		case 0x80: /* some proprietary system seen in the wild in Spain */
			loc_lattitude=bits_to_uint(bits+20, 24);
			loc_longtitude=bits_to_uint(bits+44, 24);
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
			snprintf(out,outlen,"PROPRIETARY_0x80 lat:%.6f%c lon:%.6f%c",lattitude,latdir,longtitude,londir);
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

