#include "j_dll_node.h"

/* j_dllnode_update: update node data */
int j_dllnode_update(j_dllnode_t *n,
		      void *data,
		      int (*umethod)(void *orign, void *new))
{
    return umethod(j_dllnode_data(n), data); 
}
