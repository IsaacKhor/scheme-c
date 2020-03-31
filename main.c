#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "lexer.h"
#include "common.h"

const char *prompt = "scheme> ";

int main(int argc, char **argv) {
	compile_token_definitions();

	int print_tokens_flag, print_cst_flag, print_ast_flag, verbose_flag = false;
	char *input_file;
	bool interactive_flag = false;

	struct option long_options[] = {
		{"verbose", no_argument, &verbose_flag, true},
		{"tokens", no_argument, &print_tokens_flag, true},
		{"cst", no_argument, &print_cst_flag, true},
		{"ast", no_argument, &print_ast_flag, true},
	};

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

		// Step 2: Eval
		struct tok_stream *tokens = tokenise_string(inbuf);
		if(tokens == NULL)
			continue;

		// Step 3: Print
		if(print_tokens_flag)
			print_tokens(tokens);

		// Step 4: Loop (and cleanup)
		free_tok_stream(tokens);
	}

	printf("\nExiting scheme interpreter.\n");
	return 0;
}