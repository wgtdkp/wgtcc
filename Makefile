
OBJS_DIR = build/

TARGET = wgtcc

CC = g++

SRCS = ast.cc cpp.cc code_gen.cc error.cc lexer.cc\
	   main.cc parser.cc scope.cc\
	   token.cc type.cc visitor.cc

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
