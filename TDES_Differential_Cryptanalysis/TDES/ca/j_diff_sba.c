#include "j_sba.h"

#include <stdio.h>
#include "j_tdes.h"
#include "j_bitop.h"

void diff_sba(sbox_t *s, unsigned int *t)
{
	unsigned int x1, x2, sr1, sr2, i, j;

	for (x1 = 0; x1 <= SETBIT(SBOX_TDES_INPUT_LEN); x1++)
		for (x2 = 0; x2 <= SETBIT(SBOX_TDES_INPUT_LEN); x2++) {
			sr1 = S_(s, 1, x1, rc_dec_m_tdes);
			sr2 = S_(s, 1, x2, rc_dec_m_tdes);
			t[(x1 ^ x2) * (DT_COL + 1) + (sr1 ^ sr2)]++;
		}
}
