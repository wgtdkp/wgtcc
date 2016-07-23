#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "cpp.h"

#include <cstdio>
#include <iostream>
#include <string>

std::string program;

void Usage(void)
{
    printf("Usage: wgtcc [options] file...\n"
           "Options: \n"
           "  --help    show this information\n"
           "  -D        define macro\n"
           "  -I        add search path\n");
    
    exit(0);
}

void ParseDef(Preprocessor& cpp, const char* def)
{
    auto p = def;
    while (*p != '\0' && *p != '=')
        ++p;
    std::string name(def, p);
    std::string rep;
    if (*p == '=') {
        rep = std::string(++p);
    }

    Lexer lexer;
    lexer.ReadStr(rep);
    TokenSeq tokSeq;
    lexer.Tokenize(tokSeq);
    
    cpp.AddMacro(name, Macro(tokSeq));
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        Usage();
    }
    program = std::string(argv[0]);

    Preprocessor cpp;
    const char* fileName = nullptr;

    for (auto i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            fileName = argv[i];
            break;
            continue;
        }

        switch (argv[i][1]) {
        case 'I':
            cpp.AddSearchPath(std::string(&argv[i][2]));
            break;
        case 'D':
            ParseDef(cpp, &argv[i][2]);
            break;
        case '-': // --
            switch (argv[i][2]) {
            case 'h':
                Usage();
                break;
            default:
                Error("unrecognized command line option '%s'", argv[i]);
            }
            break;
        case '\0':
        default:
            Error("unrecognized command line option '%s'", argv[i]);
        }
    }

    if (fileName == nullptr) {
        Usage();
    }

    Lexer lexer;
    TokenSeq tokSeq, ppTokSeq;
    lexer.ReadFile(fileName);
    lexer.Tokenize(tokSeq);

    PrintTokSeq(tokSeq);
    cpp.Process(ppTokSeq, tokSeq);
    PrintTokSeq(ppTokSeq);

    Parser parser(ppTokSeq);
    parser.ParseTranslationUnit();
    
    return 0;
}
