CC      = clang++
LEX     = flex
YACC    = bison
CFLAGS  = -Wall -std=c++11 -g
LIBS    = -lfl -ly
INCLUDE = -Iinclude

SCANNER = scanner
PARSER  = parser
COMPILER  = compiler

ASTDIR = src/AST/
AST := $(shell find $(ASTDIR) -name '*.cpp')

VISITORDIR = src/visitor/
VISITOR := $(shell find $(VISITORDIR) -name '*.cpp')

TABLERDIR = src/symbol/
TABLER := $(shell find $(TABLERDIR) -name '*.cpp')

SEMANTICDIR = src/semantic/
SEMANTIC := $(shell find $(SEMANTICDIR) -name '*.cpp')

CODEGENDIR = src/codegen/
CODEGEN := $(shell find $(CODEGENDIR) -name '*.cpp')

SRC := $(AST) \
       $(VISITOR) \
       $(TABLER) \
       $(SEMANTIC) \
       $(CODEGEN)

EXEC = $(COMPILER)
OBJS = $(PARSER:=.cpp) \
       $(SCANNER:=.cpp) \
       $(SRC)

# Substitution reference
DEPS := $(OBJS:%.cpp=%.d)
OBJS := $(OBJS:%.cpp=%.o)

all: $(EXEC)

# Static pattern rule
$(SCANNER).cpp: %.cpp: %.l
	$(LEX) -o $@ $<

$(PARSER).cpp: %.cpp: %.y
	$(YACC) -o $@ --defines=parser.h -v $<

%.o: %.cpp
	$(CC) -o $@ $(CFLAGS) $(INCLUDE) -c -MMD $<

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(INCLUDE)

clean:
	$(RM) $(DEPS) $(SCANNER:=.cpp) $(PARSER:=.cpp) $(OBJS) $(EXEC) parser.output parser.h

-include $(DEPS)
