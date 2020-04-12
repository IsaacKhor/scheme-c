#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "lexer.h"

struct tok_defn {
    char *pattern;
    enum tok_class cls;
    char *cls_name;
    int patflags;
    regex_t regex;
};

struct tok_lst {
    int capacity;
    int len;
    int position;
    struct token *arr;
};

// Grammar from: https://www.scheme.com/tspl2d/grammar.html
// HACK: List must be in same order as lexer.h class definitions enum
struct tok_defn definitions[] = {
    { "^[ \t\n\r\f\v]+", TOK_WHITESPACE, "whitespace", REG_EXTENDED, {} },
    { "^;.*$", TOK_COMMENT, "line comment", REG_EXTENDED | REG_NEWLINE, {} },
    { "^'()", TOK_EMPTY_LIST, "empty list", 0, {} },
    { "^#t", TOK_BOOL_TRUE, "true", 0, {} },
    { "^#f", TOK_BOOL_FALSE, "false", 0, {} },
    { "^[0-9]+", TOK_NUMBER, "number", REG_EXTENDED, {} },
    { 
        // A string is anything that is not '"', excluding '\"' and '\\',
        // surrounded by quotes. The longest match requirement will make it so
        // that we get the correct string in the case that there are multiple
        // escaped quotes
        .pattern = "^\"(\\\"|\\\\|[^\"\\])*\"", 
        .cls = TOK_STRING, 
        .cls_name = "string", 
        .patflags = REG_EXTENDED, 
        .regex = {} 
    },
    { "^#(", TOK_VEC_OPEN, "vector open", 0, {} },
    { "^(", TOK_PAREN_OPEN, "open paren", 0, {} },
    { "^)", TOK_PAREN_CLOSE, "close paren", 0, {} },
    { "^'", TOK_QUOTE, "quote", 0, {} },
    { "^`", TOK_QUASIQUOTE, "backquote", 0, {} },
    { "^,@", TOK_UNQUOTE_SPLICE, "list unquote", 0, {} },
    { "^,", TOK_UNQUOTE, "normal unquote", 0, {} },
    { "^\\. ", TOK_CONS_DOT, "cons dot", 0, {} },
    // The '-' doesn't need to be escaped since it is interpreted as literal
    // if first or last character of a character class in the POSIX
    // non-extended regex
    { 
        .pattern = "^[a-z!$%&*/:<=>?~_^+-][-+._a-z0-9]*", 
        .cls = TOK_IDENTIFIER, 
        .cls_name = "identifier", 
        .patflags = REG_ICASE, 
        .regex = {} 
    },
    { NULL, TOK_END_OF_FILE, "EOF", 0, {} },
};

char *get_tokcls_name(enum tok_class cls) {
    return definitions[cls].cls_name;
}

// DIRTY HACK: -1 to not include the EOF token
const int NUM_TOK_DEFNS = sizeof(definitions)/sizeof(struct tok_defn) - 1;

void compile_token_definitions() {
    for(int i=0; i< NUM_TOK_DEFNS; i++) {
        struct tok_defn *tdn = &definitions[i];
        int ret = regcomp(&tdn->regex, tdn->pattern, tdn->patflags);

        if(ret != 0) {
            char buf[8192];
            regerror(ret, &tdn->regex, buf, sizeof(buf));
            log_err("Failed to compile regex %s\nReason: %s\n", 
                tdn->pattern, buf);
            exit(1);
        }
    }
}

void add_token(struct tok_lst *ta, enum tok_class cls, 
    const char *const start_pos, int len) {

    if(ta->len == ta->capacity) {
        ta->arr = realloc(ta->arr, (int)(ta->capacity * 1.5));
        ensure_exit(ta->arr != NULL, 1, "Out of memory");
    } 

    struct token *tok = &ta->arr[ta->len];
    tok->cls = cls;
    tok->start_pos = start_pos;
    tok->len = len;

    ta->len++;
}

struct tok_lst *make_tok_lst(int init_capacity) {
    if(init_capacity < 16)
        init_capacity = 16;

    struct tok_lst *ta = malloc(sizeof(struct tok_lst));
    ta->capacity = init_capacity;
    ta->len = 0;
    ta->position = 0;
    ta->arr = calloc(init_capacity, sizeof(struct token));
    return ta;
}

void free_tok_lst(struct tok_lst *tokens) {
    free(tokens->arr);
    free(tokens);
}

/**
 * Given an input string, returns array of tokens. Assumes that the input string
 * is immutable, since the returned tokens will retain pointers into it.
 */
struct tok_lst *tokenise_string(char const *input_str) {
    int input_str_len = strlen(input_str);
    struct tok_lst *ta = make_tok_lst(input_str_len/4);
    char const *offset_str = input_str;

    for(int i=0; i<NUM_TOK_DEFNS; i++) {
        struct tok_defn tdn = definitions[i];

        // You're supposed to pass in an array, but an array of length one
        // is equivalent to just getting the address to the variable
        regmatch_t match;
        int ret = regexec(&tdn.regex, offset_str, 1, &match, 0);

        ensure_exit(ret != REG_ESPACE, 1, "Ran out of memory at %s",
            offset_str);

        // If not matched, try next token definition
        if(ret == REG_NOMATCH)
            continue;

        // We have matched a token
        // No -1 for len because str+rm_eo is actually 1 past the end of
        // the match for some strange reason
        int toklen = match.rm_eo - match.rm_so;

        // Ignore whitespace and comments
        if(tdn.cls != TOK_WHITESPACE && tdn.cls != TOK_COMMENT)
            add_token(ta, tdn.cls, offset_str, toklen);

        offset_str += toklen;

        // If offset goes past the end of the string, we're done with matching
        // everything. Since input_str+len is the address of the null
        // terminator, no further manipulation is needed
        if(offset_str >= input_str + input_str_len) {
            // Add EOF token
            add_token(ta, TOK_END_OF_FILE, offset_str, 0);
            return ta;
        }

        // Match another token by restarting loop
        // Set i to -1 because i++ will still run on next iteration
        i=-1;
    }

    // No token matched
    log_err("No token matched. String remaining: %s", offset_str);
    return NULL;
}

void print_token(struct token *tok) {
    assert(tok != NULL);
    printf("%s: %1.*s\n", 
        definitions[tok->cls].cls_name, tok->len, tok->start_pos);
}

void print_tokens(struct tok_lst *tokens) {
    assert(tokens != NULL);
    for(int i=0; i<tokens->len; i++) {
        struct token *tok = &tokens->arr[i];
        print_token(tok);
    }
}

bool has_next_token(struct tok_lst *tokens) {
    assert(tokens != NULL);
    int pos = tokens->position;
    if(pos + 1 >= tokens->len)
        return false; // Out of tokens
    return true;
}

struct token *read_cur_tok(struct tok_lst *tokens) {
    return &tokens->arr[tokens->position];
}

struct token *peek_next_tok(struct tok_lst *tokens) {
    assert(has_next_token(tokens));
    return &tokens->arr[tokens->position + 1];
}

void advance_token_stream(struct tok_lst *tokens) {
    assert(has_next_token(tokens));
    tokens->position++;
}
