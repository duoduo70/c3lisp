#include "fstr.h"

#include <string.h>

#define FSTR_BOOTLEN sizeof(int)
#define FSTR_CAPACITY(str) C3DATA_MEMSIZE(str)

fstr_t cstr_to_fstr_permgen(mempool_handle_t handle, char *cstr) {
        unsigned char slen = strlen(cstr);
        fstr_t str = c3_malloc_permgen(handle, slen + FSTR_BOOTLEN);

        fstr_len(str) = slen;
        FSTR_CAPACITY(str) = slen + FSTR_BOOTLEN;
        strncpy(fstr_content(str), cstr, slen);
        return str;
}

fstr_t cstr_to_fstr(char *cstr) {
        unsigned char slen = strlen(cstr);
        fstr_t str = c3_malloc(slen + FSTR_BOOTLEN + 1);

        fstr_len(str) = slen;
        FSTR_CAPACITY(str) = slen + FSTR_BOOTLEN + 1;
        strncpy(fstr_content(str), cstr, slen + 1);
        return str;
}

char *fstr_to_cstr(fstr_t str) {
        if (c3_data_gettype(str) == NORMAL_DATA) {
                return fstr_content(str);
        } else {
                char *cstr = c3_malloc(fstr_len(str) + 1);
                strncpy(cstr, fstr_content(str), fstr_len(str));
                cstr[fstr_len(str)] = '\0';
                return cstr;
        }
}

fstr_t fstr_alloc_permgen(mempool_handle_t handle, int capacity) {
        fstr_t str = c3_malloc_permgen(handle, capacity + FSTR_BOOTLEN);
        fstr_len(str) = 0;
        FSTR_CAPACITY(str) = capacity + FSTR_BOOTLEN;
        return str;
}

fstr_t fstr_add_capacity(fstr_t str, char add) {
        str = c3_realloc(str, FSTR_CAPACITY(str) + FSTR_BOOTLEN + add);
        FSTR_CAPACITY(str) += add;
        return str;
}

int fstr_eq_cstr(fstr_t a, char *b_content) {
        unsigned char alen = fstr_len(a);
        unsigned char blen = 0;
        char *a_content = fstr_content(a);
        while (1) {
                if (alen == blen) {
                        return 1;
                }
                if (*b_content == '\0') {
                        return 0;
                }
                if (*a_content != *b_content) {
                        return 0;
                }
                a_content++;
                b_content++;
                blen++;
        }
}

fstr_t fstr_addch(fstr_t fstr, char ch) {
        if (FSTR_CAPACITY(fstr) == fstr_len(fstr)) {
                fstr = fstr_add_capacity((fstr_t)fstr, FSTR_CAPACITY(fstr));
        }
        fstr_content(fstr)[fstr_len(fstr)] = ch;
        fstr_content(fstr)[fstr_len(fstr) + 1] = '\0';
        fstr_len(fstr) += 1;
        return fstr;
}

fstr_t fstr_addch_permgen(fstr_t fstr, char ch) {
        if (FSTR_CAPACITY(fstr) == fstr_len(fstr)) {
                fstr = fstr_add_capacity((fstr_t)fstr, FSTR_CAPACITY(fstr));
        }
        fstr_content(fstr)[fstr_len(fstr)] = ch;
        fstr_len(fstr) += 1;
        return fstr;
}

int fstr_isempty(fstr_t fstr) {
        if (fstr_len(fstr) == 0) {
                return 1;
        } else {
                return 0;
        }
}

fstr_t fstr_addfstr(fstr_t fstr, fstr_t src) {
        int i = 0;
        while (i < fstr_len(src)) {
                fstr = fstr_addch(fstr, *(fstr_content(src) + i));
                i += 1;
        }

        return fstr;
}

fstr_t fstr_addcstr(fstr_t fstr, char *cstr) {
        while (*cstr != '\0') {
                fstr = fstr_addch(fstr, *cstr);
                cstr += 1;
        }

        return fstr;
}

void fstr_pop(fstr_t str) {
        if (fstr_len(str) != 0) {
                fstr_len(str) -= 1;
        }
}

fstr_t fstr_alloc(int capacity) {
        fstr_t str = c3_malloc(capacity + FSTR_BOOTLEN);
        fstr_len(str) = 0;
        FSTR_CAPACITY(str) = capacity;
        return str;
}

fstr_t itoa(size_t num) {
        fstr_t str = fstr_alloc(8);
        if (num < 0) {
                num = -num;
                fstr_addch(str, '-');
        }
        do {
                fstr_addch(str, num % 10 + 48);
                num /= 10;
        } while (num);

        int swappos = fstr_len(str);
        int i = 0;
        if (fstr_content(str)[0] == '-') {
                i = 1;
                swappos += 1;
        }
        for (; i < swappos / 2; i++) {
                char tmp;
                tmp = fstr_content(str)[i];
                fstr_content(str)[i] = fstr_content(str)[swappos - 1 - i];
                fstr_content(str)[swappos - 1 - i] = tmp;
        }
        return str;
}