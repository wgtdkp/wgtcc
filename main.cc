#include <iostream>
#include "lexer.h"

using namespace std;

void Usage(void)
{
    printf("usage: \n"
           "    wgtcc filename\n");
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        Usage();
    }

    Lexer lexer(argv[1]);
    lexer.Tokenize();
    
    return 0;
}
