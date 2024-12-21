#include "expander.h"
#include "linklist.h"
#include "token.h"
#include <stdio.h>
#include <string.h>

#define argcheckexist(node, fnname)                                            \
        do {                                                                   \
                if (node == NULL) {                                            \
                        printf(fnname ": not enough args");                    \
                        exit(1);                                               \
                }                                                              \
        } while (0)
#define argchecktype(token, _type, fnname, argnum)                             \
        do {                                                                   \
                if (token.type != _type) {                                     \
                        printf(fnname ": %cth argument needs type %s, not %s", \
                               argnum, tokentype_format(_type),                \
                               tokentype_format(token.type));                  \
                        exit(1);                                               \
                }                                                              \
        } while (0)

static void args_free(struct TokenListNode *args_first,
                      struct TokenListNode *args_last) {
        struct TokenLinkList tmp;
        tmp.head = args_first;
        tmp.last = args_last;
        token_linklist_free(tmp);
}

static struct Token process_expander_log(struct ExpanderContext *context,
                                         struct TokenListNode *args_first,
                                         struct TokenListNode *args_last) {
        printf("expander-log: ");
        struct TokenListNode *tmp = args_first;
        for_each_listnode(Token, tmp, {
                expand(context, &_->value);
                token_print(&_->value);
        });
        printf("\n");
        args_free(args_first, args_last);
        return token_symbol_create(cstr_to_fstr("#nil"));
}

#define FUNCARG_MAP_LEN 32

static int rsp_offset_max = 0;
struct FuncArgNode {
        char *key;
        int rsp_reoffset;
        struct FuncArgNode *next;
};

static struct FuncArgNode *funcargmap[FUNCARG_MAP_LEN];

static unsigned int funcargmap_hash(fstr_t key) {
        unsigned int value = 0;
        for (int i = 0; i < fstr_len(key) - 1; i++) {
                value = value * 37 + fstr_content(key)[i];
        }
        return value % FUNCARG_MAP_LEN;
}

// Warning: 没有 free
static void funcargmap_insert(fstr_t key, int len) {
        unsigned int index = funcargmap_hash(key);
        struct FuncArgNode *node = malloc(sizeof(struct FuncArgNode));
        node->key = fstr_to_cstr(key);
        node->rsp_reoffset = len;
        node->next = funcargmap[index];
        funcargmap[index] = node;
}

static struct FuncArgNode *funcargmap_search(fstr_t key) {
        unsigned int index = funcargmap_hash(key);
        struct FuncArgNode *temp = funcargmap[index];
        while (temp != NULL) {
                if (fstr_eq_cstr(key, temp->key)) {
                        return temp;
                }
                temp = temp->next;
        }
        return NULL;
}

static void funcarg_reset() {
        rsp_offset_max = 0;
        for (int i = 0; i < FUNCARG_MAP_LEN; i++) {
                struct FuncArgNode *temp = funcargmap[i];
                while (temp != NULL) {
                        struct FuncArgNode *prev = temp;
                        temp = temp->next;
                        c3_free(prev->key);
                        free(prev);
                }
        }
}

static void process_operand_symbol(struct ExpanderContext *context,
                                   fstr_t opcode,
                                   struct TokenListNode *operand) {
        fstr_t symbol_fstr = token_symbol_asfstr(&operand->value);
        struct FuncArgNode *node = funcargmap_search(symbol_fstr);
        if (node != NULL) {
                fstr_t offset_tmp = itoa(rsp_offset_max);

                fstr_addcstr(context->asm_funcdef, "[rsp+");
                fstr_addfstr(context->asm_funcdef, offset_tmp);
                fstr_addch(context->asm_funcdef, ']');

                rsp_offset_max -= node->rsp_reoffset;
                c3_free(offset_tmp);
        } else {
                fstr_addfstr(context->asm_funcdef, symbol_fstr);
        }
}

static void process_real_asm_operand(struct ExpanderContext *context,
                                     fstr_t opcode,
                                     struct TokenListNode *operands) {
        fstr_addfstr(context->asm_funcdef, opcode);
        fstr_addcstr(context->asm_funcdef, " ");
        for_each_listnode(Token, operands, {
                if (_->value.type == NUMBER) {
                        fstr_t num = itoa((size_t)_->value.content);
                        fstr_addfstr(context->asm_funcdef, num);
                        c3_free(num);
                } else if (_->value.type == SYMBOL) {
                        process_operand_symbol(context, opcode, _);
                } else {
                        printf("asm: unsupport type: %s",
                               tokentype_format(_->value.type));
                        exit(1);
                }

                fstr_addch(context->asm_funcdef, ',');
        });
        fstr_pop(context->asm_funcdef);
}

