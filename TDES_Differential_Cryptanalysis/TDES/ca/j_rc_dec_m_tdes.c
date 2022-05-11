#include "j_tdes.h"

#include "j_bitop.h"

/* rc_dec_m_tdes: row and column decision method for TDES */
void rc_dec_m_tdes(unsigned int in, int *r, int *c)
{
	*r = EXTVAL(in, 5, 1) | EXTVAL(in, 0, 0);
	*c = EXTVAL(in, 4, 3) | EXTVAL(in, 3, 2) |
		EXTVAL(in, 2, 1) | EXTVAL(in, 1, 0);
	/*
	*r = (EXTBIT(in, 5) << 1) | EXTBIT(in, 0);
	*c = (EXTBIT(in, 4) << 3) |
		(EXTBIT(in, 3) << 2) |
		(EXTBIT(in, 2) << 1) |
		(EXTBIT(in, 1));
	*/
}

