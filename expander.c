#include "expander.h"
#include "linklist.h"
#include "token.h"

#include <stdlib.h>
#include <string.h>

#define argcheckexist(node, fnname)                                                    \
        do {                                                                           \
                if (node == NULL) {                                                    \
                        printf(fnname ": not enough args");                            \
                        exit(1);                                                       \
                }                                                                      \
        } while (0)
#define argchecktype(token, _type, fnname, argnum)                                     \
        do {                                                                           \
                if (token.type != _type) {                                             \
                        printf(fnname ": %cth argument needs type %s, not %s", argnum, \
                               tokentype_format(_type), tokentype_format(token.type)); \
                        exit(1);                                                       \
                }                                                                      \
        } while (0)

struct ExpanderContext *expander_context_alloc(mempool_handle_t permgen) {
        struct ExpanderContext *tmp = malloc(sizeof(struct ExpanderContext));
        tmp->asm_datasec = fstr_alloc_permgen(permgen, 2048);
        tmp->asm_extra = fstr_alloc_permgen(permgen, 256);
        tmp->asm_funcdef = fstr_alloc_permgen(permgen, 2048);
        tmp->asm_main = fstr_alloc_permgen(permgen, 1048);
        return tmp;
}

void expander_context_free(struct ExpanderContext *context) { free(context); }

void expand_all(struct ExpanderContext *context, struct TokenListNode *node) {
        start:
        expand(context, &node->value);
        if (node->next != NULL) {
                node = node->next;
                goto start;
        }
}

#define FUNC_BINDMAP_SIZE 32

struct FuncNode {
        char *key;
        enum FuncType type;
        void *content;
        struct FuncNode *next;
};
struct FuncNode *func_bindmap[FUNC_BINDMAP_SIZE];
static unsigned int func_bindmap_hash(fstr_t key) {
        unsigned int value = 0;
        for (int i = 0; i < fstr_len(key) - 1; i++) {
                value = value * 37 + fstr_content(key)[i];
        }
        return value % FUNC_BINDMAP_SIZE;
}

// Warning: 没有 free
void func_bindmap_insert(fstr_t key, enum FuncType type, void *content) {
        unsigned int index = func_bindmap_hash(key);
        struct FuncNode *node = malloc(sizeof(struct FuncNode));
        node->key = fstr_to_cstr(key);

        node->type = type;
        node->content = content;
        node->next = func_bindmap[index];
        func_bindmap[index] = node;
}

static struct FuncNode *func_bindmap_search(fstr_t key) {
        unsigned int index = func_bindmap_hash(key);
        struct FuncNode *temp = func_bindmap[index];
        while (temp != NULL) {
                if (fstr_eq_cstr(key, temp->key)) {
                        return temp;
                }
                temp = temp->next;
        }
        return NULL;
}

void func_bindmap_free() {
        for (int i = 0; i < FUNC_BINDMAP_SIZE; i++) {
                struct FuncNode *temp = func_bindmap[i];
                while (temp != NULL) {
                        struct FuncNode *prev = temp;
                        temp = temp->next;
                        c3_free(prev->key);
                        free(prev);
                }
        }
}

static struct Token process_function(struct ExpanderContext *context, struct Token *fn,
                                     struct TokenListNode *args_first,
                                     struct TokenListNode *args_last);
static struct Token process_function_symbol(struct ExpanderContext *context, struct Token *fn,
                                    struct TokenListNode *args_first,
                                    struct TokenListNode *args_last) {
        fstr_t fnname = token_symbol_asfstr(fn);
        struct FuncNode *func = func_bindmap_search(fnname);
        if (func == NULL) {
                printf("process_function: void function: %s\n", fstr_to_cstr(fnname));
                exit(1);
        }

        if (func->type == FUNC_BUILTIN) {
                struct Token ret = ((struct Token(*)(
                            struct ExpanderContext *, struct TokenListNode *,
                            struct TokenListNode *))func->content)(
                            context, args_first, args_last);
                token_free(*fn);
                return ret;
        } else if (func->type == FUNC_NORMAL) {
                struct Token ret = process_function(context, func->content,
                                               args_first, args_last);
                token_free(*fn);
                return ret;
        } else {
                // TODO: alias-func
                exit(1);
        }

}

#define LBDARG_BINDMAP_SIZE 32

struct LambdaBindMapNode {
        char *key;
        struct Token value;
        size_t used_flag;
        struct LambdaBindMapNode *next;
};
struct LambdaBindMapNode *lambda_bindmap[LBDARG_BINDMAP_SIZE];
static unsigned int lambda_bindmap_hash(fstr_t key) {
        unsigned int value = 0;
        for (int i = 0; i < fstr_len(key) - 1; i++) {
                value = value * 37 + fstr_content(key)[i];
        }
        return value % LBDARG_BINDMAP_SIZE;
}

// Warning: 没有 free
static void lambda_bindmap_insert(fstr_t key, struct Token value) {
        unsigned int index = lambda_bindmap_hash(key);
        struct LambdaBindMapNode *node = malloc(sizeof(struct LambdaBindMapNode));
        node->key = fstr_to_cstr(key);

        node->value = value;
        node->used_flag = 0;
        node->next = lambda_bindmap[index];
        lambda_bindmap[index] = node;
}

