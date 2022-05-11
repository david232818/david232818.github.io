#ifndef	__J_SBA_H__
#define	__J_SBA_H__

#include "j_sbox.h"

#define	DT_ROW	SETBIT(SBOX_TDES_INPUT_LEN)
#define	DT_COL	SETBIT(SBOX_TDES_OUTPUT_LEN)

void diff_sba(sbox_t *sbox, unsigned int *differential_distribution_table);

#endif
