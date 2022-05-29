#include <stdio.h>
#include "j_dll_node.h"

#include "j_dll.h"

/* j_dll_node_search: search node by given data */
j_dllnode_t *j_dll_node_search(j_dll_t *l, void *data)
{
    j_dllnode_t *curr_node_ptr;

    if (l == NULL || data == NULL)
	return NULL;
    curr_node_ptr = j_dll_head(l);
    while (curr_node_ptr != NULL) {
	if (l->cmethod(j_dllnode_data(curr_node_ptr), data) == 0)
	    return curr_node_ptr;
	curr_node_ptr = j_next_node_of(curr_node_ptr);
    }
    return NULL;
}
