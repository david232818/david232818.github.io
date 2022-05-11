#include "j_ca.h"

#include <string.h>
#include <stdlib.h>

/*
 * text file format should be like the following:
 * 	...
 * 	plaintext1 = [hex number string] = [binary number string]
 * 	plaintext2 = [hex number string] = [binary number string]
 * 	ciphertext1 = [hex number string] = [binary number string]
 * 	ciphertext2 = [hex number string] = [binary number string]
 * 	...
 * 	plaintext1 = [hex number string] = [binary number string]
 * 	plaintext2 = [hex number string] = [binary number string]
 * 	ciphertext1 = [hex number string] = [binary number string]
 * 	ciphertext2 = [hex number string] = [binary number string]
 * 	...
 */

static unsigned int get_text_val(char *text);

/* tdes_diff_tt: copy plaintext pairs and ciphertext pairs from tfp to tt */
void tdes_diff_tt(FILE *tfp, unsigned int *tt)
{
	int i;
	char buf[BUFSIZ];

	i = 0;
	while (fgets(buf, BUFSIZ, tfp) != NULL)
		if (strcmp(buf, "\n") > 0)
			tt[i++] = get_text_val(buf);
}

/* get_text_val: return the number value of s */
unsigned int get_text_val(char *s)
{
	char *start, *end;

	start = strchr(s, '=');
	end = strrchr(s, '=');
	*(end - 1) = '\0';
	return (unsigned int) strtol(start + 2, (char **) NULL, 16);
}
