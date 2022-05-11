#include <string.h>
#include "j_sba.h"
#include "j_bitop.h"
#include "j_tdes.h"
#include "j_ca.h"

int main()
{
	sbox_t *sb;
	int rc[2], io[2], i;
	unsigned int sbatab[(DT_ROW + 1) * (DT_COL + 1)];
	unsigned int texttab[TDES_DIFF_TEXTLEN << 2];
	unsigned int key4[SETBIT(TDES_SUBKEYLEN >> 1) + 1];
	FILE *textf;

	rc[0] = SBOX_TDES_ROW;
	rc[1] = SBOX_TDES_COL;
	io[0] = SBOX_TDES_INPUT_LEN;
	io[1] = SBOX_TDES_OUTPUT_LEN;
	sb = init_S(sbox_tdes, rc, SBOX_TDES_CNT, io);

	memset(sbatab, 0x0, (DT_ROW+1) * (DT_COL+1) * sizeof(unsigned int));
	diff_sba(sb, sbatab);

	textf = fopen("./data/outDiff", "r");
	tdes_diff_tt(textf, texttab);

	/*
	for (int i = 0; i < TDES_DIFF_TEXTLEN << 2; i += 4) {
		printf("%04x, %04x, %04x, %04x\t", texttab[i], texttab[i + 1],
					texttab[i + 2], texttab[i + 3]);
		printf("%04x, %04x\n", texttab[i] ^ texttab[i + 1],
					texttab[i + 2] ^ texttab[i + 3]);
	}
	*/

	memset(key4, 0x0, (SETBIT(TDES_SUBKEYLEN>>1)+1)*sizeof(unsigned int));
	tdes_diff_subkey_r(sb, texttab, key4);
	return 0;
}
