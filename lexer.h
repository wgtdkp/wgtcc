#ifndef _WGTCC_LEXER_H_
#define _WGTCC_LEXER_H_

#include "ast.h"
#include "error.h"
#include "token.h"

#include <cassert>
#include <vector>
#include <string>


class Lexer
{
public:
    Lexer(const char* path): _path(path) {
        ReadFile(_path);
        _tokTop = 0;
    }

    virtual ~Lexer(void) {
        auto iter = _tokBuf.begin();
        for (; iter != _tokBuf.end(); iter++)
            delete *iter;
        delete[] _text;
    }

    void Tokenize(void);

    Token* Get(void) {
        assert(_tokTop < _tokBuf.size());
        return _tokBuf[_tokTop++];
    }

    Token* Peek(void) {
        return _tokBuf[_tokTop];
    }

    void Unget(void) {
        assert(_tokTop > 0);
        --_tokTop;
    }

private:
    static const int _maxSize = 1024 * 1024 * 64;
    
    const char* _path;
    
    char* _text;

    size_t _tokTop;
    std::vector<Token*> _tokBuf;

    Lexer(const Lexer& other);
    
    const Lexer& operator=(const Lexer& other);
    
    void ReadFile(const char* fileName);
    
    const char* ParseName(const char* path);
    
    Token* NewToken(int tag, const Coordinate& coord) {
        return new Token(tag, coord);
    }
};

#endif
