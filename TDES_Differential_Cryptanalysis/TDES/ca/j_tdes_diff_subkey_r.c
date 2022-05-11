#include "j_ca.h"

#include <limits.h>
#include "j_bitop.h"
#include "j_tdes.h"

static void kdistt(sbox_t *sbox,
			unsigned int *key4_table,
			unsigned int left4,
			unsigned int left4_);

static unsigned int kmax(unsigned int *key_table, int length);

/* tdes_diff_subkey_r: recover key bit */
void tdes_diff_subkey_r(sbox_t *s, unsigned int *tt, unsigned int *k4table)
{
	unsigned int l, l_, l4, l4_, maxv;
	int i, k4, tmp;

	tmp = 0;
	for (i = 0; i < TDES_DIFF_TEXTLEN << 2; i += 4) {
		l = tt[i + 2] >> 8;
		l_ = tt[i + 3] >> 8;
		l4 = EXTVAL(l, 7, 5) | EXTVAL(l, 5, 4) | EXTVAL(l, 1, 3) |
			EXTVAL(l, 2, 2) | EXTVAL(l, 7, 1) | EXTVAL(l, 4, 0);
		l4_ = EXTVAL(l_, 7, 5) | EXTVAL(l_, 5, 4) | EXTVAL(l_, 1, 3) |
			EXTVAL(l_, 2, 2) | EXTVAL(l_, 7, 1) | EXTVAL(l_, 4, 0);
		if ((tt[i + 2] ^ tt[i + 3]) == TDES_DIFF_CIPHER) {
			printf("%04x, %04x, %04x\n", tt[i + 2], tt[i + 3],
						tt[i + 2] ^ tt[i + 3]);
			kdistt(s, k4table, l4, l4_);
			tmp++;
		}
	}
	printf("cnt: %d\n", tmp);
	maxv = kmax(k4table, SETBIT(TDES_SUBKEYLEN >> 1) + 1);
	printf("max value: %d\n", maxv);
	for (k4 = 0; k4 < SETBIT(TDES_SUBKEYLEN >> 1) + 1; k4++)
		if (k4table[k4] == maxv)
			printf("key4: 0x%02x\n", k4);
}

/* kdistt: make key distribution table */
void kdistt(sbox_t *s, unsigned int *k4t, unsigned int l4, unsigned int l4_)
{
	int k4;
	unsigned sr, sr_;

	for (k4 = 0; k4 < SETBIT(TDES_SUBKEYLEN >> 1) + 1; k4++) {
		sr = S_(s, 1, l4 ^ k4, rc_dec_m_tdes);
		sr_ = S_(s, 1, l4_ ^ k4, rc_dec_m_tdes);
		if ((sr ^ sr_) == TDES_DIFF_SR)
			k4t[k4]++;
	}
}

/* kmax: get maximum value of key distribution table */
unsigned int kmax(unsigned int *kt, int len)
{
	int i;
	unsigned int max;

	max = 0;
	for (i = 0; i < len; i++)
		if (kt[i] > max)
			max = kt[i];
	return max;
}
