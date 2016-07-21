#include "error.h"
#include "lexer.h"
#include "parser.h"

#include <cstdio>
#include <iostream>


void Usage(void)
{
    printf("usage: \n"
           "    wgtcc filename\n");
    exit(0);
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        Usage();
    }

    Lexer lexer;
    TokenList tokList;
    lexer.ReadFile(argv[1]);
    lexer.Tokenize(tokList);

    Parser parser(&tokList);
    parser.ParseTranslationUnit();
    
    return 0;
}
