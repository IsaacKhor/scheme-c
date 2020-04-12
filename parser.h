#ifndef __PARSER_H__
#define __PARSER_H__

enum ast_class {
    AST_SEXPR, AST_CONS, AST_QF, AST_ATOM
};

struct ast_node {
    int num_children;
    enum ast_class cls;
    struct ast_node **children;
    struct token *value;
};

struct ast_node *parse_tokens(struct tok_lst *tokens);
void print_node(struct ast_node *root, int indent_lv);

#endif