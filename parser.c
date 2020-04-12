#include <assert.h>

#include "common.h"
#include "lexer.h"
#include "parser.h"

/* Grammar:
Cons -> QF SE
      | SE Cons
      | SE . SE
      | Epsilon
SE -> ( Cons )
    | atom
QF -> quote | quasiquote | unquote | unquote-splice
*/

char *get_ast_class_str(enum ast_class cls) {
    switch(cls) {
        case AST_SEXPR: return "S_Expr";
        case AST_CONS: return "Cons";
        case AST_QF: return "Quote";
        case AST_ATOM: return "Atom";
        default: 
        log_err("Unknown AST class %d", cls);
        return "Unknown";
    }
}

struct ast_node *make_node(enum ast_class type, int num_children, 
    struct token *value) {
    assert(num_children >= 0);

    struct ast_node *node = malloc(sizeof(struct ast_node));

    node->cls = type;
    node->num_children = num_children;
    node->value = value;
    if(num_children > 0)
        node->children = calloc(num_children, sizeof(struct ast_node*));
    else
        node->children = NULL;

    return node;
}

void free_node(struct ast_node *node) {
    if(node->children != NULL)
        free(node->children);
    free(node);
}

struct ast_node *make_term_node(enum ast_class type, struct token *value) {
    return make_node(type, 0, value);
}

struct ast_node *make_cons_node(struct ast_node *left, struct ast_node *right) {
    struct ast_node *node = make_node(AST_CONS, 2, NULL);
    node->children[0] = left;
    node->children[1] = right;

    return node;
}

// Child MAY be NULL
void set_node_nth_child(int n, struct ast_node *node, struct ast_node *child) {
    assert(node != NULL);
    ensure_exit(n < node->num_children, EX_OSERR, "Array OOB");

    node->children[n] = child;
}

// This is the pushdown automata, with state transitions being the p_* functions
// A return value of NULL indicates a failed parse attempt. This shouldn't
// happen with the current grammar, and thus only occurs when there's user
// error and we want to report it

struct ast_node *p_sexpr(struct tok_lst *toks);
struct ast_node *p_cons(struct tok_lst *toks);

struct ast_node *p_cons(struct tok_lst *toks) {
    log("Parsing cons");

    struct token *cur = read_cur_tok(toks);
    switch(cur->cls) {

    // Deriv 1: quoted sexpr
    case TOK_QUOTE:
    case TOK_QUASIQUOTE:
    case TOK_UNQUOTE:
    case TOK_UNQUOTE_SPLICE: {
        struct ast_node *left = make_term_node(AST_QF, cur);
        advance_token_stream(toks);

        // We don't need to save state and potentially backtrack, since if
        // it fails to parse an sexpr here it means that we quoted something
        // invalid
        struct ast_node *right = p_sexpr(toks);
        ensure_exit(right != NULL, EX_DATAERR, "You can only quote an sexpr");

        struct ast_node *cons = make_cons_node(left, right);
        return cons;

    }

    // Deriv 2: either SE . SE or SE Cons
    case TOK_EMPTY_LIST:
    case TOK_BOOL_TRUE:
    case TOK_BOOL_FALSE:
    case TOK_NUMBER:
    case TOK_STRING:
    case TOK_PAREN_OPEN:
    case TOK_IDENTIFIER: {
        // Either case, begin with trying to get an sexpr
        struct ast_node *se = p_sexpr(toks);
        ensure_exit(se != NULL, EX_DATAERR, 
            "Expected to get an s-expression, but did not get one.");

        // If it's a cons dot, we're in the cons cell case(1), other wise we're
        // in the list case(2)
        struct token *after_se = read_cur_tok(toks);
        struct ast_node *right;

        if(after_se->cls == TOK_CONS_DOT) {
            // Case 1: SE |. SE
            advance_token_stream(toks);

            right = p_sexpr(toks);
            ensure_exit(se != NULL, EX_DATAERR, 
                "Expected to get an s-expression, but did not get one.");
        } else {
            // Case 2: SE |Cons
            // right can be null, it just means that we're at the 
            // end of the list
            right = p_cons(toks);
        }

        struct ast_node *cons = make_cons_node(se, right);
        return cons;
    }

    // End of list
    case TOK_PAREN_CLOSE: {
        // dont' advance token stream here, let s_expr deal with it
        //advance_token_stream(toks);
        return NULL;
    }

    // End of file
    case TOK_END_OF_FILE: {
        return NULL;
    }

    case TOK_VEC_OPEN: {
        log_err("Vectors are currently not supported.");
        exit(1);
    }

    case TOK_WHITESPACE:
    case TOK_COMMENT:
    case TOK_CONS_DOT: {
        log_err("Unexpected token: ");
        print_token(cur);
        exit(1);    
    }
    }

    ensure_exit(true, EX_SOFTWARE, "Control should never reach here");
}

struct ast_node *p_sexpr(struct tok_lst *toks) {
    log("Parsing sexpr");

    struct token *cur = read_cur_tok(toks);
    switch(cur->cls) {
    case TOK_PAREN_OPEN: {
        advance_token_stream(toks);
        struct ast_node *cons = p_cons(toks);

        // Ensure open is paired with a close paren
        cur = read_cur_tok(toks);
        ensure_exit(cur->cls == TOK_PAREN_CLOSE, EX_DATAERR,
            "Unexpected token class when parsing s-expression: %s",
            get_tokcls_name(cur->cls));
        advance_token_stream(toks);

        return cons;
    }

    case TOK_EMPTY_LIST:
    case TOK_BOOL_TRUE:
    case TOK_BOOL_FALSE:
    case TOK_NUMBER:
    case TOK_STRING:
    case TOK_IDENTIFIER: {
        struct ast_node *emptylist = make_term_node(AST_ATOM, cur);
        advance_token_stream(toks);
        return emptylist;
    }

    case TOK_WHITESPACE:
    case TOK_COMMENT:
    case TOK_VEC_OPEN:
    case TOK_PAREN_CLOSE:
    case TOK_QUOTE:
    case TOK_QUASIQUOTE:
    case TOK_UNQUOTE_SPLICE:
    case TOK_UNQUOTE:
    case TOK_CONS_DOT: {
        log_err("Unexpected token: ");
        print_token(cur);
        exit(1);
    }

    case TOK_END_OF_FILE: {
        return NULL;
    }
    }
    return NULL;
}

void print_node(struct ast_node *root, int indent_lv) {
    if(indent_lv != 0) {
        // indentation, 2 per lv
        printf("%*c", indent_lv*2, ' ');
    }

    if(root == NULL) {
        printf("Null: \n");
        return;
    }

    printf("%s: ", get_ast_class_str(root->cls));
    if(root->value == NULL)
        printf("\n");
    else
        print_token(root->value); // print_token adds newline

    for(int i=0; i<root->num_children; i++) {
        print_node(root->children[i], indent_lv+1);
    }
}

struct ast_node *parse_tokens(struct tok_lst *tokens) {
    struct ast_node *root = p_cons(tokens);
    return root;
}
