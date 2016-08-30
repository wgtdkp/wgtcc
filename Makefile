
OBJS_DIR = build/

TARGET = wgtcc

CC = g++

SRCS = main.cc  token.cc ast.cc scope.cc type.cc cpp.cc		\
	error.cc scanner.cc parser.cc evaluator.cc  code_gen.cc

CFLAGS = -g -std=c++11 -Wall


OBJS = $(addprefix $(OBJS_DIR), $(SRCS:.cc=.o))

all:
	mkdir -p $(OBJS_DIR)
	make $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(OBJS_DIR)$@ $^

$(OBJS_DIR)%.o: %.cc
#	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	-rm -rf $(OBJS_DIR)
