#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"
#include "internal_rep.h"
#include "eval.h"

void set_err_reason(char *reason, ...) {
	// For now, just print the stupid thing
	va_list args;
	va_start(args, reason);
	vprintf(reason, args);
	va_end(args);
}

bool g_verbose_flag = false;

void set_verbose(bool vb) {
	g_verbose_flag = vb;
}

bool get_verbose() {
	return g_verbose_flag;
}

void print_obj_debug(struct s_obj *obj, int indent) {
	// Print indentation
	printf("%*c", indent*2, ' ');

	switch(obj->type) {
	case OBJ_CONS:
		printf("CONS:\n");
		print_obj_debug(obj->val.cc.left, indent+1);
		print_obj_debug(obj->val.cc.right, indent+1);
		break;
	case OBJ_NUMBER:
		printf("NUMBER: ");
		if(obj->val.number.type == SCHEME_INT)
			printf("%lld", obj->val.number.value.integer);
		else
			printf("%f", obj->val.number.value.floating);
		printf("\n");
		break;
	case OBJ_STRING:
		printf("STRING: %.*s\n", obj->val.str.len, obj->val.str.str);
		break;
	case OBJ_SYMBOL:
		printf("SYMBOL: %.*s\n", obj->val.sym.len, obj->val.sym.str);
		break;
	case OBJ_BOOLEAN:
		if(obj->val.boolean)
			printf("BOOL: #t\n");
		else
			printf("BOOL: #f\n");
		break;
	case OBJ_LAMBDA:
		printf("#<function: arity %d @%p>\n", obj->val.lambda->num_args, 
			obj->val.lambda);
		break;
	case OBJ_BUILTIN_FUNC:
		printf("#<builtin-function %p>\n", obj->val.builtin.func);
		break;
	case OBJ_EMPTY_LIST:
		printf("<empty list>\n");
		break;
	}
}

void print_obj_user_util(struct s_obj *obj, bool is_start) {
	switch(obj->type) {
	case OBJ_CONS:
		if(is_start) printf("(");

		print_obj_user_util(obj->val.cc.left, true);

		if(obj->val.cc.right->type == OBJ_EMPTY_LIST) {
			printf("\b) ");
			return;
		}

		if(obj->val.cc.right->type != OBJ_CONS) {
			printf(". ");
			print_obj_user_util(obj->val.cc.right, false);
			printf("\b) ");
			return;
		}

		print_obj_user_util(obj->val.cc.right, false);
		break;

	case OBJ_NUMBER:
		printf("%lld ", obj->val.number.value.integer);
		break;

	case OBJ_STRING:
		printf("%s ", obj->val.str.str);
		break;

	case OBJ_SYMBOL:
		printf("%s ", obj->val.sym.str);
		break;

	case OBJ_BOOLEAN:
		if(obj->val.boolean)
			printf("#t ");
		else
			printf("#f ");
		break;

	case OBJ_LAMBDA:
		printf("#<procedure %p> ", obj->val.lambda);
		break;

	case OBJ_BUILTIN_FUNC:
		printf("#<builtin function %p> ", obj->val.builtin.func);
		break;

	case OBJ_EMPTY_LIST:
		printf("() ");
		break;
	}
}

void print_obj_user(struct s_obj *obj) {
	print_obj_user_util(obj, true);
	printf("\n");
}

char *get_string_rep(struct s_obj *obj, char *buf, int buflen) {
	printf("unimplemented\n");
	memset(buf, 0, buflen);
	return buf;
}

// Gets the length of a list. Returns -1 and sets error reason if the object
// is not a list
int get_list_len(struct s_obj *obj) {
	if(obj->type == OBJ_EMPTY_LIST)
		return 0;

	// Make sure we're taking the length of a list
	if(obj->type != OBJ_CONS) {
		// SET_ERR("Trying to get length of non-cons object");
		return -1;
	}

	int rest_len = get_list_len(obj->val.cc.right);
	if(rest_len == -1)
		return -1;

	return 1 + rest_len;
}

struct s_obj *get_list_head(struct s_obj *obj) {
	return obj->val.cc.left;
}

struct s_obj *get_list_rest(struct s_obj *obj) {
	return obj->val.cc.right;
}

struct s_obj *get_list_nth(struct s_obj *obj, int n) {
	int len = get_list_len(obj);

	if(n <= 0 || len < n) {
		SET_ERR("list has length %d, but n is %d", len, n);
		return NULL;
	}

	if(n == 1) return get_list_head(obj);
	return get_list_nth(get_list_rest(obj), n-1);
}

bool all_list_of_type(struct s_obj *obj, enum scheme_obj_type type) {
	int len = get_list_len(obj);
	if(len == -1) 
		return false;
	if(len == 0)
		return true;

	if(obj->val.cc.left->type != type)
		return false;

	return all_list_of_type(obj->val.cc.right, type);
}

