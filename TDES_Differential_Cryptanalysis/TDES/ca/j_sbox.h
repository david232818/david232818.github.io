#ifndef	__J_SBOX_H__
#define	__J_SBOX_H__

#include <stdint.h>

struct sbox {
	/* All sbox lookup tables should be ordered left to right */
	unsigned int *lookup;
	int rc[2];
	int cnt;
	int iolen[2];
};

typedef struct sbox sbox_t;

sbox_t *init_S(unsigned int *lookup,
		int *row_and_column,
		int sbox_count,
		int *input_len_and_output_len);

unsigned int S_(sbox_t *sbox,
			int sbox_number,
			unsigned int input,
			void (*rc_dec_method)(unsigned int input_,
						int *row_,
						int *column_));
unsigned int S(sbox_t *sbox,
		uint64_t input_,
		void (*rc_dec_method)(unsigned int input_,
						int *row_,
						int *column_));

#endif
