#include <iostream>
#include "lexer.h"

using namespace std;


class A {
public:
	A() = delete;
	A(int k) {}
	int a = 42;
	int fun() {
		a += 1;
		return a;
	}
};

/*
int main(int argc, char* argv[])
{
*/
int main(void)
{
	//int argc;
	const char* argv[2] = {"wgtcc", "test.c"};
	printf("%s\n", __FILE__); return 0;
	Lexer lexer(argv[1]);
	lexer.Tokenize();
	return 0;
}
