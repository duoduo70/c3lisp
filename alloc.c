#include "alloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define C3DATA_PERMGEN_BOOTLEN (2 * sizeof(size_t))
#define C3DATA_NORMAL_BOOTLEN (3 * sizeof(size_t))

#define C3DATA_PERMGEN_HANDLE(mem)                                                     \
        (*(mempool_handle_t *)((size_t)mem - C3DATA_PERMGEN_BOOTLEN))

/*
对常规数据的 free 遵循引用计数，当引用计数为 0 时会被真的 free
*/
#define C3DATA_NORMAL_REFCOUNT(mem) (*(size_t *)((size_t)mem - C3DATA_NORMAL_BOOTLEN))

#define C3DATA_NORMAL_REGMEM_P(mem) ((void *)((size_t)mem - C3DATA_NORMAL_BOOTLEN))

/*
我们不会对持久代使用过的内存进行任何再分配，因为持久代内存应该不会出现几次 realloc
而我们需要在 Lexer 那里频繁向持久代申请新的内存
对持久代数据的 free 会被直接忽略
*/
#define PERMGEN_FASTALLOC_POINT(handle) (*(size_t *)(handle))

#define PERMGEN_CONTENT(handle) (void *)((size_t)handle + 1 * sizeof(size_t))

typedef void *mempool_handle_t;

mempool_handle_t c3_permgen_create(size_t size) {
        void *tmp = malloc(size);
        PERMGEN_FASTALLOC_POINT(tmp) = 0;
        return tmp;
}

void *c3_malloc_permgen(mempool_handle_t handle, size_t size) {
        *(size_t *)(((char *)PERMGEN_CONTENT(handle)) +
                    PERMGEN_FASTALLOC_POINT(handle)) = (size_t)handle;
        *(size_t *)(((char *)PERMGEN_CONTENT(handle)) +
                    PERMGEN_FASTALLOC_POINT(handle) + sizeof(size_t)) = size;

        void *ret = (char *)PERMGEN_CONTENT(handle) + PERMGEN_FASTALLOC_POINT(handle) + C3DATA_PERMGEN_BOOTLEN;

        PERMGEN_FASTALLOC_POINT(handle) += size + C3DATA_PERMGEN_BOOTLEN;

        return ret;
}

void *c3_malloc(size_t size) {
        size_t *tmp = malloc(size + C3DATA_NORMAL_BOOTLEN);
        tmp[0] = 1;            // refcount
        tmp[1] = (size_t)NULL; // handle
        tmp[2] = size;
        return (void *)tmp + C3DATA_NORMAL_BOOTLEN;
}

void *c3_realloc(void *src, size_t newsize) {
        if (c3_data_gettype(src) == NORMAL_DATA) {
                return realloc((void *)((size_t)src - C3DATA_NORMAL_BOOTLEN), newsize);
        } else {
                void *newmem = c3_malloc_permgen(C3DATA_PERMGEN_HANDLE(src), newsize);
                memcpy(newmem, src, C3DATA_MEMSIZE(src));
                return newmem;
        }
}

enum DataType c3_data_gettype(void *data) {
        if (*(size_t*)(data - 2 * sizeof(size_t)) == (size_t)NULL) {
                return NORMAL_DATA;
        } else {
                return PERMGEN_DATA;
        }
}

void c3_free(void *mem) {
        // if (c3_data_gettype(mem) == NORMAL_DATA) {
        //         C3DATA_NORMAL_REFCOUNT(mem) -= 1;
        //         if (C3DATA_NORMAL_REFCOUNT(mem) == 0) {
        //                 free(C3DATA_NORMAL_REGMEM_P(mem));
        //         }
        // }
}
