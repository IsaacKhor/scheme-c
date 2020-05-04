#ifndef __PARSER_H__
#define __PARSER_H__

#include "internal_rep.h"
#include "lexer.h"

struct s_obj *parse_tokens(struct tok_lst *tokens);

#endif