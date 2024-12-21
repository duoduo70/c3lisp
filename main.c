// #define STB_LEAKCHECK_IMPLEMENTATION
// #include "leakcheck.h"

#include "parser.h"
#include "expander.h"
#include "builtin-func.h"
#include "token.h"

#include <stdio.h>

void asm_put(struct ExpanderContext *expander_context) {
        char *extra_cstr = fstr_to_cstr(expander_context->asm_extra);
        char *datasec_cstr = fstr_to_cstr(expander_context->asm_datasec);
        char *funcdef_cstr = fstr_to_cstr(expander_context->asm_funcdef);
        char *main_cstr = fstr_to_cstr(expander_context->asm_main);

        FILE *fp = fopen("test.asm", "w");
        fputs("[bits 64]\nglobal _start\n", fp);
        fputs(extra_cstr, fp);

        fputs("\nsection .text\n_start:\njmp main\n", fp);
        fputs(funcdef_cstr, fp);
        fputs("main:\n", fp);
        fputs(main_cstr, fp);

        fputs("section .data\n", fp);
        fputs(datasec_cstr, fp);
        fclose(fp);

        c3_free(extra_cstr);
        c3_free(datasec_cstr);
        c3_free(funcdef_cstr);
        c3_free(main_cstr);

        c3_free(expander_context->asm_extra);
        c3_free(expander_context->asm_datasec);
        c3_free(expander_context->asm_funcdef);
        c3_free(expander_context->asm_main);
}

int main() {
        // int i = 0;
        // while (1) {
        FILE *fp = fopen("test.c3", "r");
        struct TokenLinkList tmp = token_linklist_create();

        // TODO: 自动增加 permgen 大小
        mempool_handle_t permgen = c3_permgen_create(32768);

        struct ParserContext *parser_context = parser_context_alloc(fp, (char (*)(void *))fgetc, EOF, permgen);
        parser(parser_context, &tmp);
        free(parser_context);

        builtin_func_init(permgen);

        struct ExpanderContext *expander_context = expander_context_alloc(permgen);
        expand_all(expander_context, tmp.head);
        asm_put(expander_context);
        expander_context_free(expander_context);
        func_bindmap_free();
        fieldmap_free();

        //token_linklist_print(&tmp);
        token_linklist_free(tmp);
        free(permgen);
        fclose(fp);
        //printf("%u", i);
        // i++;
        // //stb_leakcheck_dumpmem();
        // }
        return 0;
}