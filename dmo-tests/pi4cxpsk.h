/* GMR-1 SDR - pi2-CBPSK, pi4-CBPSK and pi4-CQPSK modulation support */
/* See GMR-1 05.004 (ETSI TS 101 376-5-4 V1.2.1) - Section 5.1 & 5.2 */

/* (C) 2011-2016 by Sylvain Munaut <tnt@246tNt.com>
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
 */

#ifndef __OSMO_GMR1_SDR_PI4CXPSK_H__
#define __OSMO_GMR1_SDR_PI4CXPSK_H__

/*! \defgroup pi4cxpsk pi2-CBPSK, pi4-CBPSK and pi4-CQPSK modulation
 *  \ingroup sdr
 *  @{
 */

/*! \file sdr/pi4cxpsk.h
 *  \brief Osmocom GMR-1 pi2-CBPSK, pi4-CBPSK and pi4-CQPSK modulation support header
 */

#include <stdint.h>
#include <osmocom/core/bits.h>
#include <osmocom/dsp/cxvec.h>


#define GMR1_MAX_SYM_EBITS	2	/*!< \brief Max e bits in a symbol */
#define GMR1_MAX_SYNC		4	/*!< \brief Max diff. sync seqs    */
#define GMR1_MAX_SYNC_SYMS	40	/*!< \brief Max sym in a sync seq  */


/*! \brief pi4-CxPSK symbol description */
struct gmr1_pi4cxpsk_symbol {
	short  idx;			/*!< \brief Symbol number          */
	ubit_t data[GMR1_MAX_SYM_EBITS];/*!< \brief Encoded data bits      */
	float  mod_phase;		/*!< \brief Phase used during mod. */
	float complex mod_val;		/*!< \brief e^(1j*mod_phase)       */
};

/*! \brief pi4-CxPSK modulation description */
struct gmr1_pi4cxpsk_modulation {
	float rotation;				/*!< \brief rotation per symbol */
	int nbits;				/*!< \brief ebits/sym           */
	struct gmr1_pi4cxpsk_symbol *syms;	/*!< \brief Symbols (sym order) */
	struct gmr1_pi4cxpsk_symbol *bits;	/*!< \brief Symbols (bit order) */
};


extern struct gmr1_pi4cxpsk_modulation gmr1_pi2cbpsk;
extern struct gmr1_pi4cxpsk_modulation gmr1_pi4cbpsk;
extern struct gmr1_pi4cxpsk_modulation gmr1_pi4cqpsk;


/*! \brief pi4-CxPSK Synchronization sequence segment description */
struct gmr1_pi4cxpsk_sync {
	int pos;				/*!< \brief Sync Position  */
	int len;				/*!< \brief Sync Length    */
	uint8_t syms[GMR1_MAX_SYNC_SYMS];	/*!< \brief Sync Symbols   */
	struct osmo_cxvec *_ref;		/*!< \brief Ref signal     */
};

/*! \brief pi4-CxPSK Data segment description */
struct gmr1_pi4cxpsk_data {
	int pos;			/*!< \brief Data chunk position    */
	int len;			/*!< \brief Data chunk length      */
};

/*! \brief pi4-CxPSK Burst format description */
struct gmr1_pi4cxpsk_burst {
	/*! \brief Modulation scheme      */
	struct gmr1_pi4cxpsk_modulation *mod;

	/*! \brief Beginning guard period */
	int guard_pre;
	/*! \brief End guard period       */
	int guard_post;

	/*! \brief Total len with guard */
	int len;
	/*! \brief Number of encoded bits */
	int ebits;

	/*! \brief Sync sequences */
	struct gmr1_pi4cxpsk_sync *sync[GMR1_MAX_SYNC];
	/*! \brief Data chunks */
	struct gmr1_pi4cxpsk_data *data;
};


int
gmr1_pi4cxpsk_demod(struct gmr1_pi4cxpsk_burst *burst_type,
                    struct osmo_cxvec *burst_in, int sps, float freq_shift,
                    sbit_t *ebits,
                    int *sync_id_p, float *toa_p, float *freq_err_p);

int
gmr1_pi4cxpsk_detect(struct gmr1_pi4cxpsk_burst **burst_types, float e_toa,
                     struct osmo_cxvec *burst_in, int sps, float freq_shift,
                     int *bt_id_p, int *sync_id_p, float *toa_p);

int
gmr1_pi4cxpsk_mod_order(struct osmo_cxvec *burst_in, int sps, float freq_shift);

int
gmr1_pi4cxpsk_mod(struct gmr1_pi4cxpsk_burst *burst_type,
                  ubit_t *ebits, int sync_id, struct osmo_cxvec *burst_out);


/*! @} */

#endif /* __OSMO_GMR1_SDR_PI4CXPSK_H__ */
