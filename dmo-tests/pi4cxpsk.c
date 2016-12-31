/* GMR-1 SDR - pi2-CBPSK, pi4-CBPSK & pi4-CQPSK modulation support */
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

/*! \addtogroup pi4cxpsk
 *  @{
 */

/*! \file sdr/pi4cxpsk.c
 *  \brief Osmocom GMR-1 pi2-CBPSK, pi4-CBPSK and pi4-CQPSK modulation support implementation
 */

#include <complex.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <osmocom/core/bits.h>

#include <osmocom/dsp/cxvec.h>
#include <osmocom/dsp/cxvec_math.h>

#if 0
#include <osmocom/gmr1/sdr/defs.h>
#include <osmocom/gmr1/sdr/pi4cxpsk.h>
#else
#include "defs.h"
#include "pi4cxpsk.h"
#endif


/*
 * Symbol notation
 *
 * idx  data     modulating
 *      bits        phase
 *
 * pi4-CBPSK:
 *
 *  0    0     0 * pi/2 =  1+0j
 *  1    1     2 * pi/2 = -1+0j
 *
 * pi4-CQPSK:
 *
 *  0    00    0 * pi/2 =  1+0j
 *  1    01    1 * pi/2 =  0+1j
 *  2    11    2 * pi/2 = -1+0j
 *  3    10    3 * pi/2 =  0-1j
 *
 * - idx              : Symbol number
 * - data bits        : The encoded data bits
 * - modulating phase : Phase used during modulation (in adition to the pi/4
 *                      continuous rotation)
 */

/*! \brief pi{2,4}-CBPSK symbols descriptions */
static struct gmr1_pi4cxpsk_symbol gmr1_piNcbpsk_syms_bits[] = {
	{ 0, {0}, 0*M_PIf/2,  1+0*I },
	{ 1, {1}, 2*M_PIf/2, -1+0*I },
};

/*! \brief pi2-CBPSK modulation description */
struct gmr1_pi4cxpsk_modulation gmr1_pi2cbpsk = {
	.rotation = M_PIf/2,
	.nbits = 1,
	.syms = gmr1_piNcbpsk_syms_bits,
	.bits = gmr1_piNcbpsk_syms_bits,
};

/*! \brief pi4-CBPSK modulation description */
struct gmr1_pi4cxpsk_modulation gmr1_pi4cbpsk = {
	.rotation = M_PIf/4,
	.nbits = 1,
	.syms = gmr1_piNcbpsk_syms_bits,
	.bits = gmr1_piNcbpsk_syms_bits,
};


/*! \brief pi4-CQPSK symbols descriptions in symbol order */
static struct gmr1_pi4cxpsk_symbol gmr1_pi4cqpsk_syms[] = {
	{ 0, {0,0}, 0*M_PIf/2,  1+0*I },
	{ 1, {0,1}, 1*M_PIf/2,  0+1*I },
	{ 2, {1,1}, 2*M_PIf/2, -1+0*I },
	{ 3, {1,0}, 3*M_PIf/2,  0-1*I },
};

/*! \brief pi4-CQPSK symbols descriptions in bits order */
static struct gmr1_pi4cxpsk_symbol gmr1_pi4cqpsk_bits[] = {
	{ 0, {0,0}, 0*M_PIf/2,  1+0*I },
	{ 1, {0,1}, 1*M_PIf/2,  0+1*I },
	{ 3, {1,0}, 3*M_PIf/2,  0-1*I },
	{ 2, {1,1}, 2*M_PIf/2, -1+0*I },
};

/*! \brief pi4-CQPSK modulation description */
struct gmr1_pi4cxpsk_modulation gmr1_pi4cqpsk = {
	.rotation = M_PIf/4,
	.nbits = 2,
	.syms = gmr1_pi4cqpsk_syms,
	.bits = gmr1_pi4cqpsk_bits,
};



/*! \brief Generate a reference signal for all sync sequences of a burst type
 *  \param[in] burst_type Burst format description
 *  \returns 0 for success. -ernno for errors
 *
 * The reference waveforms are stored inside the burst_type itself.
 */
