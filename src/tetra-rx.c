/* Test program for tetra burst synchronizer */

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
 *
 * Changes from the original version (for osmo-tetra-sq5bpf):
 * - merged float_to_bits code here
 * - crude pseudo-afc implementation
 * - set some variables on startup that will later be used to communicate with telive 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <osmocom/core/utils.h>
#include <osmocom/core/talloc.h>

#include "tetra_common.h"
#include <phy/tetra_burst.h>
#include <phy/tetra_burst_sync.h>
#include "tetra_gsmtap.h"

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void *tetra_tall_ctx;

/* the following was taken from float_to_bits.c (C) 2011 by Harald Welte <laforge@gnumonks.org> --sq5bpf */
static int process_sym_fl(float fl)
{
	int ret;

	/* very simplistic scheme */
	if (fl > 2)
		ret = 3;
	else if (fl > 0)
		ret = 1;
	else if (fl < -2)
		ret = -3;
	else
		ret = -1;

	return ret;
}

static void sym_int2bits(int sym, uint8_t *ret0,uint8_t *ret1)
{
	switch (sym) {
		case -3:
			*ret0 = 1;
			*ret1 = 1;
			break;
		case 1:
			*ret0 = 0;
			*ret1 = 0;
			break;
			//case -1:
		case 3:
			*ret0 = 0;
			*ret1 = 1;
			break;
			//case 3:
		case -1:
			*ret0 = 1;
			*ret1 = 0;
			break;
	}
}



void show_help(char *prog)
{
	fprintf(stderr, "Usage: %s <-i> <-h> <-f filter_constant> <-a> <file_with_1_byte_per_bit or file_with_floats>\n", prog);
	fprintf(stderr, "-h - show help\n-i accept float values (internal float_to_bits)\n\n-a turn on pseudo-afc (works only with -i)\n-f pseudo-afc averaging filter constant (default 0.0001)\n");

}

int main(int argc, char **argv)
{
	int fd;
	struct tetra_rx_state *trs;
	struct tetra_mac_state *tms;
	char *tmphost;
	int opt;
	int accept_float=0;
	int do_afc=0;
	float filter=0;
	float filter_val=0.0001;
	float filter_goal=0;
	int ccounter=0;
	char tmpstr2[64];
	while ((opt = getopt(argc, argv, "ihf:F:a")) != -1) {

		switch (opt) {
			case 'i':
				accept_float = 1;
				break;
			case 'a':
				do_afc=1;
				break;
			case 'f':
				filter_val=atof(optarg);
				break;
			case 'F':
				filter_goal=atof(optarg);
				break;
			case 'h':
				show_help(argv[0]);
				exit(0);
			default:
				fprintf(stderr,"Bad option '%c'\n",opt);
				exit(2);
		}
	}


	if (argc <= optind) {
		show_help(argv[0]);
		exit(2);
	}


	fd = open(argv[optind], O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(2);
	}
	/* sq5bpf */
	memset((void *)&tetra_hack_db,0,sizeof(tetra_hack_db));
	tetra_hack_live_idx=0;
	tetra_hack_live_lastseen=0;
	tetra_hack_live_socket=0;

	if (getenv("TETRA_HACK_PORT")) {
		tetra_hack_rxid=atoi(getenv("TETRA_HACK_RXID"));
	} else
	{
		tetra_hack_rxid=0;
	}
	if (getenv("TETRA_HACK_PORT")) {
		tetra_hack_live_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		tetra_hack_socklen=sizeof(struct sockaddr_in);
		tetra_hack_live_sockaddr.sin_family = AF_INET;
		tetra_hack_live_sockaddr.sin_port = htons(atoi(getenv("TETRA_HACK_PORT")));
		tmphost=getenv("TETRA_HACK_IP");
		if (!tmphost) {
			tmphost="127.0.0.1";
		}
		//inet_aton("127.0.0.1", & tetra_hack_live_sockaddr.sin_addr);
		inet_aton(tmphost, & tetra_hack_live_sockaddr.sin_addr);
		if (tetra_hack_live_socket<1) tetra_hack_live_socket=0;
	}

	tetra_gsmtap_init("localhost", 0);

	tms = talloc_zero(tetra_tall_ctx, struct tetra_mac_state);
	tetra_mac_state_init(tms);

	trs = talloc_zero(tetra_tall_ctx, struct tetra_rx_state);
	trs->burst_cb_priv = tms;
#define BUFLEN 64
#define MAXVAL 5.0

	while (1) {
		uint8_t buf[BUFLEN];
		int len;
		int i;
		if (accept_float) {
			int rc;
			int rc2;
			float fl[BUFLEN];
			rc = read(fd, &fl, sizeof(fl));
			if (rc < 0) {
				perror("read");
				exit(1);
			} else if (rc == 0)
				break;
			rc2=rc/sizeof(float);
			for(i=0;i<rc2;i++) {	
				
				if ((fl[i]>-MAXVAL)&&(fl[i]<MAXVAL)) 
					filter=filter*(1.0-filter_val)+(fl[i]-filter_goal)*filter_val;
				if (do_afc) {
					rc = process_sym_fl(fl[i]-filter);
				} else {
					rc = process_sym_fl(fl[i]);
				}
				sym_int2bits(rc, &buf[2*i],&buf[2*i+1]);

			}		
			len=rc2*2;

		}
		else
		{
			len = read(fd, buf, sizeof(buf));
			if (len < 0) {
				perror("read");
				exit(1);
			} else if (len == 0) {
				printf("EOF");
				break;
			}
		}
		tetra_burst_sync_in(trs, buf, len);

		if (accept_float) {	
			ccounter++;
			if (ccounter>50)
			{
				fprintf(stderr,"\n### AFC: %f\n",filter);

				sprintf(tmpstr2,"TETMON_begin FUNC:AFCVAL AFC:%i RX:%i TETMON_end",(int) (filter*100.0),tetra_hack_rxid);
				sendto(tetra_hack_live_socket, (char *)&tmpstr2, strlen((char *)&tmpstr2)+1, 0, (struct sockaddr *)&tetra_hack_live_sockaddr, tetra_hack_socklen);

				ccounter=0;
			} 
		}

	}

	talloc_free(trs);
	talloc_free(tms);

	exit(0);
}
