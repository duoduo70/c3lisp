#ifndef _PARSER_H_
#define _PARSER_H_

#include "token.h"

struct ParserContext {
        // 我们使用这样的方式是为了让其从各种抽象源中解析
        // 而不需要先将其完整读取为字符串
        // 因为 lisp 的语法是无回溯的
        void *content;                // Example: FILE*
        char (*content_getc)(void *); // Example: fgetc(FILE*)
        char content_end_ch;
        mempool_handle_t permgen;
};

struct ParserContext *parser_context_alloc(void *content, char (*content_getc)(void *),
                                           char content_end_ch, mempool_handle_t permgen);

void parser(struct ParserContext *context, struct TokenLinkList *tokens);
#endif