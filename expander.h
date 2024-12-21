#ifndef _EXPANDER_H_
#define _EXPANDER_H_

#include "fstr.h"
#include "token.h"

struct ExpanderContext {
        fstr_t asm_extra;
        fstr_t asm_datasec;
        fstr_t asm_funcdef;
        fstr_t asm_main;
};

struct ExpanderContext *expander_context_alloc(mempool_handle_t permgen);
void expander_context_free(struct ExpanderContext *context);

void expand_all(struct ExpanderContext *context, struct TokenListNode *node);

void expand(struct ExpanderContext *context, struct Token *token);

enum FuncType { FUNC_BUILTIN, FUNC_NORMAL };

void func_bindmap_insert(fstr_t key, enum FuncType type, void *content);

void func_bindmap_free();

#endif