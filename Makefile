CFLAGS = -Wall -Wextra -g -std=gnu11 -O0
INCLUDES = -I.
LIBS = -lc -lpthread
FLAGS =

.PHONY: clean

scheme: main.c lexer.c parser.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) $(FLAGS)

clean:
	rm -f scheme
