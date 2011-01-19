/* (C) 2011 by Sylvain Munaut <tnt@246tNt.com>
 *
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
#include <string.h>

#include <lower_mac/viterbi_cch.h>


#define CONV_CCH_N		4
#define CONV_CCH_K		5
#define CONV_CCH_N_STATES	(1<<(CONV_CCH_K-1))

#define MAX_AE			0x00ffffff


static const uint8_t conv_cch_next_output[CONV_CCH_N_STATES][2] = {
	{  0, 15 }, { 11,  4 }, {  6,  9 }, { 13,  2 },
	{  5, 10 }, { 14,  1 }, {  3, 12 }, {  8,  7 },
	{ 15,  0 }, {  4, 11 }, {  9,  6 }, {  2, 13 },
	{ 10,  5 }, {  1, 14 }, { 12,  3 }, {  7,  8 },
};

static const uint8_t conv_cch_next_state[CONV_CCH_N_STATES][2] = {
	{  0,  1 }, {  2,  3 }, {  4,  5 }, {  6,  7 },
	{  8,  9 }, { 10, 11 }, { 12, 13 }, { 14, 15 },
	{  0,  1 }, {  2,  3 }, {  4,  5 }, {  6,  7 },
	{  8,  9 }, { 10, 11 }, { 12, 13 }, { 14, 15 },
};


int conv_cch_encode(uint8_t *input, uint8_t *output, int n)
{
	uint8_t state;
	int i;

	state = 0;

	for (i=0; i<n; i++) {
		int bit = input[i];
		uint8_t out = conv_cch_next_output[state][bit];
		state = conv_cch_next_state[state][bit];

		output[(i<<2)  ] = (out >> 3) & 1;
		output[(i<<2)+1] = (out >> 2) & 1;
		output[(i<<2)+2] = (out >> 1) & 1;
		output[(i<<2)+3] =  out       & 1;
	}

	return 0;
}

int conv_cch_decode(int8_t *input, uint8_t *output, int n)
{
	int i, s, b;
	unsigned int ae[CONV_CCH_N_STATES];
	unsigned int ae_next[CONV_CCH_N_STATES];
	int8_t in_sym[CONV_CCH_N];
	int8_t ev_sym[CONV_CCH_N];
	int state_history[CONV_CCH_N_STATES][n];
	int min_ae;
	int min_state;
	int cur_state;

	/* Initial error (only state 0 is valid) */
	ae[0] = 0;
	for (i=1; i<CONV_CCH_N_STATES; i++) {
		ae[i] = MAX_AE;
	}

	/* Scan the treillis */
	for (i=0; i<n; i++) {
		/* Reset next accumulated error */
		for (s=0; s<CONV_CCH_N_STATES; s++) {
			ae_next[s] = MAX_AE;
		}

		/* Get input */
		in_sym[0] = input[(i<<2)  ];
		in_sym[1] = input[(i<<2)+1];
		in_sym[2] = input[(i<<2)+2];
		in_sym[3] = input[(i<<2)+3];

		/* Scan all states */
		for (s=0; s<CONV_CCH_N_STATES; s++)
		{
			/* Scan possible input bits */
			for (b=0; b<2; b++)
			{
				int nae;

				/* Next output and state */
				uint8_t out   = conv_cch_next_output[s][b];
				uint8_t state = conv_cch_next_state[s][b];

				/* Expand */
				ev_sym[0] = (out >> 3) & 1 ? -127 : 127;
				ev_sym[1] = (out >> 2) & 1 ? -127 : 127;
				ev_sym[2] = (out >> 1) & 1 ? -127 : 127;
				ev_sym[3] =  out       & 1 ? -127 : 127;

				/* New error for this path */
				#define DIFF(x,y) (((x-y)*(x-y)) >> 9)
				nae = ae[s] + \
					DIFF(ev_sym[0], in_sym[0]) + \
					DIFF(ev_sym[1], in_sym[1]) + \
					DIFF(ev_sym[2], in_sym[2]) + \
					DIFF(ev_sym[3], in_sym[3]);

				/* Is it survivor */
				if (ae_next[state] > nae) {
					ae_next[state] = nae;
					state_history[state][i+1] = s;
				}
			}
		}

		/* Copy accumulated error */
		memcpy(ae, ae_next, sizeof(int) * CONV_CCH_N_STATES);
	}

	/* Find state with least error */
	min_ae = MAX_AE;
	min_state = -1;

	for (s=0; s<CONV_CCH_N_STATES; s++)
	{
		if (ae[s] < min_ae) {
			min_ae = ae[s];
			min_state = s;
		}
	}

	/* Traceback */
	cur_state = min_state;
	for (i=n-1; i >= 0; i--)
	{
		min_state = cur_state;
		cur_state = state_history[cur_state][i+1];
		if (conv_cch_next_state[cur_state][0] == min_state)
			output[i] = 0;
		else
			output[i] = 1;
	}

	return 0;
}
