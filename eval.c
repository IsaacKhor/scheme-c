#include <inttypes.h>

enum scheme_obj_type {
	OBJ_CONS,
	OBJ_NUMBER,
	OBJ_STRING,
	OBJ_SYMBOL,
	OBJ_LIST,
};

struct s_obj;
struct s_cons_cell;
struct s_number;
struct s_string;
struct s_symbol;

struct s_cons_cell {
	struct s_obj *left;
	struct s_obj *right;
};

enum numeric_type { INTEGER, FLOAT };
struct s_number {
	enum numeric_type type;
	union {
		int64_t integer;
		double floating;
	} value;
};

struct s_string {
	// NOT null-terminated
	char *value;
	int len;
};

// Scheme symbol names are IMMUTABLE
// To find out what a symbol evaluates to, refer to the environment
struct s_symbol {
	// NOT null-terminated
	const char *const name;
	int name_len;
};

struct s_obj {
	enum scheme_obj_type type;
	union {
		struct s_cons_cell cc;
		struct s_number number;
		struct s_string str;
		struct s_symbol sym;
	} val;
};