#ifndef _WGTCC_LEXER_H_
#define _WGTCC_LEXER_H_

#include "ast.h"
#include "error.h"
#include "token.h"

#include <cassert>
#include <cstdio>
#include <string>


class Lexer
{
public:
    static const int _maxPredict = 4;
    
    explicit Lexer(std::string* text, const std::string* fileName=nullptr,
            unsigned line=1): _text(text), _fileName(fileName), _line(line) {}
    
    virtual ~Lexer(void) {}
    
    Lexer(const Lexer& other)=delete;
    
    Lexer& operator=(const Lexer& other)=delete;

    void Tokenize(TokenSeq& tokSeq);

private:
    std::string* _text;
    const std::string* _fileName;
    int _line;

    void ProcessBackSlash(void);
};

std::string* ReadFile(const std::string& fileName);

#endif
