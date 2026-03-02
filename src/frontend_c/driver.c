/* driver.c – entry point for the C frontend (tiny-c2ll)
 *
 * Usage:
 *   tiny-c2ll [--version] [--lex-only] [--parse-only] [-o output.ll] input.c
 *
 * M0/M1: preprocess + lex.
 * M2:    preprocess + lex + parse (AST dump via --parse-only).
 * M3+:   full IR emission (TODO).
 */

#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"

#define VERSION "0.1.0-M2-parse"

/* ================================================================
 * Preprocess: run 'clang -E -P -x c <input>' and capture output.
 * Returns a heap-allocated NUL-terminated string; caller frees it.
 * ================================================================ */
static char *preprocess(const char *clang_exec, const char *input_file,
                         size_t *out_len) {
    size_t cmd_cap = 256 + strlen(clang_exec) + strlen(input_file) * 4;
    char *cmd = (char *)xmalloc(cmd_cap);

    StrBuf escaped; sb_init(&escaped);
    for (const char *q = input_file; *q; q++) {
        if (*q == '\'') sb_puts(&escaped, "'\\''");
        else            sb_push(&escaped, (unsigned char)*q);
    }
    char *esc_file = sb_str(&escaped);
    snprintf(cmd, cmd_cap, "%s -E -P -x c '%s' 2>/dev/null", clang_exec, esc_file);
    free(esc_file);

    FILE *fp = popen(cmd, "r");
    free(cmd);
    if (!fp) { fprintf(stderr, "error: failed to run preprocessor\n"); exit(1); }

    StrBuf buf; sb_init(&buf);
    int ch;
    while ((ch = fgetc(fp)) != EOF) sb_push(&buf, ch);
    int status = pclose(fp);
    if (status != 0) {
        fprintf(stderr, "error: preprocessor exited with non-zero status\n");
        exit(1);
    }
    return sb_finish(&buf, out_len);
}

/* ================================================================
 * Find clang: honour $CLANG env, otherwise use PATH
 * ================================================================ */
static const char *find_clang(void) {
    const char *env = getenv("CLANG");
    if (env && env[0]) return env;
    return "clang";
}

/* ================================================================
 * main
 * ================================================================ */
int main(int argc, char **argv) {
    const char *input_file  = NULL;
    const char *output_file = NULL;
    bool lex_only   = false;
    bool parse_only = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("tiny-c2ll %s (C frontend, M2 parser)\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--lex-only") == 0) {
            lex_only = true;
        } else if (strcmp(argv[i], "--parse-only") == 0) {
            parse_only = true;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            if (input_file) {
                fprintf(stderr, "error: multiple input files not supported\n");
                return 1;
            }
            input_file = argv[i];
        } else {
            fprintf(stderr, "warning: unknown option '%s'\n", argv[i]);
        }
    }

    if (!input_file) {
        fprintf(stderr,
                "usage: tiny-c2ll [--version] [--lex-only] [--parse-only] "
                "[-o out.ll] input.c\n");
        return 1;
    }

    (void)output_file;   /* used in M3+ */

    /* ---- Preprocess ---- */
    const char *clang_exec = find_clang();
    size_t src_len = 0;
    char *src = preprocess(clang_exec, input_file, &src_len);

    /* ---- Lex ---- */
    TokenVec tv = lex_source(src, src_len);
    free(src);

    if (lex_only) {
        for (size_t i = 0; i < tv.count; i++) {
            Token *t = &tv.tokens[i];
            if (t->kind == TK_STR) {
                printf("%d:%d %-12s \"", t->line, t->col, tok_kind_str(t->kind));
                for (size_t j = 0; j < t->text_len; j++) {
                    unsigned char b = (unsigned char)t->text[j];
                    if (b < 0x20 || b == 0x22 || b == 0x5C || b > 0x7E)
                        printf("\\%02X", b);
                    else
                        putchar(b);
                }
                printf("\"\n");
            } else {
                printf("%d:%d %-12s %s\n",
                       t->line, t->col, tok_kind_str(t->kind), t->text);
            }
        }
        tokenvec_free(&tv);
        return 0;
    }

    /* ---- Parse ---- */
    Arena arena;
    arena_init(&arena, 256 * 1024);   /* 256 KB initial block */

    Node *prog = parse(&tv, &arena);
    tokenvec_free(&tv);

    if (parse_only) {
        ast_dump(prog, 0);
        arena_free_all(&arena);
        return 0;
    }

    /* ---- Future: sema → IR emit ---- */
    (void)prog;
    fprintf(stderr, "TODO: full compilation not yet implemented "
                    "(use --lex-only or --parse-only)\n");
    arena_free_all(&arena);
    return 1;
}