static int
_gmr1_pi4cxpsk_sync_gen_ref(struct gmr1_pi4cxpsk_burst *burst_type)
{
	int i, j;

	/* Scan all possible training sequences */
	for (i=0; (i < GMR1_MAX_SYNC) && (burst_type->sync[i] != NULL); i++)
	{
		struct gmr1_pi4cxpsk_sync *csync;

		/* Scan all 'chunks' */
		for (csync=burst_type->sync[i]; csync->pos>=0; csync++)
		{
			int is_real = 1;

			/* Already done ? */
			if (csync->_ref)
				continue;

			/* Allocate it */
			csync->_ref = osmo_cxvec_alloc(csync->len);
			if (!csync->_ref)
				return -ENOMEM;

			/* Fill it */
			for (j=0; j<csync->len; j++) {
				int s;
				float complex mv;

				s = csync->syms[j];
				mv = burst_type->mod->syms[s].mod_val;

				if (cimagf(mv) != 0.0f)
					is_real = 0;

				csync->_ref->data[j] = mv;
			}

			csync->_ref->len = csync->len;

			if (is_real)
				csync->_ref->flags |= CXVEC_FLG_REAL_ONLY;
		}
	}

	return 0;
}

/*! \brief Find the sync sequence inside a burst
 *  \param[in] burst_type Burst format description
 *  \param[in] burst The input complex vector
 *  \param[in] sps Input sample per symbol (how much to decimate)
 *  \param[out] toa Pointer to estimated fractional TOA return variable
 *  \param[out] pwr Pointer to power return variable
 *  \returns >=0 index of found sync sequence. -errno for errors
 *
 * The burst input is expected to be longer than the burst. The extra amount
 * of samples will be the search window.
 */
static int
_gmr1_pi4cxpsk_sync_find(struct gmr1_pi4cxpsk_burst *burst_type,
                         struct osmo_cxvec *burst, int sps,
                         float *toa, float *pwr)
{
	struct osmo_cxvec _win, *win = &_win;
	struct osmo_cxvec *corr, *corr_tmp;
	int i, j, w;
	float p_toa = 0.0f, p_pwr = 0.0f, p_idx = -1;
	int rv;

	/* Window size */
	w = burst->len - (burst_type->len * sps) + 1;

	/* Corr vectors */
	corr = osmo_cxvec_alloc(w);
	corr_tmp = osmo_cxvec_alloc(w);

	if (!corr || !corr_tmp) {
		rv = -ENOMEM;
		goto err;
	}

	/* Scan all possible training sequences */
	for (i=0; (i < GMR1_MAX_SYNC) && (burst_type->sync[i] != NULL); i++)
	{
		struct gmr1_pi4cxpsk_sync *csync;
		float s_toa, s_pwr;
		float complex s_peak;
		int first = 1, tl = 0;

		/* Correlate all 'chunks' */
		for (csync=burst_type->sync[i]; csync->pos>=0; csync++)
		{
			int b, l;

			/* Extract the window of data to correlate with */
			b =  csync->pos * sps;
			l = (csync->len * sps) + w - 1;
			osmo_cxvec_init_from_data(win, &burst->data[b], l);

			/* Correlate */
			osmo_cxvec_correlate(csync->_ref, win, sps, first ? corr : corr_tmp);

			/* If not the first, then combine results */
			if (!first)
				for (j=0; j<w; j++)
					corr->data[j] += corr_tmp->data[j];

			first = 0;

			/* Add length of this 'chunk' */
			tl += csync->_ref->len;
		}

		/* Only considered properly aligned correlation */
		for (j=0; j<corr->len; j++)
			corr->data[j] = (crealf(corr->data[j]) > 0.0f) ? crealf(corr->data[j]) : 0.0f;

		/* Find peak */
		s_toa = osmo_cxvec_peak_energy_find(corr, 3, PEAK_EARLY_LATE, &s_peak);
		s_peak /= (float)tl;
		s_pwr = osmo_normsqf(s_peak);

		if (s_pwr > p_pwr) {
			/* Record the new winner */
			p_pwr = s_pwr;
			p_toa = s_toa;
			p_idx = i;

			/* Debug winner */
			DEBUG_SIGNAL("pi4cxpsk_corr", corr);
		}
	}

	/* Return winner */
	if (toa)
		*toa = p_toa;
	if (pwr)
		*pwr = p_pwr;
	rv = p_idx;

	/* Clean up */
err:
	osmo_cxvec_free(corr_tmp);
	osmo_cxvec_free(corr);

	return rv;
}

