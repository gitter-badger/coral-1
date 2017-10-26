CC=clang++-5.0
SHELL:=/bin/bash
COMPILE=${CC} -Wall -std=c++11 -c -o $@ $<

.PHONY: build test clean

## SECTION auto-dependency generation
DEPFILES=$(shell find obj -name '*.o.d')

-include ${DEPFILES}

.PRECIOUS: obj/%.o.d

## Whenever a cc file is updated, its mk is updated
obj/%.o.d: src/%.cc
	@mkdir -p $(shell dirname $@)
	@set -e; \
	set -o pipefail; \
	OUT=$$(${CC} -Wall -std=c++11 -MM $< | sed 's/\\//g'); \
	echo $$(dirname $@)/$${OUT} > $@; \
	echo $$'\t' '$(value COMPILE)' >> $@; \
	exit 0

## If we build a .o, we need its .d
## If the .cc was updated, we re-generate the .d and then rebuild
obj/%.o: obj/%.o.d
	@CC=${CC} ${MAKE} --no-print-directory -r -f $<

build: obj/type.o obj/main.o
	@true

test: bin/run-test
	bin/run-test

src/parsing/generated/parser.hh: src/parsing/parser.yy
	bison -d -o src/parsing/generated/parser.cc $<

src/parsing/generated/parser.cc: src/parsing/parser.yy
	bison -d -o $@ $<

src/parsing/generated/lexer.cc: src/parsing/lexer.l src/parsing/generated/parser.hh
	@mkdir -p $(shell dirname $@)
	flex -o $@ $<

bin/run-test: obj/expr.o obj/type.o obj/tests/expr.o obj/tests/type.o obj/tests/main.o
	${CC} -o $@ $^

clean:
	rm -r bin obj src/parsing/generated
	mkdir bin
	mkdir obj
	mkdir -p src/parsing/generated

## sub-projects
.PHONY: core parsing

core: bin/coral-core

parsing: bin/coral-parse

# Core includes code that other submodules are allowed to depend on
COREFILES=obj/core/expr.o obj/core/type.o obj/core/treeprinter.o
bin/coral-core: ${COREFILES} obj/core/__main__.o
	${CC} -o bin/coral-core $^

# Parsing includes all code involved in turning text into an AST
PARSERFILES=obj/parsing/generated/lexer.o obj/parsing/generated/parser.o
bin/coral-parse: ${COREFILES} ${PARSERFILES} obj/parsing/__main__.o
	${CC} -o bin/coral-parse $^

# Codegen includes all the parts involved in compiling the coral AST

# Aux is all coral logic that isn't needed in Core/Parsing/Codegen

# Main includes the primary facade for Coral
