#include <stdbool.h>

enum tok_class {
    TOK_WHITESPACE = 0,
    TOK_COMMENT,
    TOK_EMPTY_LIST,
    TOK_BOOL_TRUE,
    TOK_BOOL_FALSE,
    TOK_NUMBER,
    TOK_STRING,
    TOK_VEC_OPEN,
    TOK_PAREN_OPEN,
    TOK_PAREN_CLOSE,
    TOK_QUOTE,
    TOK_QUASIQUOTE,
    TOK_UNQUOTE_SPLICE,
    TOK_UNQUOTE,
    TOK_CONS_DOT,
    TOK_IDENTIFIER,
    TOK_END_OF_FILE,
};

struct token {
    // NOT a null-terminated string, just a pointer to start of string
    const char *start_pos;
    int len;
    enum tok_class cls;
};

struct tok_lst;

char *get_tokcls_name(enum tok_class cls);

void compile_token_definitions();
void free_tok_lst(struct tok_lst *tokens);

struct tok_lst *tokenise_string(char const *input_str);

void print_token(struct token *tok);
void print_tokens(struct tok_lst *tokens);

// Token stream API

void advance_token_stream(struct tok_lst *tokens);
struct token *read_cur_tok(struct tok_lst *tokens);
struct token *peek_next_tok(struct tok_lst *tokens);
bool has_next_token(struct tok_lst *tokens);
int get_stream_pos(struct tok_lst *tokens);
void set_stream_pos(struct tok_lst *tokens, int n);
