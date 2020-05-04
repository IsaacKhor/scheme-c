CFLAGS = -Wall -Wextra -g -std=gnu11 -O0
INCLUDES = -I.
LIBS = -lc -lpthread
FLAGS =

.PHONY: clean zip

scheme: main.c builtins.c environment.c eval.c internal_rep.c lexer.c parser.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) $(FLAGS)

zip:
	zip cs170-scheme.zip *.c *.h *.scheme Makefile

clean:
	rm -f scheme cs170-scheme.zip
