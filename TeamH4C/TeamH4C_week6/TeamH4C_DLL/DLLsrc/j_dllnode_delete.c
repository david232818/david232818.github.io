#include <stdlib.h>

#include "j_dll_node.h"

/* j_dllnode_delete: delete node data */
void j_dllnode_delete(j_dllnode_t *n)
{
    j_dllnode_data(n) = NULL;
    j_previous_node_of(n) = NULL;
    j_next_node_of(n) = NULL;
    free(n);
}
