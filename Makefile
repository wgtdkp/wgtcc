OBJS_DIR = build/
TARGET = wgtcc
CC = g++

SRCS = main.cc  token.cc ast.cc scope.cc type.cc cpp.cc		\
	error.cc scanner.cc parser.cc evaluator.cc  code_gen.cc	\
	encoding.cc
	
CFLAGS = -g -std=c++11 -Wall
OBJS = $(addprefix $(OBJS_DIR), $(SRCS:.cc=.o))

install:
	@sudo mkdir -p /usr/local/wgtcc
	@sudo cp -r ./include /usr/local/wgtcc
	@make all

uninstall:
	@sudo rm -rf /usr/local/wgtcc

all:
	@mkdir -p $(OBJS_DIR)
	@make $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(OBJS_DIR)$@ $^

$(OBJS_DIR)%.o: %.cc
	$(CC) $(CFLAGS) -O2 -o $@ -c $<

TESTS := $(filter-out test/util.c, $(wildcard test/*.c))

TEST_ASMS = $(SRCS:.c=.s)

test: $(TARGET)
	@for test in $(TESTS); do					\
		echo $$test;										\
		./$(OBJS_DIR)$(TARGET) $$test;	\
	done
	@rm -f *.s
	@rm -f ./a.out


.PHONY: clean

clean:
	-rm -rf $(OBJS_DIR)
