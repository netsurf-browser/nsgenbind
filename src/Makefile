#

CFLAGS+=-Wall

.PHONY: all clean

all: genjsbind

genjsbind: genjsbind.o genbind-parser.o genbind-lexer.o webidl-parser.o webidl-lexer.o
	$(CC) -o $@ $^

webidl-parser.o: webidl-parser.c webidl-parser.h webidl-lexer.h

webidl-parser.h webidl-parser.c: webidl-parser.y
	bison -t $<

webidl-lexer.h: webidl-lexer.c

webidl-lexer.c: webidl-lexer.l
	flex $<


genbind-parser.o: genbind-parser.c genbind-parser.h genbind-lexer.h

genbind-parser.h genbind-parser.c: genbind-parser.y
	bison -t $<

genbind-lexer.h: genbind-lexer.c

genbind-lexer.c: genbind-lexer.l
	flex $<


genjsbind.o: webidl-parser.h genbind-parser.h

clean:
	$(RM) genjsbind genjsbind.o webidl-parser.c webidl-lexer.c webidl-lexer.h webidl-parser.h genbind-parser.c genbind-lexer.c genbind-lexer.h genbind-parser.h *.o
