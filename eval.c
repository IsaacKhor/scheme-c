#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#include "common.h"
#include "environment.h"
#include "eval.h"
#include "internal_rep.h"
#include "parser.h"

struct s_obj *apply_function(struct s_obj *obj, 
	struct s_obj *arglist, struct s_env *env) {

	int args_passed_in = get_list_len(arglist);
	// Pass up the error
	if(args_passed_in == -1) return NULL;

	// Check for arity and type
	int expected_args = 0;
	if(obj->type == OBJ_BUILTIN_FUNC) {
		expected_args = obj->val.builtin.num_args;
	} else if(obj->type == OBJ_LAMBDA) {
		expected_args = obj->val.lambda->num_args;
	} else {
		SET_ERR("Unexpected type in apply_function: %d", obj->type);
		return NULL;
	}

	if(expected_args != -1 && expected_args != args_passed_in) {
		SET_ERR("Arity mismatch: expected %d, got %d",
			expected_args, args_passed_in);
		return NULL;
	}

	// If it's a builtin function, let it handle itself
	if(obj->type == OBJ_BUILTIN_FUNC) {
		return obj->val.builtin.func(arglist, env);
	}

	// Lambdas neet to bind vars to now lexical scope before eval
	struct s_lambda *lambda = obj->val.lambda;
	struct s_env *local_scope = create_new_env(env);
	struct s_obj *cur = arglist;

	// Create new lexical context and bind variables
	if(expected_args == -1) {
		// In vararg, the sole argument *is* the list of args
		associate_symbol(local_scope, lambda->arglist[0], cur);
	} else {
		for(int i=0; i<lambda->num_args; i++) {
			associate_symbol(local_scope, lambda->arglist[i], cur->val.cc.left);
			cur = cur->val.cc.right;
		}
	}

	return eval(lambda->body, local_scope, true);
}

// Evaluate the given obj in the specified environment. The parameter
// is_start is used to denote if, in the case that the obj is a cons cell,
// if the cons cell is the beginning of the list and thus the left element
// should be evaluated and applied to the right element. A return value of
// NULL means that evaluation failed for some reason, retrieve the reason
// from get_fail_reason()
struct s_obj *eval(struct s_obj *obj, struct s_env *env, bool is_start) {

	if(is_start && get_verbose()) {
		printf("Evaluating: ");
		print_obj_user(obj);
	}

	// Nubers, strings, and booleans evaluate to themselves
	// Put empty list here because when we eval cons cells recursively
	// this is how we know we're at the end
	// Not sure to do with lambda, so gonna put it here for now
	if(obj->type == OBJ_NUMBER 
		|| obj->type == OBJ_STRING 
		|| obj->type == OBJ_BOOLEAN 
		|| obj->type == OBJ_LAMBDA
		|| obj->type == OBJ_EMPTY_LIST) {
		return obj;
	}

	// Lookup symbol in the symbol table
	if(obj->type == OBJ_SYMBOL) {
		if(!has_symbol(env, obj->val.sym.str, true)) {
			log_err("Unbound symbol: %s", obj->val.sym.str);
			return NULL;
		}

		return resolve_symbol(env, obj->val.sym.str, true);
	}

	// Cons cell / function evaluation
	ensure_exit(obj->type == OBJ_CONS, EX_SOFTWARE, 
		"Expected cons, but found %d", obj->type);

	struct s_obj *newleft = eval(obj->val.cc.left, env, true);
	if(newleft == NULL) return NULL;

	struct s_obj *oldright = obj->val.cc.right;
	if(oldright->type != OBJ_CONS && oldright->type != OBJ_EMPTY_LIST) {
		SET_ERR("Expecting cons during evaluation, found ");
		print_obj_user(oldright);
		return NULL;
	}

	// Not start of list, so just eval each comp and return new cons
	if(!is_start) {
		struct s_obj *newright = eval(obj->val.cc.right, env, false);
		if(newright == NULL) return NULL;
		return new_cons(newleft, newright);
	}

	// Is start of list, so apply LHS to RHS
	// Make sure LHS is a function
	if(newleft->type != OBJ_LAMBDA && newleft->type != OBJ_BUILTIN_FUNC) {
		log_err("Trying to treat non-function object as function:");
		print_obj_user(newleft);
		return NULL;
	}

	// If function is a macro, don't evaluate RHS
	bool is_macro = false;
	struct s_obj *newright = obj->val.cc.right;
	if(newleft->type == OBJ_LAMBDA)
		is_macro = newleft->val.lambda->is_macro;
	else
		is_macro = newleft->val.builtin.is_macro;

	if(!is_macro) {
		newright = eval(obj->val.cc.right, env, false);
		if(newright == NULL) return NULL;
	}

	// Make sure, if after eval, that we still have a list
	if(newright->type != OBJ_CONS && newright->type != OBJ_EMPTY_LIST) {
		SET_ERR("Cannot apply function to non-list item: ");
		print_obj_user(newright);
		return NULL;
	}

	// Finally actually do the evaluation
	return apply_function(newleft, newright, env);
}