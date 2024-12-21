#include "parser.h"
#include "fstr.h"
#include "token.h"

#include <stdio.h>
#include <stdlib.h>

struct ParserContext *parser_context_alloc(void *content, char (*content_getc)(void *),
                                           char content_end_ch,
                                           mempool_handle_t permgen) {
        struct ParserContext *tmp = malloc(sizeof(struct ParserContext));
        tmp->content = content;
        tmp->content_getc = content_getc;
        tmp->content_end_ch = content_end_ch;
        tmp->permgen = permgen;
        return tmp;
}

static void PROCESS_LAST_TOKEN(struct Token *atomtmp, struct TokenLinkList *tokens) {
        if (!token_fstr_isempty(atomtmp)) {
                if (atomtmp->type == SYMBOL &&
                    fstr_content(atomtmp->content)[0] >= '1' &&
                    fstr_content(atomtmp->content)[0] <= '9') {
                        atomtmp->type = NUMBER;
                        char *tmpnumstr = fstr_to_cstr(atomtmp->content);
                        c3_free(atomtmp->content);
                        size_t tmpnum = (size_t)atoi(tmpnumstr);
                        c3_free(tmpnumstr);
                        atomtmp->content = (void *)tmpnum;
                }
                token_linklist_push(tokens, *atomtmp);
        } else {
                token_free(*atomtmp);
        }
}

static void START_NEW_TOKEN(struct Token *atomtmp, struct ParserContext *context, struct TokenLinkList *tokens) {
        if (!token_fstr_isempty(atomtmp)) {
                if (atomtmp->type == SYMBOL &&
                    fstr_content(atomtmp->content)[0] >= '1' &&
                    fstr_content(atomtmp->content)[0] <= '9') {
                        atomtmp->type = NUMBER;
                        char *tmpnumstr = fstr_to_cstr(atomtmp->content);
                        c3_free(atomtmp->content);
                        size_t tmpnum = (size_t)atoi(tmpnumstr);
                        c3_free(tmpnumstr);
                        atomtmp->content = (void *)tmpnum;
                }
                token_linklist_push(tokens, *atomtmp);
                *atomtmp = token_symbol_create(fstr_alloc_permgen(context->permgen, 16));
        }
}

void parser(struct ParserContext *context, struct TokenLinkList *tokens) {
#define bool int
#define false 0
#define true 1
        struct TokenLinkList subtokens;

        bool in_comment_flag = false;
        bool in_string_flag = false;

        struct Token atomtmp =
            token_symbol_create(fstr_alloc_permgen(context->permgen, 16));

        while (1) {
                char ch = context->content_getc(context->content);
                if (ch == context->content_end_ch) {
                        PROCESS_LAST_TOKEN(&atomtmp, tokens);
                        printf("parser: saw EOF\n");
                        return;
                }

                if (in_comment_flag) {
                        if (ch == '\n') {
                                in_comment_flag = false;
                        }
                        continue;
                }
                if (in_string_flag) {
                        if (ch == '"') {
                                in_string_flag = false;
                        } else {
                                token_fstr_addch(&atomtmp, ch);
                        }
                        continue;
                }

                switch (ch) {
                case ';':
                        START_NEW_TOKEN(&atomtmp, context, tokens);
                        in_comment_flag = true;
                        break;
                case '"':
                        atomtmp.type = STRING;
                        in_string_flag = true;
                        break;

                case ' ':
                case '\n':
                case '\r':
                case '\t':
                        START_NEW_TOKEN(&atomtmp, context, tokens);
                        break;
                case '[': // TODO: 匹配同类型括号
                case '(':
                        subtokens = token_linklist_create();
                        parser(context, &subtokens);
                        struct Token sublist = token_list_create_with(subtokens);
                        token_linklist_push(tokens, sublist);
                        break;
                case ']':
                case ')':
                        PROCESS_LAST_TOKEN(&atomtmp, tokens);
                        return;
                default:
                        token_fstr_addch(&atomtmp, ch);
                }
        }
#undef bool
#undef false
#undef true
}