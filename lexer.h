#ifndef _WGTCC_LEXER_H_
#define _WGTCC_LEXER_H_

#include "ast.h"
#include "error.h"
#include "token.h"

#include <cassert>
#include <vector>
#include <string>


typedef std::list<Token> TokenList;

class Lexer
{
public:
    static const int _maxPredict = 4;

    Lexer(void): _fileName(nullptr) {}

    virtual ~Lexer(void) {
        delete[] _text;
    }

    void Tokenize(TokenList& tokList);

    void ReadFile(const char* fileName);
    
    void ReadStr(const std::string& str);

    const char* ParseName(const char* path);

private:
    static const int _maxSize = 1024 * 1024 * 64;
    const char* _fileName;
    char* _text;

    Lexer(const Lexer& other);
    
    const Lexer& operator=(const Lexer& other);
};

#endif
