#include <stdio.h>
#include <stdlib.h>
#include "j_dll_node.h"

#include "j_dll.h"

int j_dll_delete(j_dll_t *l)
{
    j_dllnode_t *curr_node_ptr;

    if (l == NULL)
	return -1;
    curr_node_ptr = j_dll_head(l);
    while (curr_node_ptr != NULL) {
	if (l->node_delete(l, j_dllnode_data(curr_node_ptr)) == -1)
	    return -1;
	curr_node_ptr = j_next_node_of(curr_node_ptr);
    }
    free(l);
    return 0;
}