static struct LambdaBindMapNode *lambda_bindmap_search(fstr_t key) {
        unsigned int index = lambda_bindmap_hash(key);
        struct LambdaBindMapNode *temp = lambda_bindmap[index];
        while (temp != NULL) {
                if (fstr_eq_cstr(key, temp->key)) {
                        temp->used_flag = 1;
                        return temp;
                }
                temp = temp->next;
        }
        return NULL;
}

static void lambda_bindmap_free() {
        for (int i = 0; i < LBDARG_BINDMAP_SIZE; i++) {
                struct LambdaBindMapNode *temp = lambda_bindmap[i];
                while (temp != NULL) {
                        struct LambdaBindMapNode *prev = temp;
                        temp = temp->next;
                        c3_free(prev->key);
                        if (!prev->used_flag) {
                                token_free(prev->value);
                        }
                        free(prev);
                }
        }
}

void lambda_bindargs(struct TokenListNode *lambda_args,
                     struct TokenListNode *real_args) {
        if (lambda_args == NULL && real_args != NULL) {
                printf("lambda-call: lambda doesn't have any args are passed in");
                exit(1);
        }
        if (lambda_args != NULL && real_args == NULL) {
                printf(
                    "lambda-call: lambda has some args but none of args are passed in");
                exit(1);
        }
        if (lambda_args == NULL && real_args == NULL) {
                return;
        }

        memset(lambda_bindmap, 0, sizeof(lambda_bindmap));
        struct TokenListNode *tmp_l = lambda_args;
        struct TokenListNode *tmp_r = real_args;
        struct TokenListNode *prev_l;
        struct TokenListNode *prev_r;
        do {
                if (tmp_l->value.type != SYMBOL) {
                        printf("lambda: lambda args must be a symbol, not %s",
                               tokentype_format(tmp_l->value.type));
                        exit(1);
                }

                fstr_t tmp = token_symbol_asfstr(&tmp_l->value);
                lambda_bindmap_insert(tmp, tmp_r->value);

                prev_l = tmp_l;
                prev_r = tmp_r;
                tmp_l = tmp_l->next;
                tmp_r = tmp_r->next;
                token_listnode_free(prev_r);

                if (tmp_l == NULL && tmp_r != NULL) {
                        printf("lambda-call: not enough args");
                        exit(1);
                } else if (tmp_l != NULL && tmp_r == NULL) {
                        printf("lambda-call: too much args");
                        exit(1);
                }
        } while (tmp_l != NULL && tmp_r != NULL);
}

void lambda_replace_args(struct TokenListNode *lambdaexp) {
        for_each_listnode(Token, lambdaexp, {
                if (_->value.type == SYMBOL) {
                        struct LambdaBindMapNode *LambdaBindMapNode =
                            lambda_bindmap_search(token_symbol_asfstr(&_->value));
                        if (LambdaBindMapNode != NULL) {
                                token_free(_->value);
                                _->value = LambdaBindMapNode->value;
                        }
                } else if (_->value.type == LIST) {
                        lambda_replace_args(token_list_aslinklist(&_->value)->head);
                }
        });
}

static struct Token process_function_list(struct ExpanderContext *context,
                                          struct Token *fn,
                                          struct TokenListNode *args_first,
                                          struct TokenListNode *args_last) {
        struct TokenListNode *fn_firstnode = token_list_aslinklist(fn)->head;
        if (fn_firstnode->value.type == SYMBOL &&
            fstr_eq_cstr(token_symbol_asfstr(&fn_firstnode->value), "#")) {
                for_each_listnode(Token, args_first, { expand(context, &_->value); })

                argcheckexist(fn_firstnode->next, "lambda");
                struct Token lambda_args = fn_firstnode->next->value;
                argchecktype(lambda_args, LIST, "lambda", 1);

                argcheckexist(fn_firstnode->next->next, "lambda");
                struct Token lambda_exp = fn_firstnode->next->next->value;
                argchecktype(lambda_exp, LIST, "lambda", 2);

                lambda_bindargs(token_list_aslinklist(&lambda_args)->head, args_first);
                lambda_replace_args(token_list_aslinklist(&lambda_exp)->head);

                lambda_bindmap_free();

                token_print(&lambda_exp);
                expand(context, &lambda_exp);

                token_listnode_free(fn_firstnode);
                token_listnode_free(fn_firstnode->next);
                token_listnode_free(fn_firstnode->next->next);
                return lambda_exp;
        } else {
                printf("process_function: unsupport to use a list as a "
                       "function\n");
                exit(1);
        }
        return token_symbol_create(cstr_to_fstr("#err"));
}

static struct Token process_function(struct ExpanderContext *context, struct Token *fn,
                                     struct TokenListNode *args_first,
                                     struct TokenListNode *args_last) {
        switch (fn->type) {
        case SYMBOL:
                return process_function_symbol(context, fn, args_first, args_last);
        case LIST:
                return process_function_list(context, fn, args_first, args_last);
        default:
                break;
        }
        return token_symbol_create(cstr_to_fstr("#err"));
}

void expand(struct ExpanderContext *context, struct Token *token) {
        struct TokenListNode *fn;
        switch (token->type) {
        case SYMBOL:
                // todo
                break;
        case LIST:
                fn = token_list_aslinklist(token)->head;
                if (fn == NULL) { // empty list
                        break;
                }
                *token = process_function(context, &fn->value, fn->next,
                                          (struct TokenListNode *)token->extra_content);
                token_listnode_free(fn);
        default:
                break;
        }
}