#include <stdio.h>
#include <unistd.h>
#include "j_dll_node.h"

#include "j_dll.h"

static int j_dll_update_with_add(j_dll_t *,
				 j_dllnode_t *);

/* j_dll_update: update list l by umethod with adding n or without adding n */
int j_dll_update(j_dll_t *l, j_dllnode_t *n)
{
    j_dllnode_t *ptr;

    if (l == NULL || n == NULL)
	return -1;
    else
	return j_dll_update_with_add(l, n);
}

/* j_dll_update_with_add: update dll with adding node */
static int j_dll_update_with_add(j_dll_t *l,
				 j_dllnode_t *n)
{
    j_dllnode_t *curr_node_ptr;
    int ures;

    if (j_dll_cnt(l) == 0) {
	j_dll_head(l) = n;
	j_dll_tail(l) = n;
	j_dll_cnt(l)++;
	return 0;
    }
    curr_node_ptr = j_dll_head(l);
    while ((ures = l->cmethod(j_dllnode_data(curr_node_ptr), n->data)) <= 0) {
	if (curr_node_ptr->next == NULL)
	    break;
	curr_node_ptr = j_next_node_of(curr_node_ptr);
    }
    if (curr_node_ptr == j_dll_head(l) && ures > 0) {
	j_dll_head(l) = n;
	j_previous_node_of(n) = NULL;
	j_next_node_of(n) = curr_node_ptr;
	j_previous_node_of(curr_node_ptr) = n;
	j_dll_tail(l) = (j_dll_cnt(l) == 1) ? curr_node_ptr : j_dll_tail(l);
    } else if (ures > 0) {
	j_previous_node_of(n) = j_previous_node_of(curr_node_ptr);
	j_next_node_of(n) = curr_node_ptr;
	j_next_node_of(j_previous_node_of(curr_node_ptr)) = n;
	j_previous_node_of(curr_node_ptr) = n;
    } else {
	j_dll_tail(l) = n;
	j_previous_node_of(n) = curr_node_ptr;
	j_next_node_of(curr_node_ptr) = n;
	j_next_node_of(n) = NULL;
    }
    j_dll_cnt(l)++;
    return 0;
}
