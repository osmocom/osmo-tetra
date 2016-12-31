
#include <stdio.h>
#include <string.h>

#include <osmocom/dsp/cfile.h>
#include <osmocom/dsp/cxvec.h>
#include <osmocom/dsp/cxvec_math.h>

#include "pi4cxpsk.h"


static struct gmr1_pi4cxpsk_sync tetra_dsb_sync[] = {
	{  17,  6, { 0, 1, 1, 0, 1, 3 } },
	{  24, 40, {
		3, 3, 3, 3,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		3, 3, 3, 3
	}},
	{ 124, 19, { 3, 0, 0, 1, 2, 1, 3, 0, 3, 2, 2, 1, 3, 0, 0, 1, 2, 1, 3 } },
	{ -1 },
};

static struct gmr1_pi4cxpsk_data tetra_dsb_data[] = {
	{  64,  60 },
	{ 143, 108 },
	{ -1 },
};


struct gmr1_pi4cxpsk_burst tetra_dsb = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 17,
	.guard_post =  3,
	.len = 255,
	.ebits = 336,
	.sync = { tetra_dsb_sync, NULL },
	.data = tetra_dsb_data,
};

static struct gmr1_pi4cxpsk_sync tetra_dnb_sync_1[] = {
	{  17,  6, { 0, 3, 0, 2, 0, 3 } },			/* Preamble 1 */
	{ 132, 11, { 3, 1, 0, 0, 3, 2, 2, 1, 3, 1, 0 } },	/* Normal training sequence 1 */
	{ -1 },
};

static struct gmr1_pi4cxpsk_sync tetra_dnb_sync_2[] = {
	{  17,  6, { 2, 1, 2, 2, 2, 1 } },			/* Preamble 2 */
	{ 132, 11, { 1, 3, 2, 2, 1, 0, 0, 3, 1, 3, 1 } },	/* Normal training sequence 2 */
	{ -1 },
};

static struct gmr1_pi4cxpsk_data tetra_dnb_data[] = {
	{  24, 108 },
	{ 143, 108 },
	{ -1 },
};

struct gmr1_pi4cxpsk_burst tetra_dnb = {
	.mod = &gmr1_pi4cqpsk,
	.guard_pre = 17,
	.guard_post =  3,
	.len = 255,
	.ebits = 432,
	.sync = { tetra_dnb_sync_1, tetra_dnb_sync_2, NULL },
	.data = tetra_dnb_data,
};



#if 0
#define EBITS 336

int main(int argc, char *argv[])
{
	struct cfile *src;
	struct osmo_cxvec *burst;
	int i;
	float toa, freq_err;
	sbit_t ebits[EBITS];
	ubit_t bits[EBITS];

	src = cfile_load(argv[1]);

	burst = osmo_cxvec_alloc(src->len);
	burst->len = src->len - 4;

	for (i=0; i<burst->len; i++)
		burst->data[i] = src->data[i+4] * conjf(src->data[i]) * cexpf(-I*M_PIf/4.0f);

	i = gmr1_pi4cxpsk_demod(&tetra_dsb, burst, 4,  tetra_dsb.mod->rotation, ebits, NULL, &toa, &freq_err);
	fprintf(stderr, "rv=%d   toa=%f freq_err=%f\n", i, toa, freq_err);

	for (i=0; i<EBITS; i++)
		bits[i] = ebits[i] < 0;

	for (i=0; i<EBITS; i++)
		printf("%d", bits[i]);
	printf("\n");

	cfile_release(src);
	
	return 0;
}

#else
static const int sps = 4;
static const int win = 20;


static void
dsb_demod(float complex *data, int len)
{
	struct osmo_cxvec *burst;
	sbit_t ebits[336];
	float toa, freq_err;
	int i;

	burst = osmo_cxvec_alloc(len);
	burst->len = len - sps;

	for (i=0; i<burst->len; i++)
		burst->data[i] = data[i+sps] * conjf(data[i]) * cexpf(-I*M_PIf/4.0f);

	i = gmr1_pi4cxpsk_demod(&tetra_dsb, burst, sps, tetra_dsb.mod->rotation, ebits, NULL, &toa, &freq_err);
	printf("rv=%d toa=%4.1f freq_err=%4.2f ", i, toa, freq_err);

	for (i=0; i<336; i++)
		printf("%d", ebits[i] < 0);
}

static void
dsb_scan(float complex *data, int n)
{
	int i, cc[sps];

	memset(cc, 0, sizeof(cc));

	for (i=0; i<n-sps; i++)
	{
		float a = cargf(data[i+sps] * conjf(data[i]));

		if ((a > 0) && (a < M_PIf))
		{
			cc[i%sps]++;
		}
		else
		{
			if (cc[i%sps] > 25)
			{
				printf("Burst @ %9d :", i);
				dsb_demod(&data[i-(24+25+win)*sps], (255+2*win)*sps);
				printf("\n");
			}
			memset(cc, 0, sizeof(cc));
		}
	}
}

int main(int argc, char *argv[])
{
	struct cfile *src;
	int i;

	src = cfile_load(argv[1]);
	if (!src)
		return 0;

	dsb_scan(src->data, src->len);

	cfile_release(src);

	return 0;
}
#endif
