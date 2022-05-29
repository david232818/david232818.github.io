#ifndef __J_DLL_H__
#define __J_DLL_H__

#include <stddef.h>
#include <stdint.h>

#include "j_dll_node.h"

typedef struct j_dll {
    j_dllnode_t *head;		/* head node of a list */
    j_dllnode_t *tail;		/* tail node of a list */

    size_t cnt;			/* node count of a list */

    int (*umethod)(void *, void *); /* update method */
    int (*cmethod)(void *, void *); /* compare method */
    int (*rmethod)(void *); /* read method */

    int (*update)(struct j_dll *,
		  j_dllnode_t *);
    int (*read)(struct j_dll *);

    j_dllnode_t* (*node_search)(struct j_dll *,
				void *);
    int (*node_create)(struct j_dll *,
		       void *);
    int (*node_update)(struct j_dll *,
		       void *,
		       void *);
    int (*node_delete)(struct j_dll *, void *);
} j_dll_t;

#define j_dll_head(dll) ((dll)->head)
#define j_dll_tail(dll) ((dll)->tail)
#define j_dll_cnt(dll) ((dll)->cnt)

#define j_dll_is_empty(dll) (j_dll_cnt((dll)) == 0)
#define j_dll_is_full(dll) (j_dll_cnt((dll)) >= SIZE_MAX)

/*
 * j_dll_init: initialize dll list, user shall provide
 * update method (umethod), compare method (cmethod),
 * read method (rmethod)
 */
extern j_dll_t *j_dll_init(int (*umethod)(void *, void *),
			   int (*cmethod)(void *, void *),
			   int (*rmethod)(void *));

/* j_dll_update: update list l by umethod with adding n or without adding n */
extern int j_dll_update(j_dll_t *list,
			j_dllnode_t *update_node);

/* j_dll_read: read all of dll node data */
extern int j_dll_read(j_dll_t *list);

/* j_dll_node_search: search node by given data */
extern j_dllnode_t *j_dll_node_search(j_dll_t *list,
				      void *search_data);

/* j_dll_node_create: add node to dll */
extern int j_dll_node_create(j_dll_t *list,
			     void *new_data);

/* j_dll_node_update: update node data in dll */
extern int j_dll_node_update(j_dll_t *list,
			     void *original_data,
			     void *new_data);

/* j_dll_node_delete: delete node from dll */
extern int j_dll_node_delete(j_dll_t *list,
			     void *delete_data);

#endif
