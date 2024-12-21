#include "token.h"

REGISTER_LINKLIST_FUNCTIONS(Token, token, struct Token, token_free)

struct Token token_number_create(size_t n) {
        struct Token token;
        token.type = NUMBER;
        token.content = (void *)n;
        return token;
}

struct Token token_symbol_create(fstr_t sym) {
        struct Token token;
        token.type = SYMBOL;
        token.content = (void *)sym;
        return token;
}

struct Token token_string_create(fstr_t str) {
        struct Token token;
        token.type = STRING;
        token.content = (void *)str;
        return token;
}

struct Token token_list_create() {
        struct Token token;
        token.type = LIST;
        token.content = NULL;
        token.extra_content = NULL;
        return token;
}

struct Token token_list_create_with(struct TokenLinkList ll) {
        struct Token token;
        token.type = LIST;
        token.content = ll.head;
        token.extra_content = ll.last;
        return token;
}

struct TokenLinkList *token_list_aslinklist(struct Token *token) {
        return (struct TokenLinkList *)&token->content;
}

void token_list_push(struct Token *dest, struct Token src) {
        token_linklist_push(token_list_aslinklist(dest), src);
}

static void token_list_print(struct Token *token) {
        for_each_listnode(Token, token->content, { token_print(&_->value); })
}

void token_print(struct Token *token) {
        char *tmp;
        switch (token->type) {
        case NUMBER:
                printf("%zu ", (size_t)token->content);
                break;
        case SYMBOL:
                tmp = fstr_to_cstr((fstr_t)token->content);
                printf("%s ", tmp);
                c3_free(tmp);
                break;
        case LIST:
                printf("(");
                if (token->content != NULL) {
                        token_list_print(token);
                }
                printf(")");
                break;
        case ASM:
                printf("(asm ");
                if (token->content != NULL) {
                        token_list_print(token);
                }
                printf(")");
                break;
        case STRING:
                tmp = fstr_to_cstr((fstr_t)token->content);
                printf("\"%s\" ", tmp);
                c3_free(tmp);
                break;
        }
}

void token_linklist_print(struct TokenLinkList *linklist) {
        for_each_listnode(Token, linklist->head, { token_print(&_->value); })
}

static struct TokenListNode *token_list_aslistnode(struct Token *token) {
        return token->content;
}

static void token_symbol_free(struct Token token) { c3_free(token.content);}

static void token_string_free(struct Token token) { c3_free(token.content);}

static void token_list_free(struct Token token) {
        for_each_listnode(Token, token_list_aslistnode(&token), {
                switch (_->value.type) {
                case SYMBOL:
                        token_symbol_free(_->value);
                        break;
                case STRING:
                        token_string_free(_->value);
                        break;
                case LIST:
                        token_list_free(_->value);
                        break;
                default:
                        break;
                }
                token_listnode_free(_);
        })
}

void token_free(struct Token token) {
        switch (token.type) {
        case NUMBER:
                break;
        case SYMBOL:
                token_symbol_free(token);
                break;
        case LIST:
                token_list_free(token);
                break;
        case ASM:
                token_list_free(token);
                break;
        case STRING:
                token_string_free(token);
                break;
        }
}

void token_fstr_addch(struct Token *token, char ch) {
        token->content = fstr_addch(token->content, ch);
}

void token_fstr_addch_permgen(struct Token *token, char ch) {
        token->content = fstr_addch_permgen(token->content, ch);
}

int token_fstr_isempty(struct Token * token) {
        return fstr_isempty(token->content);
}

fstr_t token_symbol_asfstr(struct Token *token) {
        return token->content;
}

const char *tokentype_format(int type) {
        switch (type) {
        case NUMBER:
                return "Number";
        case SYMBOL:
                return "Symbol";
        case LIST:
                return "List";
        case STRING:
                return "String";
        case ASM:
                return "Asm";
        default:
                exit(1); // TODO
                //eturn exttnum_to_name(type);
        }
}