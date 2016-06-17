TARGET = wgtcc

CC = g++
SRCS = ast.cc env.cc error.cc lexer.cc main.cc parser.cc token.cc type.cc visitor.cc
CFLAGS = -g -std=c++11 -Wall

OBJS_DIR = build/
OBJS = $(SRCS:.cc=.o)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	-rm -rf $(TARGET) $(OBJS)
