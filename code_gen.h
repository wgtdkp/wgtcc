#ifndef _WGTCC_CODE_GEN_H_
#define _WGTCC_CODE_GEN_H_

#include <cstdio>

class Parser;
class Expr;

class Register
{
public:
    Register(void): _expr(nulltpr) {}

    bool Using(void) const {
        return _expr != nullptr;
    }
    
private:
    Expr* _expr;
};


class CodeGen
{
public:
    CodeGen(Parser* parser, FILE* outFile)
            : _parser(parser), _outFile(outFile) {}
    
    PodeGen& operator=(const CodeGen& other) = delete;
    
    CodeGen(const CodeGen& other) = delete;

    ~CodeGen(void) {}

    void Emit(const char* format, ...);

private:
    Parser* _parser;
    FILE* _outFile;    
};

#endif