// TODO: hashtablize
static void compile_op(struct ExpanderContext *context, fstr_t opcode,
                       struct TokenListNode *operand) {
        if (fstr_eq_cstr(opcode, "arg")) {
                for_each_listnode(Token, operand, {
                        funcargmap_insert(token_symbol_asfstr(&_->value),
                                          sizeof(size_t));
                        rsp_offset_max += sizeof(size_t);
                });
        } else {
                process_real_asm_operand(context, opcode, operand);
        }
}

static struct Token process_asm(struct ExpanderContext *context,
                                struct TokenListNode *args_first,
                                struct TokenListNode *args_last) {
        memset(funcargmap, '\0', sizeof(funcargmap));
        struct TokenListNode *arg = args_first;
        for_each_listnode(Token, arg, {
                struct Token *op = &_->value;
                struct TokenListNode *op_list = token_list_aslinklist(op)->head;
                compile_op(context, token_symbol_asfstr(&op_list->value),
                           op_list->next);
                fstr_addch(context->asm_funcdef, '\n');
        });
        funcarg_reset();
        args_free(args_first, args_last);
        return token_symbol_create(cstr_to_fstr("#nil"));
}

#define FIELD_MAP_LEN 128

struct FieldNode {
        char *key;
        int value;
        struct FieldNode *next;
};

static struct FieldNode *fieldmap[FIELD_MAP_LEN];

static unsigned int fieldmap_hash(fstr_t key) {
        unsigned int value = 0;
        for (int i = 0; i < fstr_len(key) - 1; i++) {
                value = value * 37 + fstr_content(key)[i];
        }
        return value % FIELD_MAP_LEN;
}

static int counter_num = 0;
static int counter() {
        counter_num += 1;
        return counter_num;
}

// Warning: 没有 free
static int fieldmap_insert(fstr_t key) {
        unsigned int index = fieldmap_hash(key);
        struct FieldNode *node = malloc(sizeof(struct FieldNode));
        node->key = fstr_to_cstr(key);
        int count = counter();
        node->value = count;
        node->next = fieldmap[index];
        fieldmap[index] = node;
        return count;
}

static struct FieldNode *fieldmap_search(fstr_t key) {
        unsigned int index = fieldmap_hash(key);
        struct FieldNode *temp = fieldmap[index];
        while (temp != NULL) {
                if (fstr_eq_cstr(key, temp->key)) {
                        return temp;
                }
                temp = temp->next;
        }
        return NULL;
}

void fieldmap_free() {
        for (int i = 0; i < FIELD_MAP_LEN; i++) {
                struct FieldNode *temp = fieldmap[i];
                while (temp != NULL) {
                        struct FieldNode *prev = temp;
                        temp = temp->next;
                        c3_free(prev->key);
                        free(prev);
                }
        }
}

static void put_field_name(struct ExpanderContext *context,
                           struct TokenListNode *arg) {
        argcheckexist(arg, "set^");

        expand(context, &arg->value);
        argchecktype(arg->value, SYMBOL, "set^", 1);

        fstr_t fieldname = token_symbol_asfstr(&arg->value);
        fstr_t field_fstr = itoa(fieldmap_insert(fieldname));

        fstr_addch(context->asm_funcdef, 'f');
        fstr_addfstr(context->asm_funcdef, field_fstr);
        fstr_addch(context->asm_funcdef, ':');
        c3_free(field_fstr);
}

#define fstr_last(str) fstr_content(str)[fstr_len(str) - 1]

static int can_put_to_asm(int tokentype) {
        if (tokentype == NUMBER || tokentype == STRING) {
                return 1;
        } else {
                return 0;
        }
}

static void put_value_to_datasec(struct ExpanderContext *context,
                                 struct Token token) {
        fstr_addcstr(context->asm_datasec, "db ");
        if (token.type == NUMBER) {
                fstr_t tmp = itoa((size_t)token.content);
                fstr_addfstr(context->asm_datasec, tmp);
                free(tmp);
        } else if (token.type == STRING) {
                fstr_addch(context->asm_datasec, '"');
                fstr_addfstr(context->asm_datasec, token_symbol_asfstr(&token));
                fstr_addch(context->asm_datasec, '"');
        } else {
                printf("Unsupport type: %s", tokentype_format(token.type));
        }
        fstr_addch(context->asm_datasec, '\n');
}

static void putasm_from_list(struct ExpanderContext *context,
                             struct TokenListNode *arg) {
        int db_flag = 1;
        for_each_listnode(Token, arg, {
                expand(context, &arg->value);
                if (can_put_to_asm(arg->value.type)) {
                        if (db_flag) {
                                fstr_addcstr(context->asm_datasec, "\ndb ");
                                db_flag = 0;
                        }

                        put_value_to_datasec(context, arg->value);

                        fstr_addch(context->asm_datasec, ',');
                } else {
                        // skip
                }
        });

        if (fstr_last(context->asm_datasec) == ',') {
                fstr_pop(context->asm_datasec);
        }
}

