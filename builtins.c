#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "builtins.h"
#include "eval.h"

typedef struct s_obj sobj;
typedef struct s_env senv;

bool is_false(sobj *obj) {
	return obj->type == OBJ_BOOLEAN && obj->val.boolean == false;
}

struct s_obj *builtin_quote(struct s_obj *obj, struct s_env *env) {
	return get_list_nth(obj, 1);
}

// All non-emptylist and #f objects are true
struct s_obj *builtin_if(struct s_obj *obj, struct s_env *env) {
	struct s_obj *cond = get_list_nth(obj, 1);
	struct s_obj *iftrue = get_list_nth(obj, 2);
	struct s_obj *iffalse = get_list_nth(obj, 3);

	struct s_obj *res = eval(cond, env, true);

	if(is_false(res)) {
		return eval(iffalse, env, true);
	}

	return eval(iftrue, env, true);
}

struct s_obj *builtin_lambda(struct s_obj *obj, struct s_env *env);

struct s_obj *builtin_define(struct s_obj *obj, struct s_env *env) {
	struct s_obj *symobj = get_list_nth(obj, 1);
	struct s_obj *val = get_list_nth(obj, 2);

	// We're lucky and dealing with normal (define <symbol> <expr>)
	if(symobj->type == OBJ_SYMBOL) {
		struct s_obj *evaluated_val = eval(val, env, true);
		associate_symbol(env, symobj->val.sym.str, evaluated_val);
		return fetch_singleton_object(SG_EMPTY_LIST);
	}

	// EITHER (define (funcname arg1 arg2 arg3 ...) <body>)
	// OR     (define (funcname . varargs) <body>)
	if(symobj->type != OBJ_CONS) {
		SET_ERR("1st arg to define must be symbol on cons");
		return NULL;
	}

	sobj *fname = get_list_head(symobj);
	sobj *fargs = get_list_rest(symobj);
	sobj *fbody = val;
	sobj *lambda = NULL;

	// Case 1: (define (funcname arg1 arg2 arg3 ...) <body>)
	if(symobj->val.cc.right->type == OBJ_CONS) {
		if(!all_list_of_type(symobj, OBJ_SYMBOL)) {
			SET_ERR("Function arguments MUST be symbols");
			return NULL;
		}

		lambda = new_lambda(fargs, fbody, env);
	} else {
		// Case 2: (define (funcname . varargs) <body>)
		if(fname->type != OBJ_SYMBOL || fargs->type != OBJ_SYMBOL) {
			SET_ERR("Function arguments must be symbols");
			return NULL;
		}

		lambda = new_lambda(fargs, fbody, env);
	}

	if(lambda == NULL) return NULL;
	associate_symbol(env, fname->val.sym.str, lambda);
	return fetch_singleton_object(SG_EMPTY_LIST);

}

struct s_obj *builtin_set_bang(struct s_obj *obj, struct s_env *env) {
	// For now, no difference from define
	return builtin_define(obj, env);
}

struct s_obj *builtin_lambda(struct s_obj *obj, struct s_env *env) {
	struct s_obj *arglistobj = get_list_nth(obj, 1);
	struct s_obj *body = get_list_nth(obj, 2);

	if(get_list_len(arglistobj) == -1) {
		SET_ERR("Lambda arglist must be a list");
		return NULL;
	}

	return new_lambda(arglistobj, body, env);
}

struct s_obj *builtin_begin(struct s_obj *obj, struct s_env *env) {
	struct s_obj *head = get_list_head(obj);
	struct s_obj *rest = get_list_rest(obj);

	if(rest->type != OBJ_EMPTY_LIST)
		return builtin_begin(rest, env);

	return head;
}

sobj *builtin_write(sobj *obj, senv *env) {
	sobj *arg = get_list_head(obj);
	print_obj_user(arg);
	return fetch_singleton_object(SG_EMPTY_LIST);
}

sobj *builtin_eval(sobj *obj, senv *env) {
	sobj *arg = get_list_head(obj);
	return eval(arg, env, true);
}

