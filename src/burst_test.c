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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <osmocore/utils.h>

#include "tetra_common.h"
#include <phy/tetra_burst.h>
#include <phy/tetra_burst_sync.h>
#include "tetra_gsmtap.h"

int main(int argc, char **argv)
{
	int fd;
	struct tetra_rx_state *trs;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file_with_1_byte_per_bit>\n", argv[0]);
		exit(1);
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(2);
	}

	tetra_gsmtap_init("localhost", 0);

	trs = calloc(1, sizeof(*trs));

	while (1) {
		uint8_t buf[64];
		int len;

		len = read(fd, buf, sizeof(buf));
		if (len < 0) {
			perror("read");
			exit(1);
		} else if (len == 0) {
			printf("EOF");
			break;
		}
		tetra_burst_sync_in(trs, buf, len);
	}

	free(trs);

	exit(0);
}
