OBJS_DIR = build/
TARGET = wgtcc
CXX = clang++

SRCS = main.cc  token.cc ast.cc scope.cc type.cc cpp.cc		\
	error.cc scanner.cc parser.cc evaluator.cc  code_gen.cc	\
	encoding.cc
	
CXXFLAGS = -g -std=c++11 -Wall -Wfatal-errors -DDEBUG
OBJS = $(addprefix $(OBJS_DIR), $(SRCS:.cc=.o))

default: all

install:
	@sudo mkdir -p /usr/local/wgtcc
	@sudo cp -r ../include /usr/local/wgtcc
	@make all
	@sudo cp ./build/wgtcc /usr/local/bin/wgtcc

uninstall:
	@sudo rm -f  /usr/local/bin/wgtcc
	@sudo rm -rf /usr/local/wgtcc

all:
	@mkdir -p $(OBJS_DIR)
	@make $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(OBJS_DIR)$@ $^

$(OBJS_DIR)%.o: %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

TESTS := $(filter-out ../test/util.c, $(wildcard ../test/*.c))

TEST_ASMS = $(SRCS:.c=.s)

test: install
	@for test in $(TESTS); do					\
		echo "clang $$test";					\
		clang -std=c11 -w $$test;				\
		./a.out;								\
		echo "wgtcc $$test";					\
		./$(OBJS_DIR)$(TARGET) -no-pie $$test;	\
		./a.out;								\
	done
	@rm -f *.s
	@rm -f ./a.out


.PHONY: clean

clean:
	-rm -rf $(OBJS_DIR)
