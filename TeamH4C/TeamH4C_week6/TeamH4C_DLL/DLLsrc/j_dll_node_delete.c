#include <stdio.h>
#include "j_dll_node.h"

#include "j_dll.h"

/* j_dll_node_delete: delete node from dll */
int j_dll_node_delete(j_dll_t *l,
		      void *data)
{
    j_dllnode_t *curr_node_ptr;

    if (l == NULL || data == NULL || j_dll_is_empty(l))
	return -1;
    curr_node_ptr = l->node_search(l, data);
    if (curr_node_ptr == NULL)
	return -1;
    if (curr_node_ptr == j_dll_head(l)) {
	l->head = j_next_node_of(curr_node_ptr);
	if (j_dll_head(l) != NULL)
	    j_previous_node_of(j_next_node_of(curr_node_ptr)) = NULL;
	j_next_node_of(curr_node_ptr) = NULL;
	j_dllnode_delete(curr_node_ptr);
    } else {
	j_next_node_of(j_previous_node_of(curr_node_ptr)) =
	    j_next_node_of(curr_node_ptr);
	if (j_next_node_of(curr_node_ptr) != NULL) {
	    j_previous_node_of(j_next_node_of(curr_node_ptr)) =
		j_previous_node_of(curr_node_ptr);
	    j_previous_node_of(curr_node_ptr) = NULL;
	    j_next_node_of(curr_node_ptr) = NULL;
	    j_dllnode_delete(curr_node_ptr);
	} else {
	    j_dll_tail(l) = j_previous_node_of(curr_node_ptr);
	    j_next_node_of(j_previous_node_of(curr_node_ptr)) = NULL;
	    j_previous_node_of(curr_node_ptr) = NULL;
	    j_dllnode_delete(curr_node_ptr);
	}
    }
    j_dll_cnt(l)--;
    return 0;
}