/*! \brief Perform final alignement (1 sps and proper length/alignement)
 *  \param[in] burst_type Burst format description
 *  \param[in] burst The input complex vector
 *  \param[in] sps Input sample per symbol (how much to decimate)
 *  \param[in] toa Estimated fractional TOA to align to
 *  \returns 0 for success. -errno for errors
 *
 *  In the end, each complex inside the burst corresponds to a sample,
 *  aligned according to the burst description.
 */
static int
_gmr1_pi4cxpsk_align(struct gmr1_pi4cxpsk_burst *burst_type,
                     struct osmo_cxvec *burst, int sps, float toa)
{
	int i, rv = 0;

	if (sps >= 4) {
		/* Easy case: we can just round everything and not use
		 * fractional TOA. At worse we have a +-1/8 symbol alignement
		 * error, which doesn't matter */
		int d;

		d = roundf(toa);

		for (i=0; i<burst_type->len; i++)
			burst->data[i] = burst->data[i*sps+d];

		burst->len = burst_type->len;
	} else {
		/* Hard case: we need to interpolate every point */
		struct osmo_cxvec *conv = NULL, *src = burst;
		int ofs_int;
		float ofs_frac;

		ofs_int = roundf(toa);
		ofs_frac = toa - ofs_int;

		src = burst;

		/* Fractional part (if reasonable) */
		if (fabs(ofs_frac) > 0.1f) {
			const int N = 21;
			float complex _data[N];
			struct osmo_cxvec _sinc_pulse, *sinc_pulse = &_sinc_pulse;

			/* Build sinc pulse */
			for (i=0; i<N; i++)
				_data[i] = osmo_sinc(
					M_PIf * ((float)(i - (N>>1)) + ofs_frac)
				);

			osmo_cxvec_init_from_data(sinc_pulse, _data, N);
			sinc_pulse->flags |= CXVEC_FLG_REAL_ONLY;

			/* Apply it */
			conv = osmo_cxvec_convolve(sinc_pulse, burst, CONV_NO_DELAY, NULL);
			src = conv;
		}

		/* Integer part */
		for (i=0; i<burst_type->len; i++) {
			int j = (i*sps) + ofs_int;
			if (j < 0 || j >= src->len)
				burst->data[i] = 0.0f;
			else
				burst->data[i] = src->data[j];
		}

		burst->len = burst_type->len;

		/* Cleanup */
		if (conv)
			osmo_cxvec_free(conv);
	}

	DEBUG_SIGNAL("pi4cxpsk_align", burst);

	return rv;
}

/*! \brief Estimate fine frequency error based on sync sequence chunks phase
 *  \param[in] burst_type Burst format description
 *  \param[in] burst The input complex vector (1 sample per symbol)
 *  \param[in] sync_id ID of the sync sequence to use
 *  \param[out] freq_error Pointer to the return frequency error variable (rad/sym)
 *  \returns 0 for success. -errno for errors
 *
 *  The method needs several chunks to estimate the frequency error. If
 *  there is only one, 0.0f is returned.
 */
static int
_gmr1_pi4cxpsk_freq_err(struct gmr1_pi4cxpsk_burst *burst_type,
                        struct osmo_cxvec *burst, int sync_id,
                        float *freq_error)
{
	struct gmr1_pi4cxpsk_sync *csync;
	int n, i, j;

	/* Count the chunks */
	for (n=0,csync=burst_type->sync[sync_id]; csync->pos>=0; n++,csync++);

	/* Do we have several ? */
	if (n > 1)
	{
		float complex corr[n];
		float pos[n], f;

		/* Correlate all 'chunks' */
		for (i=0; i<n; i++)
		{
			csync = &burst_type->sync[sync_id][i];

			corr[i] = 0.0f;
			pos[i] = (float)csync->pos + (float)csync->len / 2.0f;

			for (j=0; j<csync->len; j++)
				corr[i] +=
					conjf(csync->_ref->data[j]) *
					burst->data[csync->pos+j];
		}

		/* From the data points, extract a single value */
		f = 0.0f;
		for (i=1; i<n; i++)
			f += cargf(corr[i] * conjf(corr[0])) / (pos[i] - pos[0]);
		f /= n - 1;

		*freq_error = f;
	}
	else
	{
		/* FIXME: How the hell to do this reliably ??? */
		*freq_error = 0.0f;
	}

	return 0;
}