sobj *builtin_apply(sobj *obj, senv *env) {
	sobj *func = get_list_nth(obj, 1);
	sobj *arglist = get_list_nth(obj, 2);

	if(get_list_len(arglist) == -1) {
		SET_ERR("Must apply function to a list");
		return NULL;
	}

	return apply_function(func, arglist, env);
}

struct s_obj *builtin_cons(struct s_obj *obj, struct s_env *env) {
	struct s_obj *car = get_list_nth(obj, 1);
	struct s_obj *cdr = get_list_nth(obj, 2);

	return new_cons(car, cdr);
}

struct s_obj *builtin_car(struct s_obj *obj, struct s_env *env) {
	struct s_obj *arg = get_list_head(obj);
	return arg->val.cc.left;
}

struct s_obj *builtin_cdr(struct s_obj *obj, struct s_env *env) {
	struct s_obj *arg = get_list_head(obj);
	return arg->val.cc.right;
}

struct s_obj *builtin_length(struct s_obj *obj, struct s_env *env) {
	sobj *arg = get_list_head(obj);
	int len = get_list_len(arg);
	return new_numeric(SCHEME_INT, len, 0);
}

sobj *builtin_list(sobj *obj, senv *env) {
	return obj;
}

struct s_obj *builtin_is_null(struct s_obj *obj, struct s_env *env) {
	struct s_obj *arg = get_list_head(obj);
	if(arg->type == OBJ_EMPTY_LIST)
		return fetch_singleton_object(SG_TRUE);

	return fetch_singleton_object(SG_FALSE);
}

bool elt_eq(sobj *obj1, sobj *obj2) {
	if(obj1->type != obj2->type)
		return false;

	switch(obj1->type) {
	case OBJ_CONS:
		return elt_eq(obj1->val.cc.left, obj2->val.cc.left) &&
			elt_eq(obj1->val.cc.right, obj2->val.cc.right);

	case OBJ_NUMBER:
		return obj1->val.number.value.integer == obj2->val.number.value.integer;

	case OBJ_STRING:
		return strncmp(obj1->val.str.str, obj2->val.str.str, 
			obj1->val.str.len) == 0;

	case OBJ_SYMBOL:
		return strncmp(obj1->val.sym.str, obj2->val.sym.str, 
			obj1->val.sym.len) == 0;

	case OBJ_BOOLEAN:
		return obj1->val.boolean == obj2->val.boolean;

	case OBJ_LAMBDA:
		// Just check if they're the exact same function pointer
		return obj1->val.lambda == obj2->val.lambda;	

	case OBJ_BUILTIN_FUNC:
		return obj1->val.builtin.func == obj2->val.builtin.func;

	case OBJ_EMPTY_LIST:
		return true;
	}
}

sobj *builtin_is_list(sobj *obj, senv *env) {
	sobj *x = get_list_nth(obj, 1);
	int len = get_list_len(x);
	return fetch_bool(len != -1);
}

sobj *builtin_is_number(sobj *obj, senv *env) {
	sobj *x = get_list_nth(obj, 1);
	return fetch_bool(x->type == OBJ_NUMBER);
}

struct s_obj *builtin_is_equal(sobj *obj, senv *env) {
	sobj *x = get_list_nth(obj, 1);
	sobj *y = get_list_nth(obj, 2);
	bool res = elt_eq(x, y);
	return fetch_bool(res);
}

sobj *builtin_is_func(sobj *obj, senv *env) {
	sobj *x = get_list_nth(obj, 1);
	return fetch_bool(x->type == OBJ_LAMBDA || x->type == OBJ_BUILTIN_FUNC);
}

sobj *builtin_not(sobj *obj, senv *env) {
	if(obj->type == OBJ_BOOLEAN && obj->val.boolean == false) {
		return fetch_bool(true);
	}
	return fetch_bool(false);
}

