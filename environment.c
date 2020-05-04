#include <assert.h>

#include "common.h"
#include "environment.h"
#include "uthash.h"

struct s_env {
	struct s_env *parent;
	struct s_env_kp *map;
};

struct s_env_kp {
	// Key
	char *name;
	struct s_obj *value;
	UT_hash_handle hh;
};

bool root_env_initialised = false;
struct s_env *root_env = NULL;

void init_root() {
	root_env = malloc(sizeof(struct s_env));
	ensure_mem(root_env);

	root_env->parent = NULL;
	// Must start off as null according to docs
	root_env->map = NULL;

	root_env_initialised = true;
}

struct s_env *get_root_env() {
	if(!root_env_initialised)
		init_root();

	return root_env;
}

struct s_env *get_parent_env(struct s_env *env) {
	assert(env != NULL);
	return env->parent;
}

bool has_symbol(struct s_env *env, const char *sym, bool traverse) {
	assert(sym != NULL);

	// Not in root env
	if(env == NULL) return false;

	struct s_env_kp *kp;
	HASH_FIND_STR(env->map, sym, kp);

	// If we find it, we're done
	if(kp != NULL) return true;

	// We didn't find it here, if not looking in parent we're also done
	if(!traverse) return false;

	return has_symbol(env->parent, sym, true);
}

struct s_obj *resolve_symbol(struct s_env *env, const char *sym, bool traverse) {
	assert(sym != NULL);

	// Not in root env
	if(env == NULL) return NULL;

	struct s_env_kp *kp = NULL;
	HASH_FIND_STR(env->map, sym, kp);

	if(kp != NULL) return kp->value;
	if(!traverse) return NULL;
	return resolve_symbol(env->parent, sym, true);
}

void associate_symbol(struct s_env *env, const char *sym, struct s_obj *obj) {
	assert(env != NULL && sym != NULL);

	if(has_symbol(env, sym, false))
		remove_symbol(env, sym);

	struct s_env_kp *kp = malloc(sizeof(struct s_env_kp));
	ensure_mem(kp);

	kp->name = strdup(sym);
	kp->value = obj;

	HASH_ADD_KEYPTR(hh, env->map, kp->name, strlen(sym), kp);
}

void remove_symbol(struct s_env *env, const char *sym) {
	assert(env != NULL && sym != NULL);

	struct s_env_kp *kp = NULL;
	HASH_FIND_STR(env->map, sym, kp);

	if(kp == NULL)
		return;

	HASH_DEL(env->map, kp);
	free(kp);
}

struct s_env *create_new_env(struct s_env *parent) {
	assert(parent != NULL);

	struct s_env *env = malloc(sizeof(struct s_env));
	env->parent = parent;
	env->map = NULL;
	return env;
}
