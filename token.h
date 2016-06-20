#ifndef _WGTCC_TOKEN_H_
#define _WGTCC_TOKEN_H_

#include "ast.h"
#include "visitor.h"

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <list>
#include <cstring>


class Token: public ASTNode
{
    friend class Lexer;
    
public:
    Token(int tag, const Coordinate& coord)
            : ASTNode(nullptr, coord), _tag(tag) {}
    
    virtual ~Token(void) {}
    
    // Do nothing
    virtual void Accept(Visitor* v) {}
    
    //Token::NOTOK represents not a kw.
    static int KeyWordTag(const char* begin, const char* end) {
        std::string key(begin, end);
        auto kwIter = _kwTypeMap.find(key);
        
        if (_kwTypeMap.end() == kwIter)
            return Token::NOTOK;	//not a key word type
        
        return kwIter->second;
    }

    enum {
        /* punctuators */
        LPAR = '(',
        RPAR = ')',
        LSQB = '[',
        RSQB = ']',
        COLON = ':',
        COMMA = ',',
        SEMI = ';',
        ADD = '+',
        SUB = '-',
        MUL = '*',
        DIV = '/',
        OR = '|',
        AND = '&',
        XOR = '^',
        LESS = '<',
        GREATER = '>',
        EQUAL = '=',
        DOT = '.',
        MOD = '%',
        LBRACE = '{',
        RBRACE = '}',
        TILDE = '~',
        NOT = '!',
        COND = '?',
        SHARP = '#',

        DSHARP = 128, //'##'
        PTR_OP,
        INC_OP,
        DEC_OP,
        LEFT_OP,
        RIGHT_OP,
        LE_OP,
        GE_OP,
        EQ_OP,
        NE_OP,
        AND_OP,
        OR_OP,

        MUL_ASSIGN,
        DIV_ASSIGN,
        MOD_ASSIGN,
        ADD_ASSIGN,
        SUB_ASSIGN,
        LEFT_ASSIGN,
        RIGHT_ASSIGN,
        AND_ASSIGN,
        XOR_ASSIGN,
        OR_ASSIGN,

        ELLIPSIS,
        /* punctuators end */

        /* key words */
        //type qualifier
        CONST,
        RESTRICT,
        VOLATILE,
        ATOMIC, //_Atomic

        //type specifier
        VOID,
        CHAR,
        SHORT,
        INT,
        LONG,
        FLOAT,
        DOUBLE,
        SIGNED,
        UNSIGNED,
        BOOL,		//_Bool
        COMPLEX,	//_Complex
        STRUCT,
        UNION,
        ENUM,

        TYPEDEF_NAME, //not used

        //function specifier
        INLINE,
        NORETURN,	//_Noreturn

        //alignment specifier
        ALIGNAS, //_Alignas

        //storage class specifier
        TYPEDEF,
        EXTERN,
        STATIC,
        THREAD,	//_Thread_local
        AUTO,
        REGISTER,

        BREAK,
        CASE,
        CONTINUE,
        DEFAULT,
        DO,
        ELSE,
        FOR,
        GOTO,
        IF,		
        RETURN,
        SIZEOF,
        SWITCH,		
        WHILE,
        ALIGNOF, //_Alignof
        GENERIC, //_Generic
        IMAGINARY, //_Imaginary
        STATIC_ASSERT, //_Static_assert
        /* key words end */
    
        IDENTIFIER,
        CONSTANT,
        I_CONSTANT,
        F_CONSTANT,
        STRING_LITERAL,

        //for the parser, a identifier is a typedef name or user defined type
        POSTFIX_INC,
        POSTFIX_DEC,
        PREFIX_INC,
        PREFIX_DEC,
        ADDR,  // '&'
        DEREF, // '*'
        PLUS,
        MINUS,
        CAST,
            
        IGNORE,
        INVALID,
        END,
        NOTOK = -1,
    };


    int Tag(void) const {
        return _tag;
    }

    static bool IsKeyWord(int tag) {
        return CONST <= tag && tag <= STATIC_ASSERT;
    }

    bool IsKeyWord(void) const {
        return IsKeyWord(_tag);
    }

    bool IsPunctuator(void) const {
        return 0 <= _tag && _tag <= ELLIPSIS;
    }

    bool IsString(void) const {
        return _tag == STRING_LITERAL;
    }

    bool IsConstant(void) const {
        return CONSTANT <= _tag && _tag <= F_CONSTANT;
    }

    bool IsIdentifier(void) const {
        return IDENTIFIER == _tag;
    }

    bool IsEOF(void) const {
        return _tag == Token::END;
    }

    bool IsTypeSpecQual(void) const { 
        return CONST <= _tag && _tag <= ENUM;
    }
    
    bool IsDecl(void) const {
        return CONST <= _tag && _tag <= REGISTER;
    }
    
    static const char* Lexeme(int tag) {
        auto iter = _TagLexemeMap.find(tag);
        if (iter == _TagLexemeMap.end())
            return nullptr;
            
        return iter->second;
    }

private:
    int _tag;
    
    static const std::unordered_map<std::string, int> _kwTypeMap;
    static const std::unordered_map<int, const char*> _TagLexemeMap;
    //static const char* _tokenTable[TOKEN_NUM - OFFSET - 1];
    
    Token(const Token& other);
    const Token& operator=(const Token& other);	
};

#endif
