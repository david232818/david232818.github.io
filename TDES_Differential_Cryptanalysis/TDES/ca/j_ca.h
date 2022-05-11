#ifndef	__J_CA_H__
#define	__J_CA_H__

#include <stdio.h>
#include "j_sbox.h"

#define	TDES_DIFF_TEXTLEN	100

#define	TDES_DIFF_CIPHER	0x0202
#define	TDES_DIFF_SR		0x02

void tdes_diff_tt(FILE *diff_text_file, unsigned int *diff_text_table);

void tdes_diff_subkey_r(sbox_t *sbox,
			unsigned int *diff_text_table,
			unsigned int *key4_table);

#endif