sobj *builtin_and(sobj *obj, senv *env) {
	if(obj->type == OBJ_EMPTY_LIST)
		return fetch_bool(false);

	sobj *head = get_list_head(obj);
	sobj *res = eval(head, env, true);

	if(is_false(res))
		return fetch_bool(false);

	return builtin_and(get_list_rest(obj), env);
}

sobj *builtin_or(sobj *obj, senv *env) {
	if(obj->type == OBJ_EMPTY_LIST)
		return fetch_bool(false);

	sobj *head = get_list_head(obj);
	sobj *res = eval(head, env, true);

	if(!is_false(res))
		return fetch_bool(true);

	return builtin_and(get_list_rest(obj), env);
}

struct s_obj *builtin_add(struct s_obj *obj, struct s_env *env) {
	if(obj->type == OBJ_EMPTY_LIST)
		return new_numeric(SCHEME_INT, 0, 0);

	struct s_obj *x = get_list_nth(obj, 1);

	if(x->type != OBJ_NUMBER) {
		SET_ERR("Arguments to add not numbers");
		return NULL;
	}

	sobj *rest_sum = builtin_add(get_list_rest(obj), env);

	int64_t xn = x->val.number.value.integer;
	int64_t yn = rest_sum->val.number.value.integer;
	int64_t sum = xn + yn;

	struct s_obj *sum_obj = new_numeric(SCHEME_INT, sum, 0);
	return sum_obj;
}

sobj *builtin_sub(sobj *obj, senv *env) {
	int len = get_list_len(obj);
	if(len < 1) {
		SET_ERR("Arity mismatch: sub expects at least 1 arg");
		return NULL;
	}

	struct s_obj *x = get_list_nth(obj, 1);
	if(x->type != OBJ_NUMBER) {
		SET_ERR("Arguments to sub not numbers");
		return NULL;
	}

	int64_t xn = x->val.number.value.integer;

	if(len == 1) {
		return new_numeric(SCHEME_INT, -1 * xn, 0);
	}

	sobj *rest_sum = builtin_add(get_list_rest(obj), env);
	int64_t yn = rest_sum->val.number.value.integer;
	int64_t diff = xn - yn;

	struct s_obj *sum_obj = new_numeric(SCHEME_INT, diff, 0);
	return sum_obj;
}

sobj *builtin_mul(sobj *obj, senv *env) {
	if(obj->type == OBJ_EMPTY_LIST)
		return new_numeric(SCHEME_INT, 1, 0);

	struct s_obj *x = get_list_nth(obj, 1);

	if(x->type != OBJ_NUMBER) {
		SET_ERR("Arguments to mul not numbers");
		return NULL;
	}

	sobj *rest_prod = builtin_mul(get_list_rest(obj), env);

	int64_t xn = x->val.number.value.integer;
	int64_t yn = rest_prod->val.number.value.integer;
	int64_t sum = xn * yn;

	struct s_obj *sum_obj = new_numeric(SCHEME_INT, sum, 0);
	return sum_obj;
}

sobj *builtin_cond(sobj *obj, senv *env) {
	sobj *clause = get_list_head(obj);

	sobj *test_cond = get_list_head(clause);
	sobj *bodies = get_list_rest(clause);

	if(test_cond->type == OBJ_SYMBOL && strncmp(test_cond->val.sym.str, "else", 4) == 0)
		goto cond_true;

	sobj *ev_cond = eval(test_cond, env, true);

	if(ev_cond->type == OBJ_BOOLEAN && ev_cond->val.boolean == false) {
		sobj *rest_clauses = get_list_rest(obj);
		if(rest_clauses->type == OBJ_EMPTY_LIST) {
			return fetch_singleton_object(SG_EMPTY_LIST);
		}

		return builtin_cond(rest_clauses, env);
	}

cond_true:;
	// Evaluate clause body and return. Because we may have multiple expressions
	// in the body, first wrap in begin
	sobj *beg = fetch_or_create_symbol(strlen("begin"), "begin");
	sobj *wrapped = new_cons(beg, bodies);
	return eval(wrapped, env, true);
}

