#include "error.h"
#include "lexer.h"
#include "parser.h"

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

    Lexer lexer(argv[1]);
    lexer.Tokenize();

    Parser parser(&lexer);
    parser.ParseTranslationUnit();
    
    return 0;
}
