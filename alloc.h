#include <stdio.h>
#ifndef _ALLOC_H_
#define _ALLOC_H_

#define C3DATA_MEMSIZE(mem) (*(size_t *)((size_t)mem - 1 * sizeof(size_t)))

typedef void *mempool_handle_t;

enum DataType { NORMAL_DATA, PERMGEN_DATA };

mempool_handle_t c3_permgen_create(size_t size);
void *c3_malloc(size_t size);
void *c3_realloc(void *src, size_t newsize);
void *c3_malloc_permgen(mempool_handle_t handle, size_t size);
void *c3_realloc_permgen(mempool_handle_t handle, void *src, size_t newsize);
void c3_free(void *mem);
enum DataType c3_data_gettype(void *data);

#endif