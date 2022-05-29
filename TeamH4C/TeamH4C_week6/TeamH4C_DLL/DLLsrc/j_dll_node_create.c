#include <stdio.h>
#include "j_dll_node.h"

#include "j_dll.h"

/* j_dll_node_create: add node to dll */
int j_dll_node_create(j_dll_t *l, void *data)
{
    j_dllnode_t *nnode;

    if (l == NULL || data == NULL || j_dll_is_full(l))
	return -1;
    if ((nnode = j_dllnode_init(data)) == NULL)
	return -1;
    if (l->update(l, nnode) == -1)
	return -1;
    return 0;
}
