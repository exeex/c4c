/* driver.c – entry point for the C frontend (tiny-c2ll)
 *
 * Usage:
 *   tiny-c2ll [--version] [--lex-only] [-o output.ll] input.c
 *
 * M0/M1: preprocess + lex only; IR emission stubbed as TODO.
 */

#include "common.h"
#include "lexer.h"

#define VERSION "0.1.0-M1-lex"

/* ================================================================
 * Preprocess: run 'clang -E -P -x c <input>' and return the result
 * as a heap-allocated NUL-terminated string.  The caller frees it.
 * ================================================================ */
static char *preprocess(const char *clang_exec, const char *input_file,
                         size_t *out_len) {
    /* Build shell command.  We quote the file path with single quotes and
     * escape any embedded single quotes (unlikely in practice). */
    size_t cmd_cap = 256 + strlen(clang_exec) + strlen(input_file) * 4;
    char *cmd = (char *)xmalloc(cmd_cap);

    /* Escape single quotes in input_file: replace ' with '\'' */
    StrBuf escaped; sb_init(&escaped);
    for (const char *p = input_file; *p; p++) {
        if (*p == '\'') {
            sb_puts(&escaped, "'\\''");
        } else {
            sb_push(&escaped, (unsigned char)*p);
        }
    }
    char *esc_file = sb_str(&escaped);
    snprintf(cmd, cmd_cap,
             "%s -E -P -x c '%s' 2>/dev/null",
             clang_exec, esc_file);
    free(esc_file);

    FILE *fp = popen(cmd, "r");
    free(cmd);
    if (!fp) {
        fprintf(stderr, "error: failed to run preprocessor\n");
        exit(1);
    }

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
    const char *input_file = NULL;
    const char *output_file = NULL;
    bool lex_only = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            printf("tiny-c2ll %s (C frontend, M1 lexer)\n", VERSION);
            return 0;
        } else if (strcmp(argv[i], "--lex-only") == 0) {
            lex_only = true;
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
        fprintf(stderr, "usage: tiny-c2ll [--version] [--lex-only] [-o out.ll] input.c\n");
        return 1;
    }

    /* Suppress unused-variable warning when output_file not yet used */
    (void)output_file;

    /* ---- Preprocess ---- */
    const char *clang_exec = find_clang();
    size_t src_len = 0;
    char *src = preprocess(clang_exec, input_file, &src_len);

    /* ---- Lex ---- */
    TokenVec tv = lex_source(src, src_len);
    free(src);

    if (lex_only) {
        /* Print token stream for debugging/smoke-testing */
        for (size_t i = 0; i < tv.count; i++) {
            Token *t = &tv.tokens[i];
            if (t->kind == TK_STR) {
                /* Print string text safely: escape non-printable */
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

    /* ---- Future: parse → sema → IR emit ---- */
    fprintf(stderr, "TODO: full compilation not yet implemented (use --lex-only)\n");
    tokenvec_free(&tv);
    return 1;
}