/*! \brief Compute the current phase of a burst (compared to a 0 reference)
 *  \param[in] burst_type Burst format description
 *  \param[in] burst The input complex vector (1 sample per symbol)
 *  \param[in] sync_id ID of the sync sequence to use
 *  \param[out] phasor Pointer to the return phase variable
 *  \returns 0 for success. -errno for errors
 */
static int
_gmr1_pi4cxpsk_phase(struct gmr1_pi4cxpsk_burst *burst_type,
                     struct osmo_cxvec *burst, int sync_id,
                     float complex *phasor)
{
	struct gmr1_pi4cxpsk_sync *csync;
	float complex corr = 0.0f;
	int i;

	/* Correlate all 'chunks' */
	for (csync=burst_type->sync[sync_id]; csync->pos>=0; csync++)
		for (i=0; i<csync->len; i++)
			corr += conjf(csync->_ref->data[i]) *
				burst->data[csync->pos+i];

	*phasor = corr / cabsf(corr);

	return 0;
}

/*! \brief Convert complex vector into soft symbols based on phase
 *  \param[in] burst_type Burst format description
 *  \param[in] burst The input complex vector
 *  \returns Newly malloc'd array of float of same legnth as burst
 *
 * Phase must have been aligned properly obviously
 */
static float *
_gmr1_pi4cxpsk_soft_symbols(struct gmr1_pi4cxpsk_burst *burst_type,
                            struct osmo_cxvec *burst)
{
	float *ssyms;
	float d;
	int i;

	ssyms = malloc(sizeof(float) * burst->len);
	if (!ssyms)
		return NULL;

	d = (2.0f * M_PIf) / (1<<burst_type->mod->nbits);

	for (i=0; i<burst->len; i++)
		ssyms[i] = cargf(burst->data[i]) / d;

	return ssyms;
}

/*! \brief Convert a soft symbols array into softbits
 *  \param[in] burst_type Burst format description
 *  \param[in] ssyms Soft symbols array
 *  \param[out] ebits Encoded soft bits return array
 *  \returns 0 for success. -errno for errors
 */
static int
_gmr1_pi4cxpsk_soft_bits(struct gmr1_pi4cxpsk_burst *burst_type,
                         float *ssyms, sbit_t *ebits)
{
	struct gmr1_pi4cxpsk_modulation *mod = burst_type->mod;
	struct gmr1_pi4cxpsk_data *dc;
	int mask = (1<<mod->nbits) - 1;
	int i,j,k;

	k=0;

	for (dc = burst_type->data; dc->pos>=0; dc++) {
		for (i=dc->pos; i<dc->pos+dc->len; i++)
		{
			float sv, svr;
			int sp, ss, d;

			sv  = ssyms[i];
			svr = roundf(sv);

			sp = (int)svr & mask;
			ss = (svr > sv ? (sp-1) : (sp+1)) & mask;

			d = roundf((2.0f * fabs(svr - sv)) * 64.0f);

			for (j=0; j<mod->nbits; j++) {
				uint8_t vp = mod->syms[sp].data[j];
				uint8_t vs = mod->syms[ss].data[j];
				sbit_t v = 127 - ((vp^vs) ? d : (d>>1));
				ebits[k++] = vp ? -v : v;
			}
		}
	}

	return 0;
}

/*! \brief All-in-one pi4-CxPSK demodulation method
 *  \param[in] burst_type Burst format description
 *  \param[in] burst_in Complex signal of the burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to burst_in (rad/sym)
 *  \param[out] ebits Encoded soft bits return array
 *  \param[out] sync_id_p Pointer to sync sequence id return variable
 *  \param[out] toa_p Pointer to TOA return variable
 *  \param[out] freq_err_p Pointer to frequency error return variable (rad/sym)
 *  \returns 0 for success. -errno for errors
 *
 * burst_in is expected to be longer than necessary. Any extra length will be
 * used as 'search window' to find proper alignement. Good practice is to have
 * a few samples too much in front and a few samples after the expected TOA.
 */
