#include <assert.h>
#include <string.h>

#include "common.h"
#include "internal_rep.h"
#include "lexer.h"
#include "parser.h"

struct s_obj *tok_to_obj(struct token *tok) {
	switch(tok->cls) {
	case TOK_EMPTY_LIST:
		return fetch_singleton_object(SG_EMPTY_LIST);
	case TOK_BOOL_TRUE:
		return fetch_singleton_object(SG_TRUE);
	case TOK_BOOL_FALSE:
		return fetch_singleton_object(SG_FALSE);
	case TOK_NUMBER: {
		// floats are currently unimplemented
		char buf[tok->len+1];
		strncpy(buf, tok->start_pos, tok->len);
		buf[tok->len] = '\0';
		long num = atol(buf);
		return new_numeric(SCHEME_INT, num, 0);
	}
	case TOK_STRING:
	case TOK_QUOTE:
		return fetch_or_create_symbol(strlen("quote"), "quote");
	case TOK_QUASIQUOTE:
		return fetch_or_create_symbol(strlen("quasiquote"), "quasiquote");
	case TOK_UNQUOTE_SPLICE:
		return fetch_or_create_symbol(strlen("unquote-splice"), "unquote-splice");
	case TOK_UNQUOTE:
		return fetch_or_create_symbol(strlen("unquote"), "unquote");
	case TOK_IDENTIFIER:
		return fetch_or_create_symbol(tok->len, tok->start_pos);
	default:
		exit_msg(EX_SOFTWARE, "Cannot convert token class %d to scheme obj",
			tok->cls);
	}
}

// =========================== PUSHDOWN AUTOMATA =============================
// These two functions constitute a two-state deterministic pushdown automata,
// so there is no backtracking and parsing is done in linear time. Instead of 
// returnig a parse tree, however, it directly converts the tokens into
// the internal object representation of the runtime, which is possible
// due to homoiconicity, where code *is* scheme data. By design, there
// is no parse tree, only the scheme representation of the input code.
// ===========================================================================

/* Grammar:
Cons -> SE Cons
      | SE . SE
      | Epsilon
SE -> QF SE
    | ( Cons )
    | atom
QF -> quote | quasiquote | unquote | unquote-splice
*/

struct s_obj *p_sexpr(struct tok_lst *toks);
struct s_obj *p_cons(struct tok_lst *toks);

struct s_obj *p_cons(struct tok_lst *toks) {
    struct token *cur = read_cur_tok(toks);
    switch(cur->cls) {

    // Deriv 1: quoted sexpr
    case TOK_QUOTE:
    case TOK_QUASIQUOTE:
    case TOK_UNQUOTE:
    case TOK_UNQUOTE_SPLICE:
    case TOK_EMPTY_LIST:
    case TOK_BOOL_TRUE:
    case TOK_BOOL_FALSE:
    case TOK_NUMBER:
    case TOK_STRING:
    case TOK_PAREN_OPEN:
    case TOK_IDENTIFIER: {
        // Either case, begin with trying to get an sexpr
        struct s_obj *se = p_sexpr(toks);
        ensure_exit(se != NULL, EX_DATAERR, 
            "Expected to get an s-expression, but did not get one.");

        // If it's a cons dot, we're in the cons cell case(1), other wise we're
        // in the list case(2)
        struct token *after_se = read_cur_tok(toks);
        struct s_obj *right;

        if(after_se->cls == TOK_CONS_DOT) {
            // Case 1: SE |. SE
            advance_token_stream(toks);

            right = p_sexpr(toks);
            ensure_exit(se != NULL, EX_DATAERR, 
                "Expected to get an s-expression, but did not get one.");
        } else {
            // Case 2: SE |Cons
            right = p_cons(toks);
        }

        struct s_obj *cons = new_cons(se, right);
        return cons;
    }

    // End of list
    case TOK_PAREN_CLOSE: {
        // dont' advance token stream here, let s_expr deal with it
        //advance_token_stream(toks);
        return fetch_singleton_object(SG_EMPTY_LIST);
    }

    // End of file
    case TOK_END_OF_FILE: {
    	// debug("Hit EOF");
        return fetch_singleton_object(SG_EMPTY_LIST);
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

struct s_obj *p_sexpr(struct tok_lst *toks) {
    struct token *cur = read_cur_tok(toks);
    switch(cur->cls) {

    // Deriv 1: quoted sexpr
    case TOK_QUOTE:
    case TOK_QUASIQUOTE:
    case TOK_UNQUOTE:
    case TOK_UNQUOTE_SPLICE: {
        struct s_obj *left = tok_to_obj(cur);
        advance_token_stream(toks);

        // We don't need to save state and potentially backtrack, since if
        // it fails to parse an sexpr here it means that we quoted something
        // invalid
        struct s_obj *right = p_sexpr(toks);
        ensure_exit(right != NULL, EX_DATAERR, "You can only quote an sexpr");

        struct s_obj *quote_arglist = new_cons(right, 
        	fetch_singleton_object(SG_EMPTY_LIST));
        struct s_obj *cons = new_cons(left, quote_arglist);
        return cons;

    }

    case TOK_PAREN_OPEN: {
        advance_token_stream(toks);
        struct s_obj *cons = p_cons(toks);

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
        struct s_obj *emptylist = tok_to_obj(cur);
        advance_token_stream(toks);
        return emptylist;
    }

    case TOK_WHITESPACE:
    case TOK_COMMENT:
    case TOK_VEC_OPEN:
    case TOK_PAREN_CLOSE:
    case TOK_CONS_DOT:
    case TOK_END_OF_FILE: {
        log_err("Unexpected token: ");
        print_token(cur);
        exit(1);
    }
    }
}

struct s_obj *parse_tokens(struct tok_lst *tokens) {
    struct s_obj *lst = p_cons(tokens);

    // Add implicit begin
    struct s_obj *beg = fetch_or_create_symbol(strlen("begin"), "begin");
    struct s_obj *root = new_cons(beg, lst);

    return root;
}
