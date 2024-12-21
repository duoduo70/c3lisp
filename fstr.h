#include "alloc.h"
#ifndef _FSTR_H_
#define _FSTR_H_
typedef unsigned char *fstr_t;

#define fstr_len(str) (*(int *)(str))
#define fstr_content(str) ((char *)(str) + sizeof(int))

// permgen 版本的 fstr 必须一直使用带有 permgen 后缀的函数（如果有），否则自动变为普通版本
// 为了在 parser 那里快速插入字符， permgen 版本的 fstr 尾部没有 '\0'
fstr_t cstr_to_fstr_permgen(mempool_handle_t handle, char *cstr);
fstr_t cstr_to_fstr(char *cstr);
char *fstr_to_cstr(fstr_t str);

fstr_t fstr_alloc(int capacity);
fstr_t fstr_alloc_permgen(mempool_handle_t handle, int capacity);
fstr_t fstr_add_capacity(fstr_t str, char add);
char *fstr_to_cstr(fstr_t str);
int fstr_eq_cstr(fstr_t a, char *b_content);

fstr_t fstr_addch(fstr_t fstr, char ch);
fstr_t fstr_addch_permgen(fstr_t fstr, char ch);

/*
retcode:
        1: is empty
        0: is not empty
*/
int fstr_isempty(fstr_t);

fstr_t fstr_addcstr(fstr_t fstr, char *cstr);
fstr_t fstr_addfstr(fstr_t fstr, fstr_t src);

void fstr_pop(fstr_t str);

fstr_t itoa(size_t num);
#endif