int
gmr1_pi4cxpsk_demod(struct gmr1_pi4cxpsk_burst *burst_type,
                    struct osmo_cxvec *burst_in, int sps, float freq_shift,
                    sbit_t *ebits,
                    int *sync_id_p, float *toa_p, float *freq_err_p)
{
	struct osmo_cxvec *burst = NULL;
	float toa, fine_freq_error;
	float complex phasor;
	float *ssyms = NULL;
	int sync_id;
	int rv = 0;

	/* Generate reference sync bursts */
	rv = _gmr1_pi4cxpsk_sync_gen_ref(burst_type);
	if (rv)
		goto err;

	/* Normalize the burst and counter rotate by pi/4 */
	burst = osmo_cxvec_sig_normalize(burst_in, 1, (freq_shift - burst_type->mod->rotation) / sps, NULL);
	if (!burst) {
		rv = -ENOMEM;
		goto err;
	}

	DEBUG_SIGNAL("pi4cxpsk_burst", burst);

	/* Find the training sequence */
	sync_id = _gmr1_pi4cxpsk_sync_find(burst_type, burst, sps, &toa, NULL);
	if (sync_id < 0) {
		rv = sync_id;
		goto err;
	}

	if (sync_id_p)
		*sync_id_p = sync_id;

	if (toa_p)
		*toa_p = toa;

	/* Align and decimate the burst */
	rv = _gmr1_pi4cxpsk_align(burst_type, burst, sps, toa);
	if (rv)
		goto err;

#if 0
	/* Use sync sequence to find fine freq error */
	rv = _gmr1_pi4cxpsk_freq_err(burst_type, burst, sync_id, &fine_freq_error);
	if (rv)
		goto err;

	if (freq_err_p)
		*freq_err_p = fine_freq_error;

	/* Compensate fine freq error (in-place) */
	if (fine_freq_error != 0.0f)
		osmo_cxvec_rotate(burst, -fine_freq_error, burst);

	/* Find current phase using sync sequence */
	_gmr1_pi4cxpsk_phase(burst_type, burst, sync_id, &phasor);

	/* Align phase for detection */
	osmo_cxvec_scale(burst, conjf(phasor), burst);
#endif
	DEBUG_SIGNAL("pi4cxpsk_final", burst);

	/* Convert phase to soft symbols */
	ssyms = _gmr1_pi4cxpsk_soft_symbols(burst_type, burst);
	if (!ssyms) {
		rv = -ENOMEM;
		goto err;
	}

	/* Convert to data bits */
	rv = _gmr1_pi4cxpsk_soft_bits(burst_type, ssyms, ebits);
	if (rv)
		goto err;

	/* Cleanup */
err:
	free(ssyms);
	osmo_cxvec_free(burst);

	return rv;
}

/*! \brief Try to identify burst type by matching training sequences
 *  \param[in] burst_types Array of burst types to test (NULL terminated)
 *  \param[in] e_toa Expected time of arrival
 *  \param[in] burst_in Complex signal of the burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to burst_in (rad/sym)
 *  \param[out] bt_id_p Pointer to burst type ID return variable
 *  \param[out] sync_id_p Pointer to sync sequence id return variable
 *  \param[out] toa_p Pointer to TOA return variable
 *  \returns -errno for errors, 0 for success
 *
 * The various burst types must be compatible in length and modulation !
 */
int
gmr1_pi4cxpsk_detect(struct gmr1_pi4cxpsk_burst **burst_types, float e_toa,
                     struct osmo_cxvec *burst_in, int sps, float freq_shift,
                     int *bt_id_p, int *sync_id_p, float *toa_p)
{
	struct gmr1_pi4cxpsk_burst *bt;
	struct osmo_cxvec *burst = NULL;
	int id, p_id=-1, p_sid=-1;
	float p_toa=0.0f, p_pwr=0.0f;
	int rv = 0;

	/* Normalize the burst and counter rotate */
	burst = osmo_cxvec_sig_normalize(burst_in, 1, (freq_shift - burst_types[0]->mod->rotation) / sps, NULL);
	if (!burst) {
		rv = -ENOMEM;
		goto err;
	}

	DEBUG_SIGNAL("pi4cxpsk_burst", burst);

	/* Scan all burst types */
	for (id=0; burst_types[id]; id++)
	{
		int sid;
		float toa, pwr;

		bt = burst_types[id];

		/* Generate reference sync bursts */
		rv = _gmr1_pi4cxpsk_sync_gen_ref(bt);
		if (rv)
			goto err;

		/* Try this burst type */
		sid = _gmr1_pi4cxpsk_sync_find(bt, burst, sps, &toa, &pwr);
		if (sid < 0) {
			rv = sid;
			goto err;
		}

		/* If we have an expected, toa, we 'modulate' power */
		if (e_toa >= 0.0f)
			pwr /= fabs(e_toa - toa);

		/* Check for better ? */
		if (pwr > p_pwr) {
			p_id  = id;
			p_sid = sid;
			p_pwr = pwr;
			p_toa = toa;
		}
	}

	if (bt_id_p)
		*bt_id_p = p_id;
	if (sync_id_p)
		*sync_id_p = p_sid;
	if (toa_p)
		*toa_p = p_toa;

	/* Done */
err:
	osmo_cxvec_free(burst);

	return rv;
}

