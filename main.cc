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

int main(int argc, char* argv[])
{
    if (argc < 2) {
        Usage();
    }
    program = std::string(argv[0]);
    std::string* fileName = nullptr;

    for (auto i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            fileName = new std::string(argv[i]);
            break;
            continue;
        }

        switch (argv[i][1]) {
        case 'I':
            assert(0);
            //cpp.AddSearchPath(std::string(&argv[i][2]));
            break;
        case 'D':
            assert(0);
            //ParseDef(cpp, &argv[i][2]);
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
    Preprocessor cpp(fileName);
    TokenSeq tokSeq;
    cpp.Process(tokSeq);

    Parser parser(tokSeq);
    parser.ParseTranslationUnit();
    
    return 0;
}
