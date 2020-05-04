#ifndef __EVAL_H__
#define __EVAL_H__

#include <stdbool.h>

#include "internal_rep.h"
#include "environment.h"

struct s_obj *apply_function(struct s_obj *obj, 
    struct s_obj *arglist, struct s_env *env);

struct s_obj *eval(struct s_obj *obj, struct s_env *env, bool is_start);

#endif