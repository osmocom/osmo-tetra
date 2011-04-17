/* (C) 2011 by Holger Hans Peter Freyther
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

#include <osmocom/core/bits.h>

#include "tetra_common.h"
#include <lower_mac/crc_simple.h>

int main(int argc, char **argv)
{
	uint8_t input1[] = { 0x01 };
	uint16_t crc;

	crc = crc16_itut_bytes(0x0, input1, 8);
	printf("The CRC is now: %u/0x%x\n", crc, crc);

	crc = crc16_itut_bits(0x0, input1, 1);
	printf("The CRC is now: %d/0x%x\n", crc, crc);

	crc = crc16_itut_bytes(0xFFFF, input1, 8);
	printf("The CRC is now: %u/0x%x\n", crc, crc);

	crc = crc16_itut_bits(0xFFFF, input1, 1);
	printf("The CRC is now: %d/0x%x\n", crc, crc);

	/* actual CRC-CCIT usage */
	uint8_t input2[] = {
			0, 0, 0, 1, 0, 0, 0, 0,
			1, 0, 1, 1, 0, 0, 0, 0,
			1, 0, 1, 1, 1, 1, 1, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 1, 1,
			0, 0, 0, 0, 0, 1, 1, 1,
			1, 1, 0, 1, 0, 0, 1, 1,
			0, 0, 1, 1,

			/* checksum */
			1, 1, 0, 1, 1, 1, 1, 0,
			1, 1, 1, 1, 0, 0, 0, 1,
			/* tail bits */
			0, 0, 0, 0 };


	crc = ~crc16_itut_bits(0xffff, input2, 60);
	printf("The CRC is now: %d/0x%x\n", crc, crc);
	
	/* swap the bytes */
	crc = (crc << 8) | (crc >> 8);
	osmo_pbit2ubit(&input2[60], (uint8_t *)&crc, 16);

	crc = crc16_itut_bits(0xFFFF, input2, 60 + 16);
	printf("The CRC is now: %d/0x%x\n", crc, crc);
	if (crc == 0x1d0f)
		printf("Decoded successfully.\n");
	else
		printf("Failed to decode.\n");

	return 0;
}
