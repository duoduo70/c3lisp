#include "fstr.h"
#include "linklist.h"

#ifndef _TOKEN_H_
#define _TOKEN_H_

enum TokenType { NUMBER = 0, SYMBOL = 1, LIST = 2, STRING = 3, ASM = 4 };

struct Token {
        int type;

        // type == LIST 会指向链表第一个元素
        // 否则实际上是 fstr_t 类型
        void *content;

        // 链表的“当前尾部”
        void *extra_content;
};

REGISTER_LINKLIST_STRUCTS(Token, struct Token)

struct Token token_number_create(size_t n);
struct Token token_symbol_create(fstr_t sym);
struct Token token_string_create(fstr_t str);
struct Token token_list_create();

void token_fstr_addch(struct Token *token, char ch);
void token_fstr_addch_permgen(struct Token *token, char ch);
/*
retcode:
        1: is empty
        0: is not empty
*/
int token_fstr_isempty(struct Token *);

fstr_t token_symbol_asfstr(struct Token *token);

void token_list_push(struct Token *dest, struct Token src);
struct TokenLinkList *token_list_aslinklist(struct Token *token);

void token_print(struct Token *);
void token_linklist_print(struct TokenLinkList *linklist);

void token_free(struct Token);

struct TokenLinkList token_linklist_create();
void token_linklist_push(struct TokenLinkList *linklist, struct Token value);

struct Token token_list_create_with(struct TokenLinkList);

void token_linklist_free(struct TokenLinkList linklist);

void token_listnode_free(struct TokenListNode *node);

const char *tokentype_format(int type);

#endif