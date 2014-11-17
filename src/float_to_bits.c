/* Convert floating point symbols in the range +3/-3 to hard bits,
 * in the 1-bit-per-byte format */

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


#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

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

static void sym_int2bits(int sym, uint8_t *ret)
{
	switch (sym) {
	case -3:
		ret[0] = 1;
		ret[1] = 1;
		break;
	case 1:
		ret[0] = 0;
		ret[1] = 0;
		break;
	//case -1:
	case 3:
		ret[0] = 0;
		ret[1] = 1;
		break;
	//case 3:
	case -1:
		ret[0] = 1;
		ret[1] = 0;
		break;
	}
}

// size of IO buffers (number of elements)
#define BUF_SIZE 1024

int main(int argc, char **argv)
{
	int fd, fd_out, opt;

	int opt_verbose = 0;

	while ((opt = getopt(argc, argv, "v")) != -1) {
		switch (opt) {
		case 'v':
			opt_verbose = 1;
			break;
		default:
			exit(2);
		}
	}

	if (argc <= optind+1) {
		fprintf(stderr, "Usage: %s [-v] <infile> <outfile>\n", argv[0]);
		exit(2);
	}

	fd = open(argv[optind], O_RDONLY);
	if (fd < 0) {
		perror("open infile");
		exit(1);
	}
	fd_out = creat(argv[optind+1], 0660);
	if (fd_out < 0) {
		perror("open outfile");
		exit(1);
	}
	while (1) {
		int rc;
		float fl[BUF_SIZE];
		uint8_t bits[2*BUF_SIZE];
		rc = read(fd, fl, sizeof(*fl) * BUF_SIZE);
		if (rc < 0) {
			perror("read");
			exit(1);
		} else if (rc == 0) {
			break;
		}
		rc /= sizeof(*fl);
		int i;
		for (i = 0; i < rc; ++i) {
			int sym = process_sym_fl(fl[i]);
			sym_int2bits(sym, bits + i*2);
			//printf("%2d %1u %1u  %f\n", rc, bits[0], bits[1], fl);
			if (opt_verbose) {
				printf("%1u%1u", bits[2*i + 0], bits[2*i + 1]);
			}
		}

		rc = write(fd_out, bits, rc * 2);
		if (rc < 0) {
			perror("write");
			exit(1);
		} else if (rc == 0)
			break;
	}
	exit(0);
}
