#ifndef _TOKEN_H_
#define _TOKEN_H_

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <list>
#include <cstring>

class Token
{
	friend class Lexer;
public:
	enum {
		/**punctuators**/
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
		LESS = '<',
		GREATER = '>',
		EQUAL = '=',
		DOT = '.',
		MOD = '%',
		LBRACE = '{',
		RBRACE = '}',
		XOR = '^',
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
		/**punctuators end**/

		/**key words**/
		AUTO,
		BREAK,
		CASE,
		CHAR,
		CONST,
		CONTINUE,
		DEFAULT,
		DO,
		DOUBLE,
		ELSE,
		ENUM,
		EXTERN,
		FLOAT,
		FOR,
		GOTO,
		//FUNC_NAME,
		IF,		
		INLINE,
		INT,
		LONG,		
		REGISTER,
		RESTRICT,
		RETURN,
		SHORT,
		SIGNED,
		SIZEOF,
		STATIC,		
		STRUCT,
		SWITCH,		
		TYPEDEF,
		UNION,
		UNSIGNED,
		VOID,
		VOLATILE,
		WHILE,
		ALIGNAS, //_Alignas
		ALIGNOF, //_Alignof
		ATOMIC, //_Atomic
		BOOL, //_Bool
		COMPLEX, //_Complex
		GENERIC, //_Generic
		IMAGINARY, //_Imaginary
		NORETURN, //_Noreturn
		STATIC_ASSERT, //_Static_assert
		THREAD_LOCAL, //_Thread_local
		/**key words end**/
	
		IDENTIFIER,
		CONSTANT,
		I_CONSTANT,
		F_CONSTANT,
		STRING_LITERAL,

		//for the parser, a identifier is a typedef name or user defined type
		TYPE_NAME,
		POSTFIX_INC,
		POSTFIX_DEC,
		PREFIX_INC,
		PREFIX_DEC,
		ADDR,
		DEREF,
		PLUS,
		MINUS,
			

		IGNORE,
		INVALID,
		END,
		NOTOK = -1,
	};

	virtual ~Token(void) {
		if (IsString() || IsKeyWord() || IsIdentifier())
			delete[] _val;
	}

	int Tag(void) const {
		return _tag;
	}

	int Line(void) const {
		return _line;
	}

	int Column(void) const {
		return _column;
	}

	const char* Val(void) const {
		return _val;
	}

	const char* FileName(void) const {
		return _fileName;
	}

	void SetVal(const char* begin, const char* end) {
		size_t size = end - begin;
		auto val = new char[size + 1];
		memcpy(val, begin, size);
		val[size] = 0;
		_val = val;
	}

	static bool IsKeyWord(int tag) {
		return AUTO <= tag && tag <= THREAD_LOCAL;
	}

	bool IsKeyWord(void) const {
		return IsKeyWord(_tag);
	}

	bool IsPunctuator(void) const {
		return LPAR <= _tag && _tag <= ELLIPSIS;
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

	bool IsTypeQual(void) const {
		switch (_tag) {
		case CONST: case RESTRICT:
		case VOLATILE: case ATOMIC:
			return true;
		default: return false;
		}
	}

	bool IsTypeSpec(void) const {
		switch (_tag) {
		case VOID: case CHAR: case SHORT: 
		case INT: case LONG: case FLOAT:
		case DOUBLE: case SIGNED: case UNSIGNED:
		case BOOL: case COMPLEX: case IMAGINARY:
		case STRUCT: case UNION: case ENUM: case ATOMIC:
			return true;
		default: return false;
		}
	}

	static const char* Lexeme(int tag) {
		return _TagLexemeMap.at(tag);
	}

private:
	int _tag;
	int _line;
	int _column;
	const char* _fileName;
	const char* _val;

	static const std::unordered_map<std::string, int> _kwTypeMap;
	static const std::unordered_map<int, const char*> _TagLexemeMap;
	//static const char* _tokenTable[TOKEN_NUM - OFFSET - 1];
	
private:
	Token(int tag, const char* fileName=nullptr, int line=1, int column=1, const char* begin = nullptr, const char* end = nullptr)
		: _tag(tag), _fileName(fileName), _line(line), _column(column) {
		if (nullptr == begin) {
			assert(IsPunctuator() || IsKeyWord() || IsEOF());
			_val = _TagLexemeMap.at(tag);
		} else {
			SetVal(begin, end);
		}
	}

	//Token::NOTOK represents not a kw.
	static int KeyWordTag(const char* begin, const char* end) {
		std::string key(begin, end);
		auto kwIter = _kwTypeMap.find(key);
		if (_kwTypeMap.end() == kwIter)
			return Token::NOTOK;	//not a key word type
		return kwIter->second;
	}

	Token(const Token& other);
	const Token& operator=(const Token& other);
};


/*
class TokenConstant: public Token
{

private:
	TokenConstant(const TokenConstant& other);
	const TokenConstant& operator=(const TokenConstant& other);
};

class TokenIdentifier : public Token
{
public:
	TokenIdentifier(const char* begin, const char* end):
		_lexeme(nullptr), Token(IDENTIFIER) {
		if (end - begin > INIT_SIZE) {
			auto lexeme = new char[end - begin + 1];
			strncpy(lexeme, begin, end - begin);
			lexeme[end - begin] = 0;
			_lexeme = lexeme; 
		} else {
			strncpy(_init, begin, end - begin);
			_init[end - begin] = 0;
		}
	}

	virtual ~TokenIdentifier(void) {
		if (nullptr != _lexeme)
			delete[] _lexeme;
	}

	const char* Lexeme(void) const {
		return _lexeme;
	}

private:
	enum { INIT_SIZE = 8 };
	char _init[INIT_SIZE + 1];
};
*/

#endif
