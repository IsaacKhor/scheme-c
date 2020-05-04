#include "uthash.h"
#include <stdio.h>

struct s_env_kp {
	// Key
	char *name;
	int val;
	UT_hash_handle hh;
};

struct s_env_kp *map = NULL;

int main() {
	char *foo1 = "foo";
	char *foo2 = strdup(foo1);

	struct s_env_kp *kp = malloc(sizeof(struct s_env_kp));
	kp->name = foo1;
	kp->val = 1;
	HASH_ADD_KEYPTR(hh, map, kp->name, strlen(foo1), kp);
	printf("foo1: %p\n", kp);

	struct s_env_kp *kp2 = NULL;
	HASH_FIND_STR(map, foo2, kp2);
	printf("%p", kp2);
}