void add_builtins(struct s_env *env) {

	// Fundamental special forms
	struct s_obj *quote_fn =    new_builtin(true, 1, &builtin_quote);
	struct s_obj *if_fn =       new_builtin(true, 3, &builtin_if);
	struct s_obj *define_fn =   new_builtin(true, 2, &builtin_define);
	struct s_obj *setbang_fn =  new_builtin(true, 2, &builtin_set_bang);
	struct s_obj *lambda_fn =   new_builtin(true, 2, &builtin_lambda);
	struct s_obj *begin_fn =    new_builtin(false, -1, &builtin_begin);
	struct s_obj *write_fn =    new_builtin(false, 1, &builtin_write);
	struct s_obj *eval_fn =     new_builtin(false, 1, &builtin_eval);
	struct s_obj *apply_fn =    new_builtin(false, 2, &builtin_apply);
	associate_symbol(env, "quote", quote_fn);
	associate_symbol(env, "if", if_fn);
	associate_symbol(env, "define", define_fn);
	associate_symbol(env, "set!", setbang_fn);
	associate_symbol(env, "lambda", lambda_fn);
	associate_symbol(env, "begin", begin_fn);
	associate_symbol(env, "write", write_fn);
	associate_symbol(env, "eval", eval_fn);
	associate_symbol(env, "apply", apply_fn);

	// List manipulation
	struct s_obj *cons_fn =     new_builtin(false, 2, &builtin_cons);
	struct s_obj *car_fn =      new_builtin(false, 1, &builtin_car);
	struct s_obj *cdr_fn =      new_builtin(false, 1, &builtin_cdr);
	struct s_obj *length_fn =   new_builtin(false, 1, &builtin_length);
	struct s_obj *list_fn =     new_builtin(false, -1, &builtin_list);
	associate_symbol(env, "cons", cons_fn);
	associate_symbol(env, "car", car_fn);
	associate_symbol(env, "cdr", cdr_fn);
	associate_symbol(env, "length", length_fn);
	associate_symbol(env, "list", list_fn);

	// Predicates
	struct s_obj *is_null_fn =  new_builtin(false, 1, &builtin_is_null);
	struct s_obj *is_list_fn =  new_builtin(false, 1, &builtin_is_list);
	struct s_obj *is_number_fn = new_builtin(false, 1, &builtin_is_number);
	struct s_obj *is_eq_fn =    new_builtin(false, 2, &builtin_is_equal);
	struct s_obj *is_func_fn =  new_builtin(false, 1, &builtin_is_func);
	associate_symbol(env, "null?", is_null_fn);	
	associate_symbol(env, "list?", is_list_fn);	
	associate_symbol(env, "number?", is_number_fn);	
	associate_symbol(env, "equal?", is_eq_fn);	
	associate_symbol(env, "procedure?", is_func_fn);	
	// Green wants function? instead of the R5RS procedure?, so we do both
	associate_symbol(env, "function?", is_func_fn);	
	// TODO: make '=' number-specific
	associate_symbol(env, "=", is_eq_fn);	

	// Arithmetic and boolean algebra
	struct s_obj *not_fn = new_builtin(false, 1, &builtin_not);
	struct s_obj *and_fn = new_builtin(true, -1, &builtin_and);
	struct s_obj *or_fn =  new_builtin(true, -1, &builtin_or);
	struct s_obj *add_fn = new_builtin(false, -1, &builtin_add);
	struct s_obj *sub_fn = new_builtin(false, -1, &builtin_sub);
	struct s_obj *mul_fn = new_builtin(false, -1, &builtin_mul);
	associate_symbol(env, "not", not_fn);
	associate_symbol(env, "and", and_fn);
	associate_symbol(env, "or", or_fn);
	associate_symbol(env, "+", add_fn);
	associate_symbol(env, "-", sub_fn);
	associate_symbol(env, "*", mul_fn);

	// Cond is here because my macro system sucks
	// TODO: rewrite cond as a macro
	struct s_obj *cond_fn = new_builtin(true, -1, &builtin_cond);
	associate_symbol(env, "cond", cond_fn);
}
