#include "j_sbox.h"

#include <stdlib.h>
#include "j_bitop.h"

/* init_S: initialize sbox_t type variable s */
sbox_t *init_S(unsigned int *l, int *rc, int cnt, int *iolen)
{
	sbox_t *s;

	s = malloc(sizeof(sbox_t));
	s->lookup = l;
	s->rc[0] = rc[0];
	s->rc[1] = rc[1];
	s->cnt = cnt;
	s->iolen[0] = iolen[0];
	s->iolen[1] = iolen[1];
	return s;
}

/* S_: return single sbox output of in */
unsigned int S_
(sbox_t *s, int sbn, unsigned int in, void (*rc_dmethod)(unsigned int in_,
							int *r,
							int *c))
{
	int row, col;

	(rc_dmethod)(in, &row, &col);
	return s->lookup[sbn*s->rc[0]*s->rc[1] + row*s->rc[1] + col];
}

/* S: Sbox of block cipher */
unsigned int S(sbox_t *s, uint64_t in, void (*rc_dmethod)(unsigned int in_,
							int *r,
							int *c))
{
	int i;
	unsigned int input, res;

	res = 0;
	for (i = 0; i < s->cnt; i++) {
		res <<= s->iolen[1];
		input = (in >> (s->cnt - 1 - i) * s->iolen[0]) &
			SETBIT(s->iolen[0]);
		res |= S_(s, i, input, rc_dmethod);
	}
	return res;
}
