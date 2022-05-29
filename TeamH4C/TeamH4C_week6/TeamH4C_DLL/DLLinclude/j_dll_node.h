#ifndef __J_DLL_NODE_H__
#define __J_DLL_NODE_H__

#include <stdint.h>

struct j_dllnode {
    void *data;			/* user defined data */

    struct j_dllnode *prev;	/* previous node address */
    struct j_dllnode *next;	/* next node address */
};

typedef struct j_dllnode j_dllnode_t;

#define j_dllnode_data(node) ((node)->data)
#define j_previous_node_of(node) ((node)->prev)
#define j_next_node_of(node) ((node)->next)

/* j_dllnode_init: initialize node */
extern j_dllnode_t *j_dllnode_init(void *data);

/* j_dllnode_update: update node data */
extern int j_dllnode_update(j_dllnode_t *target_node,
			    void *data,
			    int (*umethod)(void *original, void *new));

/* j_node_read: read node data by rmethod */
extern int j_dllnode_read(j_dllnode_t *target_node,
			  int (*read_method)(void *data));

/* j_dllnode_delete: delete node data */
extern void j_dllnode_delete(j_dllnode_t *target_node);

#endif
