#include <stdio.h>
#include <stdlib.h>
#include "j_dll.h"

int read_method(void *data);
int compare_method(void *d1, void *d2);
int update_method(void *origin, void *new);

/* dll library test program */
int main()
{
    j_dll_t *dll;
    j_dllnode_t *searched;
    int *data, searchdata;

    if ((dll = j_dll_init(update_method,
			  compare_method,
			  read_method)) == NULL)
	return -1;
    printf("list init successful..\n");

    data = malloc(sizeof(int));
    *data = 12;
    if (dll->node_create(dll, data) == -1)
	return -1;
    printf("cnt: %zu\n", j_dll_cnt(dll));

    data = malloc(sizeof(int));
    *data = 10;
    if (dll->node_create(dll, data) == -1)
	return -1;
    printf("cnt: %zu\n", j_dll_cnt(dll));

    data = malloc(sizeof(int));
    *data = 24;
    if (dll->node_create(dll, data) == -1)
	return -1;
    printf("cnt: %zu\n", j_dll_cnt(dll));

    data = malloc(sizeof(int));
    *data = 23;
    if (dll->node_create(dll, data) == -1)
	return -1;
    printf("cnt: %zu\n", j_dll_cnt(dll));

    data = malloc(sizeof(int));
    *data = 100;
    if (dll->node_create(dll, data) == -1)
	return -1;
    printf("cnt: %zu\n", j_dll_cnt(dll));


    searchdata = 12;
    if ((searched = dll->node_search(dll, &searchdata)) == NULL)
	printf("Not found..\n");
    printf("found: %d\n", *((int *) searched->data));

    if (dll->node_delete(dll, &searchdata) == -1)
	printf("Not deleted..\n");
    printf("Deleted..\n");
    printf("cnt: %zu\n", j_dll_cnt(dll));
    if (dll->read(dll) == -1)
	return -1;

    searchdata = 9;
    int origin = 24;
    if (dll->node_update(dll, &origin, &searchdata) == -1)
	printf("Not updated..\n");
    printf("update successful..\n");
    if (dll->read(dll) == -1)
	return -1;

    if (dll->delete(dll) == -1)
	return -1;
    printf("list delete successful..\n");
    return 0;
}

int read_method(void *data)
{
    printf("[\tdata: %04d\t]\n", *((int *) data));
    return 0;
}

int compare_method(void *d1, void *d2)
{
    int data1, data2;

    data1 = *((int *) d1);
    data2 = *((int *) d2);
    return (data1 - data2);
}

int update_method(void *d1, void *d2)
{
    return (*((int *) d1) = *((int *) d2));
}
