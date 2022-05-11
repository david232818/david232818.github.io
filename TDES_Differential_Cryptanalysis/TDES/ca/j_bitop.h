#ifndef	__J_BITOP_H__
#define	__J_BITOP_H__

/* EXTBIT: extract bit from x at pos */
#define	EXTBIT(x, pos)	(((x) >> (pos)) & 1)

/* SETBIT: get bn length setted bit set */
#define	SETBIT(bn)	(~(~0 << (bn)))

/* EXTVAL: extract value of bit of pos in x */
#define EXTVAL(x, pos, vpos) (EXTBIT((x), (pos)) << (vpos))

#endif
