#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "builtins.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "environment.h"

const char *prompt = "scheme> ";

long file_size(FILE *fp) {
	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}

void eval_file(char *path, struct s_env *env) {
	log("Evaluating file: %s", path);
	FILE *fp = fopen(path, "r");
	long size = file_size(fp);
	char *buf = calloc(1, size);
	fread(buf, 1, size, fp);
	fclose(fp);

	struct tok_lst *toks = tokenise_string(buf);
	struct s_obj *objs = parse_tokens(toks);
	free(buf);

	eval(objs, env, true);
}

int main(int argc, char **argv) {
    compile_token_definitions();

    int print_tokens_flag = false;
    int interactive_flag = false;
    int print_cst_flag = false;
    int verbose_flag = false;
    int help_flag = false;
    // char *input_file;

    struct option long_options[] = {
        {"interactive", no_argument, &interactive_flag, true},
        {"verbose", no_argument, &verbose_flag, true},
        {"tokens", no_argument, &print_tokens_flag, true},
        {"cst", no_argument, &print_cst_flag, true},
        {"help", no_argument, &help_flag, true},
    };

    int ch;
    while((ch = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        continue;
    }

    // Advance past parsed options
    argc -= optind;
    argv += optind;

    if(help_flag) {
    	printf("Usage: ./scheme <options> <input-file>\n");
    	printf("Options:\n");
    	printf("  --verbose: Verbose logging\n");
    	printf("  --tokens: Print lexer output\n");
    	printf("  --cst:    Print debug output of parser\n");
    	printf("\nIf you don't want to pass in an input file, use noin,"
    		" as in `./scheme noin`");
    	return EX_USAGE;
    }

    if(argc == 0) {
    	printf("Missing input file. Use --help for help.\n");
    	return EX_USAGE;
    }

    if(verbose_flag)      log("Verbose mode enabled.");
    if(print_tokens_flag) log("Printing lexer output");
    if(print_cst_flag)    log("Printing parser output");

    set_verbose(verbose_flag);

    // Initialise everything
    initialise_singleton_objects();
    struct s_env *root_env = get_root_env();
	add_builtins(root_env);

	// Add builtin functions written in scheme
	eval_file("builtins.scheme", root_env);

	// Evaluate input file
	if(strncmp(argv[0], "noin", 4) != 0) {
		eval_file(argv[0], root_env);
	}

	printf("\n\nWelcome to scheme. Use <C-d> when input is empty to exit.\n");

    // Start REPL
    while(true) {
        // Step 1: Read

        printf("%s", prompt);
        fflush(stdout);

        char *inbuf = NULL;
        size_t inbuf_len = 0;
        ssize_t linelen = getline(&inbuf, &inbuf_len, stdin);

        // Gracefully exit if user types <C-d>
        if(feof(stdin))
            break;

        ensure_exit(linelen >= 0, 1, "Failed to read input");

        // Step 2: Parse an eval
        struct tok_lst *tokens = tokenise_string(inbuf);
        if(tokens == NULL)
            continue;
        if(print_tokens_flag)
            print_tokens(tokens);

        struct s_obj *root_obj = parse_tokens(tokens);
        if(print_cst_flag)
            print_obj_debug(root_obj, 0);

        struct s_obj *eval_res = eval(root_obj, root_env, true);

        // Step 3: print output
        if(eval_res != NULL)
	        print_obj_user(eval_res);

        // Step 4: Loop (and cleanup)
        free_tok_lst(tokens);
    }

    printf("\nExiting scheme interpreter.\n");
    return 0;
}