static struct Token process_set_hyper(struct ExpanderContext *context,
                                      struct TokenListNode *args_first,
                                      struct TokenListNode *args_last) {
        put_field_name(context, args_first);

        struct TokenListNode *arg = args_first->next;
        putasm_from_list(context, arg);

        args_free(args_first, args_last);
        return token_symbol_create(cstr_to_fstr("#nil"));
}

static void push_stack(struct ExpanderContext *context,
                       struct TokenListNode *arg) {
        for_each_listnode(Token, arg, {
                expand(context, &_->value);
                if (_->value.type == STRING) {
                        fstr_t tmp = itoa(counter());

                        fstr_addch(context->asm_datasec, 'f');
                        fstr_addfstr(context->asm_datasec, tmp);
                        fstr_addch(context->asm_datasec, ':');
                        put_value_to_datasec(context, _->value);

                        fstr_addcstr(context->asm_main, "push f");
                        fstr_addfstr(context->asm_main, tmp);
                        fstr_addch(context->asm_main, '\n');

                        c3_free(tmp);
                } else if (_->value.type == NUMBER) {
                        fstr_addcstr(context->asm_main, "push ");
                        fstr_t tmp = itoa((size_t)_->value.content);
                        fstr_addfstr(context->asm_main, tmp);
                        c3_free(tmp);
                        fstr_addch(context->asm_main, '\n');
                } else {
                        printf("\n\n\n");
                        token_print(&_->value);
                        printf("push-stack: unsupoort type: %s\n",
                               tokentype_format(_->value.type));
                }
        });
}

static void call_field(struct ExpanderContext *context, fstr_t fnname) {
        struct FieldNode *fieldnode = fieldmap_search(fnname);
        if (fieldnode == NULL) {
                printf("call^: function name \"%s\" is not exists",
                       fstr_to_cstr(fnname));
        }
        fstr_t fieldnum_fstr = itoa(fieldnode->value);
        fstr_addcstr(context->asm_main, "call f");
        fstr_addfstr(context->asm_main, fieldnum_fstr);
        fstr_addch(context->asm_main, '\n');
        c3_free(fieldnum_fstr);
}

static struct Token process_call_hyper(struct ExpanderContext *context,
                                       struct TokenListNode *args_first,
                                       struct TokenListNode *args_last) {
        argcheckexist(args_first, "call^");
        expand(context, &args_first->value);
        argchecktype(args_first->value, SYMBOL, "call^", 1);

        push_stack(context, args_first->next);

        fstr_t fnname = token_symbol_asfstr(&args_first->value);
        call_field(context, fnname);

        args_free(args_first, args_last);

        return token_symbol_create(cstr_to_fstr("#nil"));
}

#define argcheck_not_exist(node, fnname)                                       \
        do {                                                                   \
                if (node != NULL) {                                            \
                        printf(fnname ": too many args");                      \
                        exit(1);                                               \
                }                                                              \
        } while (0)

// TODO: token_number_asnum(token)
// token_string_asfstr
struct Token process_len(struct ExpanderContext *context,
                         struct TokenListNode *args_first,
                         struct TokenListNode *args_last) {
        argcheckexist(args_first, "set!");
        expand(context, &args_first->value);
        argcheck_not_exist(args_first->next, "len");

        argchecktype(args_first->value, STRING, "len", 1);
        token_listnode_free(args_first);
        return token_number_create(
            fstr_len(token_symbol_asfstr(&args_first->value)));
}

struct Token process_set_bang(struct ExpanderContext *context,
                              struct TokenListNode *args_first,
                              struct TokenListNode *args_last) {
        argcheckexist(args_first, "set!");
        expand(context, &args_first->value);
        argchecktype(args_first->value, SYMBOL, "set!", 1);

        argcheckexist(args_first->next, "set!");
        argcheck_not_exist(args_first->next->next, "set!");

        fstr_t aliasname = token_symbol_asfstr(&args_first->value);

        func_bindmap_insert(aliasname, FUNC_NORMAL, &args_first->next->value);
        token_print(&args_first->next->value);

        c3_free(args_first->next);
        //c3_free(aliasname);
        //token_listnode_free(args_first);
        return token_symbol_create(cstr_to_fstr("#nil"));
}

void builtin_func_init(mempool_handle_t handle) {
#define REGFUNC(funcname, lispname)                                            \
        do {                                                                   \
                fstr_t funcname_##funcname =                                   \
                    cstr_to_fstr_permgen(handle, lispname);                    \
                func_bindmap_insert(funcname_##funcname, FUNC_BUILTIN,         \
                                    funcname);                                 \
        } while (0)

        REGFUNC(process_expander_log, "expander-log");
        REGFUNC(process_asm, "asm");
        REGFUNC(process_set_hyper, "set^");
        REGFUNC(process_call_hyper, "call^");
        REGFUNC(process_len, "len");
        REGFUNC(process_set_bang, "set!");
#undef REGFUNC
}