#include <assert.h>

#include "common.h"
#include "lexer.h"

/* Grammar:
Cons -> QF SE
      | SE Cons
	  | SE . SE
      | Epsilon
SE -> ( Cons )
    | atom
QF -> quote | quasiquote | unquote | unquote-splice
*/

enum ast_class {
	AST_PROG, AST_SEXPR, AST_CONS, AST_QF, AST_ATOM
};

struct ast_node {
	int num_children;
	enum ast_class cls;
	struct ast_node **children;
	struct token *value;
};

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

struct ast_node *parse_tokens(struct tok_lst *tokens) {
	int position = 0;

	return NULL;
}

// This is the pushdown automata, with state transitions being the p_* functions
// A return value of NULL indicates a failed parse attempt. This shouldn't
// happen with the current grammar, and thus only occurs when there's user
// error and we want to report it

struct ast_node *p_sexpr(struct tok_lst *toks);
struct ast_node *p_cons(struct tok_lst *toks);

struct ast_node *p_cons(struct tok_lst *toks) {
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

		if(after_se->cls == TOK_CONS_DOT) { // Case 1
			advance_token_stream(toks);

			struct ast_node *right = p_sexpr(toks);
			ensure_exit(se != NULL, EX_DATAERR, 
				"Expected to get an s-expression, but did not get one.");

			struct ast_node *cons = make_cons_node(se, right);
			return cons;
		}
	}

	case TOK_VEC_OPEN: {
		log_err("Vectors are currently not supported. Please try again later");
		exit(1);
	}

	case TOK_WHITESPACE:
	case TOK_COMMENT:
	case TOK_CONS_DOT: 
	case TOK_PAREN_CLOSE: 
	case TOK_END_OF_FILE: {
		log_err("Unexpected token: ");
		print_token(cur);
		exit(1);	
	}
	}

	ensure_exit(true, EX_SOFTWARE, "Control should never reach here");
}

struct ast_node *p_sexpr(struct tok_lst *toks) {
	return NULL;
}