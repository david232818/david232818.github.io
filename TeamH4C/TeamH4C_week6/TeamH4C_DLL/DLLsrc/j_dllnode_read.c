#include <stdio.h>

#include "j_dll_node.h"

/* j_node_read: read node data by rmethod */
int j_dllnode_read(j_dllnode_t *n,
		   int (*rmethod)(void *data))
{
    if (n == NULL || rmethod == NULL)
	return -1;
    rmethod(j_dllnode_data(n));
    return 0;
}