struct s_obj *new_lambda(
    struct s_obj *arglist, 
    struct s_obj *body,
    struct s_env *parent_env) {

	int num_args = get_list_len(arglist);
	char **argnames = NULL;

	// Varargs case
	if(num_args == -1 && arglist->type == OBJ_SYMBOL) {
		argnames = calloc(1, sizeof(char *));
		ensure_mem(argnames);

		char *name = strdup(arglist->val.sym.str);
		ensure_mem(name);
		argnames[0] = name;
	} else {
		argnames = calloc(num_args, sizeof(char *));
		ensure_mem(argnames);

		// Get names of arguments
		for(int i=0; i<num_args; i++) {
			struct s_obj *no = get_list_nth(arglist, i+1);

			if(no->type != OBJ_SYMBOL) {
				SET_ERR("Lambda arguments must be symbols");
				return NULL;
			}

			char *name = strdup(no->val.sym.str);
			ensure_mem(name);
			argnames[i] = name;
		}
	}

	struct s_lambda *lambda = malloc(sizeof(struct s_lambda));
	ensure_mem(lambda);

	lambda->is_macro = false;
	lambda->num_args = num_args;
	lambda->arglist = argnames;
	lambda->body = body;
	lambda->parent_env = parent_env;

	struct s_obj *lamb_obj = malloc(sizeof(struct s_obj));
	ensure_mem(lamb_obj);

	lamb_obj->type = OBJ_LAMBDA;
	lamb_obj->val.lambda = lambda;

	return lamb_obj;
}

struct s_obj *new_builtin(
    bool is_macro, int num_args,
    struct s_obj *(*func)(struct s_obj *, struct s_env *)) {

	struct s_obj *obj = malloc(sizeof(struct s_obj));
	ensure_mem(obj);

	obj->type = OBJ_BUILTIN_FUNC;
	obj->val.builtin.is_macro = is_macro;
	obj->val.builtin.num_args = num_args;
	obj->val.builtin.func = func;

	return obj;
}

struct s_obj *new_cons(struct s_obj *left, struct s_obj *right) {
	struct s_obj *obj = malloc(sizeof(struct s_obj));
	ensure_mem(obj);

	obj->type = OBJ_CONS;
	obj->val.cc.left = left;
	obj->val.cc.right = right;
	return obj;
}

struct s_obj *new_numeric(enum numeric_type type, long i, double f){
	// Floats are not implemented yet
	assert(type != SCHEME_FLOAT);
	struct s_obj *obj = malloc(sizeof(struct s_obj));
	ensure_mem(obj);

	obj->type = OBJ_NUMBER;
	obj->val.number.type = SCHEME_INT;
	obj->val.number.value.integer = i;
	return obj;
}

struct s_obj *new_string(int len, char *str) {
	struct s_obj *obj = malloc(sizeof(struct s_obj));
	ensure_mem(obj);

	char *newstr = calloc(len+1, sizeof(char));
	strncpy(newstr, str, len);
	newstr[len] = '\0';

	obj->type = OBJ_STRING;
	obj->val.str.len = len;
	obj->val.str.str = newstr;
	return obj;
}

struct s_obj *fetch_or_create_symbol(int len, const char *name) {
	// Don't bother de-duplicating symbols for now
	struct s_obj *obj = malloc(sizeof(struct s_obj));
	ensure_mem(obj);

	char *newstr = calloc(len+1, sizeof(char));
	strncpy(newstr, name, len);
	newstr[len] = '\0';

	obj->type = OBJ_SYMBOL;
	obj->val.sym.len = len;
	obj->val.sym.str = newstr;
	return obj;
}

static bool initialised_singletons = false;

static struct s_obj *singleton_true = NULL;
static struct s_obj *singleton_false = NULL;
static struct s_obj *singleton_emptylist = NULL;

void initialise_singleton_objects() {
	if(initialised_singletons)
		return;

	singleton_true = malloc(sizeof(struct s_obj));
	singleton_false = malloc(sizeof(struct s_obj));
	singleton_emptylist = malloc(sizeof(struct s_obj));
	ensure_mem(singleton_true);
	ensure_mem(singleton_false);
	ensure_mem(singleton_emptylist);

	singleton_true->type = OBJ_BOOLEAN;
	singleton_true->val.boolean = true;

	singleton_false->type = OBJ_BOOLEAN;
	singleton_false->val.boolean = false;

	singleton_emptylist->type = OBJ_EMPTY_LIST;
	// no need to add value here, since it doesn't matter

	initialised_singletons = true;
}

struct s_obj *fetch_singleton_object(enum singleton_objects sg) {
	assert(initialised_singletons == true);

	switch(sg) {
	case SG_TRUE: return singleton_true;
	case SG_FALSE: return singleton_false;
	case SG_EMPTY_LIST: return singleton_emptylist;
	}
}

struct s_obj *fetch_bool(bool b) {
	if(b) return fetch_singleton_object(SG_TRUE);
	else return fetch_singleton_object(SG_FALSE);
}
