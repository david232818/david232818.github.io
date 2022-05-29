#include <stdio.h>
#include "j_dll_node.h"

#include "j_dll.h"

/* j_dll_node_update: update node data in dll */
int j_dll_node_update(j_dll_t *l,
		      void *origin,
		      void *new)
{
    j_dllnode_t *curr_node_ptr;

    if (l == NULL ||
        origin == NULL ||
	new == NULL)
	return -1;
    curr_node_ptr = l->node_search(l, origin);

    /* temporarily delete node from dll but node is not deleted from memory */
    if (curr_node_ptr != j_dll_head(l)) {
	j_next_node_of(j_previous_node_of(curr_node_ptr)) =
	    j_next_node_of(curr_node_ptr);
    }
    if (curr_node_ptr != j_dll_tail(l)) {
	j_previous_node_of(j_next_node_of(curr_node_ptr)) =
	    j_previous_node_of(curr_node_ptr);
    }
    j_previous_node_of(curr_node_ptr) = NULL;
    j_next_node_of(curr_node_ptr) = NULL;
    
    j_dllnode_update(curr_node_ptr, new, l->umethod);
    l->update(l, curr_node_ptr); /* readd node to dll */
    return 0;
}
