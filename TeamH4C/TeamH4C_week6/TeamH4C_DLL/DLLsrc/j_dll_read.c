#include <stdio.h>
#include <stdint.h>
#include "j_dll_node.h"

#include "j_dll.h"

/* j_dll_read: read all of dll node data */
int j_dll_read(j_dll_t *l)
{
    j_dllnode_t *curr_node_ptr;

    if (l == NULL)
	return -1;
    curr_node_ptr = j_dll_head(l);
    while (curr_node_ptr != NULL) {
	if (j_dllnode_read(curr_node_ptr, l->rmethod) == -1)
	    return -1;
	curr_node_ptr = j_next_node_of(curr_node_ptr);
    }
    return 0;
}
