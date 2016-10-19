/* testsds.c - a quick hack to test the sds parsing functions. 
 * expects a copy of the sds binary dump from telive.log as an argument
 *
 * (C) 2015 by Jacek Lipkowski <sq5bpf@lipkowski.org>
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

#include <osmocom/core/bits.h>


#include "tetra_common.h"
#include <lower_mac/crc_simple.h>
#include <lower_mac/tetra_conv_enc.h>
#include <lower_mac/tetra_interleave.h>
#include <lower_mac/tetra_scramb.h>
#include <lower_mac/tetra_rm3014.h>
#include <lower_mac/viterbi.h>
#include <phy/tetra_burst.h>
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

extern int rx_tl_sdu(struct tetra_mac_state *tms, struct msgb *msg, unsigned int len);

extern int tetra_hack_all_sds_as_text;

int char_to_ubits(char *c,unsigned char *out)
{
	int len=0;
	while (*c) {
		if (*c=='1') {  *out=1; } else { *out=0; }

		c++;
		len++;
		out++;

	}
	return(len);
}

show_ascii_strings(unsigned char *buf,int len)
{
	int i,len2;
	unsigned char *buf2;
	unsigned char c;
	int waschar;

	printf("Ascii/binary dump:\n");

	for (i=0;i<8;i++)
	{
		buf2=buf+i;
		len2=len-i;
		printf("Bit shift %i:  [",i);
		waschar=0;
		while (len2>7)
		{
			c=bits_to_uint(buf2, 8); 
			len2=len2-8; 
			buf2=buf2+8;

			if ((isprint(c))&&(c!='\n')&&(c!='\r')) 
				//		if (1==0)
			{ 
				if (!waschar) printf(" ");
				printf("%c",c); 
				waschar=1;
			} 
			else 
			{ 
				if (waschar) printf(" ");
				printf("\\x%2.2X ",c); 
				waschar=0;
			}
		}
		printf("]\tbits left: %i\n",len2);

	}
}

int main(int argc,char **argv) {
	unsigned char buf1[8192];
	unsigned char buf[8192];
	int len,len1;

	struct tetra_mac_state tms;
	struct msgb msg;

	len1=char_to_ubits("0000000010011110000000000000000000000001111011001000100110",(char *)&buf1); //dummy header so the osmo-tetra functions work
	len=char_to_ubits(argv[1],(char *)&buf);
	msg.l1h=(unsigned char *)&buf1;
	msg.l3h=(unsigned char *)&buf;

	tetra_hack_all_sds_as_text=1;

	show_ascii_strings((unsigned char *)&buf,len);

	printf("\n\n");

	parse_d_sds_data(&tms,&msg,len);


}
