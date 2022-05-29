#include <stdio.h>
#include <stdlib.h>

#include "j_dll.h"

/*
 * j_dll_init: initialize dll list, user shall provide
 * update method (umethod), compare method (cmethod),
 * read method (rmethod)
 */
j_dll_t *j_dll_init(int (*umethod)(void *, void *),
		    int (*cmethod)(void *, void *),
		    int (*rmethod)(void *))
{
    j_dll_t *l;

    if ((l = malloc(sizeof(j_dll_t))) == NULL)
	return NULL;
    j_dll_head(l) = NULL;
    j_dll_tail(l) = NULL;
    j_dll_cnt(l) = 0;
    l->umethod = umethod;
    l->cmethod = cmethod;
    l->rmethod = rmethod;
    l->update = j_dll_update;
    l->node_search = j_dll_node_search;
    l->node_create = j_dll_node_create;
    l->read = j_dll_read;
    l->node_update = j_dll_node_update;
    l->node_delete = j_dll_node_delete;
    return l;
}
