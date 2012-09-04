#

#CFLAGS+=-Wall

#.PHONY: all clean

#all: genjsbind

#genjsbind: genjsbind.o genbind-parser.o genbind-lexer.o webidl-parser.o webidl-lexer.o
#	$(CC) -o $@ $^

#webidl-parser.o: webidl-parser.c webidl-parser.h webidl-lexer.h

#webidl-parser.h webidl-parser.c: webidl-parser.y
#	bison -t $<

#webidl-lexer.h: webidl-lexer.c

#webidl-lexer.c: webidl-lexer.l
#	flex $<


#genbind-parser.o: genbind-parser.c genbind-parser.h genbind-lexer.h

#genbind-parser.h genbind-parser.c: genbind-parser.y
#	bison -t $<

#genbind-lexer.h: genbind-lexer.c

#genbind-lexer.c: genbind-lexer.l
#	flex $<


#genjsbind.o: webidl-parser.h genbind-parser.h

#clean:
#	$(RM) genjsbind genjsbind.o webidl-parser.c webidl-lexer.c webidl-lexer.h webidl-parser.h genbind-parser.c genbind-lexer.c genbind-lexer.h genbind-parser.h *.o

CFLAGS+=-I$(BUILDDIR)

$(BUILDDIR)/%-lexer.c $(BUILDDIR)/%-lexer.h: src/%-lexer.l
	$(VQ)$(ECHO) "    FLEX: $<"
	$(Q)flex --outfile=$(BUILDDIR)/$(*F)-lexer.c --header-file=$(BUILDDIR)/$(*F)-lexer.h $<

$(BUILDDIR)/%-lexer.c: $(BUILDDIR)/%-parser.h

$(BUILDDIR)/%-parser.c $(BUILDDIR)/%-parser.h: src/%-parser.y
	$(VQ)$(ECHO) "   BISON: $<"
	$(Q)bison -d -t  --output=$(BUILDDIR)/$(*F)-parser.c --defines=$(BUILDDIR)/$(*F)-parser.h $<


# Sources in this directory
DIR_SOURCES := genjsbind.c 

SOURCES := $(SOURCES) $(BUILDDIR)/genbind-parser.c $(BUILDDIR)/genbind-lexer.c $(BUILDDIR)/webidl-parser.c $(BUILDDIR)/webidl-lexer.c

# Grab the core makefile
include $(NSBUILD)/Makefile.subdir
