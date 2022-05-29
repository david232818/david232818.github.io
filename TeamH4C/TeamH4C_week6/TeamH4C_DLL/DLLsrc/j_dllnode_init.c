#include <stdio.h>
#include <stdlib.h>

#include "j_dll_node.h"

/* j_dllnode_init: initialize node */
j_dllnode_t *j_dllnode_init(void *data)
{
    j_dllnode_t *n;

    if ((n = malloc(sizeof(j_dllnode_t))) == NULL)
	return NULL;
    j_dllnode_data(n) = data;
    j_previous_node_of(n) = NULL;
    j_next_node_of(n) = NULL;
    return n;
}
