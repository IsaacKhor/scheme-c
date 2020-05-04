#ifndef __INTERNAL_REP_H__
#define __INTERNAL_REP_H__

#include <stdint.h>
#include <stdbool.h>

#define SET_ERR(MSG, ...)           \
    do {                            \
        set_err_reason(             \
            "[ERR (%s:%d)] " MSG "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

void set_err_reason(char *reason, ...);

enum scheme_obj_type {
    OBJ_CONS = 0,
    OBJ_NUMBER,
    OBJ_STRING,
    OBJ_SYMBOL,
    OBJ_BOOLEAN,
    OBJ_LAMBDA,
    OBJ_BUILTIN_FUNC,
    OBJ_EMPTY_LIST,
};

struct s_obj;
struct s_cons_cell;
struct s_number;
struct s_string;
struct s_symbol;
struct s_lambda;

// non-symbol singleton objects
enum singleton_objects {
    SG_EMPTY_LIST,
    SG_TRUE,
    SG_FALSE,
};

struct s_cons_cell {
    struct s_obj *left;
    struct s_obj *right;
};

enum numeric_type { SCHEME_INT, SCHEME_FLOAT };
struct s_number {
    enum numeric_type type;
    union {
        int64_t integer;
        double floating;
    } value;
};

struct s_string {
    const char *str;
    int len;
};

// Scheme symbol names are IMMUTABLE
// To find out what a symbol evaluates to, refer to the environment
struct s_symbol {
    const char *str;
    int len;
};

struct s_lambda {
    bool is_macro;
    int num_args;
    char **arglist;
    struct s_obj *body;
    // Lexical scoping, so we base the evaluation on the env when the lambda
    // was created
    struct s_env *parent_env;
};

struct s_builtin {
    bool is_macro;
    int num_args;
    struct s_obj *(*func)(struct s_obj *arglist, struct s_env *env);
};

struct s_obj {
    enum scheme_obj_type type;
    union {
        struct s_cons_cell cc;
        struct s_number number;
        struct s_string str;
        struct s_symbol sym;
        struct s_lambda *lambda;
        struct s_builtin builtin;
        bool boolean;
    } val;
};

void set_verbose(bool vb);
bool get_verbose();

// Printing
void print_obj_debug(struct s_obj *obj, int indent);
void print_obj_user(struct s_obj *obj);
char *get_string_rep(struct s_obj *obj, char *buf, int buflen);

// List manipulation utilities
int get_list_len(struct s_obj *obj);
struct s_obj *get_list_nth(struct s_obj *obj, int n); // 1-indexed
struct s_obj *get_list_head(struct s_obj *obj);
struct s_obj *get_list_rest(struct s_obj *obj);

bool all_list_of_type(struct s_obj *obj, enum scheme_obj_type type);

// Object creation
struct s_obj *new_lambda(
    struct s_obj *arglist, 
    struct s_obj *body,
    struct s_env *parent_env);

struct s_obj *new_builtin(
    bool is_macro, int num_args,
    struct s_obj *(*func)(struct s_obj *, struct s_env *));

struct s_obj *new_cons(struct s_obj *left, struct s_obj *right);
struct s_obj *new_numeric(enum numeric_type type, long i, double f);
struct s_obj *new_string(int len, char *str);
struct s_obj *fetch_or_create_symbol(int len, const char *name);

// Singletons
void initialise_singleton_objects();
struct s_obj *fetch_singleton_object(enum singleton_objects sg);
struct s_obj *fetch_bool(bool b);

#endif