/*! \brief Estimates modulation order by comparing power of x^2 vs x^4
 *  \param[in] burst_in Complex signal of the burst
 *  \param[in] sps Oversampling used in the input complex signal
 *  \param[in] freq_shift Frequency shift to pre-apply to burst_in (rad/sym)
 *  \returns <0 for error. 2 for BPSK, 4 for QPSK.
 *
 *  Since x^4 only make sense for pi/4 variant, the pi/4 counter rotation is
 *  always applied.
 */
int
gmr1_pi4cxpsk_mod_order(struct osmo_cxvec *burst_in, int sps, float freq_shift)
{
	struct osmo_cxvec *burst = NULL;
	float complex sb = 0.0f, sq = 0.0f;
	float pb, pq;
	int rv, i;

	/* Normalize the burst and counter rotate by pi/4 */
	burst = osmo_cxvec_sig_normalize(burst_in, 1, (freq_shift - (M_PIf/4)) / sps, NULL);
	if (!burst) {
		rv = -ENOMEM;
		goto err;
	}

	DEBUG_SIGNAL("pi4cxpsk_burst", burst);

	/* Detect modulation order by estimating power of x^2 vs x^4 */
	for (i=0; i<burst->len; i++) {
		float complex v;
		v = burst->data[i];
		v = (v * v) / osmo_normsqf(v);
		sb += v;
		sq += v * v;
	}

	pb = osmo_normsqf(sb);
	pq = osmo_normsqf(sq);

	rv = pb < (pq / 2.0f) ? 4 : 2;

	/* Done */
err:
	osmo_cxvec_free(burst);

	return rv;
}

/*! \brief Modulates (currently at 1 sps)
 *  \param[in] burst_type Burst format description
 *  \param[in] ebits Encoded hard bits to pack in the burst
 *  \param[in] sync_id The sequence id to use (0 if burst_type only has one)
 *  \param[out] burst_out Complex signal to fill with modulated symbols
 *  \returns 0 for success. -errno for errors
 *
 *  burst_out is expected to be long enough to contains the resulting symbols
 *  see the burst_type structure for how long that is.
 */
int
gmr1_pi4cxpsk_mod(struct gmr1_pi4cxpsk_burst *burst_type,
                  ubit_t *ebits, int sync_id, struct osmo_cxvec *burst_out)
{
	struct gmr1_pi4cxpsk_modulation *mod = burst_type->mod;
	struct gmr1_pi4cxpsk_sync *sync;
	struct gmr1_pi4cxpsk_data *data;
	int rv, i, j, k;

	/* Check the output vector is long enough */
	if (burst_out->max_len < burst_type->len) {
		rv = -ENOMEM;
		goto err;
	}

	burst_out->len = burst_type->len;

	/* Generate reference sync bursts */
	rv = _gmr1_pi4cxpsk_sync_gen_ref(burst_type);
	if (rv)
		goto err;

	/* Fill guard */
	for (i=0; i<burst_type->guard_pre; i++)
		burst_out->data[i] = 0.0f;
	for (i=0; i<burst_type->guard_post; i++)
		burst_out->data[burst_out->len - i - 1] = 0.0f;

	/* Fill training sequence */
	for (sync=burst_type->sync[sync_id]; sync->len; sync++)
	{
		for (i=0; i<sync->len; i++)
			burst_out->data[sync->pos+i] = sync->_ref->data[i];
	}

	/* Fill ebits */
	k = 0;

	for (data=burst_type->data; data->len; data++)
	{
		for (i=0; i<data->len; i++)
		{
			int sym = 0;

			for (j=0; j<mod->nbits; j++)
				sym = (sym << 1) | ebits[k++];

			burst_out->data[data->pos+i] = burst_type->mod->bits[sym].mod_val;
		}
	}

	/* Apply the final pi/4 rotation */
	osmo_cxvec_rotate(burst_out, burst_type->mod->rotation, burst_out);

	rv = 0;

err:
	return rv;
}

/*! @} */
