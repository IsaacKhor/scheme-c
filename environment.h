#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include <stdbool.h>

#include "internal_rep.h"

struct s_env;

// Get the parent envionment
struct s_env *get_parent_env(struct s_env *env);

// Checks if the environment has the symbol
bool has_symbol(struct s_env *env, const char *sym, bool traverse);

// Resolve the symbol. If traverse is true, automatically search parent
// environment as well. sym MUST have type s_symbol
struct s_obj *resolve_symbol(struct s_env *env, 
    const char *sym, bool traverse);

// Associate symbol with object.
void associate_symbol(struct s_env *env, 
    const char *sym, struct s_obj *obj);

// Remove symbol from environment. Returns previous association if any.
void remove_symbol(struct s_env *env, const char *sym);

// Creates a new environment
struct s_env *create_new_env(struct s_env *parent);

// Get the root environment with all default symbols
struct s_env *get_root_env();

#endif
