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




int main(int argc,char **argv) {
	unsigned char buf1[8192];
	unsigned char buf[8192];
	int len,len1;

	struct tetra_mac_state tms;
	struct msgb msg;

	//len1=char_to_ubits("0000000010011110100110001011110110010001111011001000100110",(char *)&buf1); //dummy header so the osmo-tetra functions work
	len1=char_to_ubits("0000000010011110000000000000000000000001111011001000100110",(char *)&buf1); //dummy header so the osmo-tetra functions work
	len=char_to_ubits(argv[1],(char *)&buf);
	msg.l1h=(unsigned char *)&buf1;
	msg.l3h=(unsigned char *)&buf;

	parse_d_sds_data(&tms,&msg,len);

}
