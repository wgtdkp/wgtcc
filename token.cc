#include "token.h"

using namespace std;


const unordered_map<string, int> Token::_kwTypeMap = {
    { "auto", Token::AUTO },
    { "break", Token::BREAK },
    { "case", Token::CASE },
    { "char", Token::CHAR },
    { "const", Token::CONST },
    { "continue", Token::CONTINUE },
    { "default", Token::DEFAULT },
    { "do", Token::DO },
    { "double", Token::DOUBLE },
    { "else", Token::ELSE },
    { "enum", Token::ENUM },
    { "extern", Token::EXTERN },
    { "float", Token::FLOAT },
    { "for", Token::FOR },
    { "goto", Token::GOTO },
    { "if", Token::IF },
    { "inline", Token::INLINE },
    { "int", Token::INT },
    { "long", Token::LONG },
    { "signed", Token::SIGNED },
    { "unsigned", Token::UNSIGNED },
    { "register", Token::REGISTER },
    { "return", Token::RETURN },
    { "short", Token::SHORT },
    { "sizeof", Token::SIZEOF },
    { "static", Token::STATIC },
    { "struct", Token::STRUCT },
    { "switch", Token::SWITCH },
    { "typedef", Token::TYPEDEF },
    { "union", Token::UNION },
    { "void", Token::VOID },
    { "volatile", Token::VOLATILE },
    { "while", Token::WHILE },
    { "_Alignas", ALIGNAS },
    { "_Alignof", ALIGNOF },
    { "_Atomic", ATOMIC },
    { "_Bool", BOOL },
    { "_Complex", COMPLEX },
    { "_Generic", GENERIC },
    { "_Imaginary", IMAGINARY },
    { "_Noreturn", NORETURN },
    { "_Static_assert", STATIC_ASSERT },
    { "_Thread_local", THREAD },
};

const unordered_map<int, const char*> Token::_TagLexemeMap = {
    { '(', "(" },
    { ')', ")" },
    { '[', "[" },
    { ']', "]" },
    { ':', ":" },
    { ',', "," },
    { ';', ";" },
    { '+', "+" },
    { '-', "-" },
    { '*', "*" },
    { '/', "/" },
    { '|', "|" },
    { '&', "&" },
    { '<', "<" },
    { '>', ">" },
    { '=', "=" },
    { '.', "." },
    { '%', "%" },
    { '{', "{" },
    { '}', "}" },
    { '^', "^" },
    { '~', "~" },
    { '!', "!" },
    { '?', "?" },
    { '#', "#" },

    { Token::DSHARP, "##" },
    { Token::PTR_OP, "->" },
    { Token::INC_OP, "++" },
    { Token::DEC_OP, "--" },
    { Token::LEFT_OP, "<<" },
    { Token::RIGHT_OP, ">>" },
    { Token::LE_OP, "<=" },
    { Token::GE_OP, ">=" },
    { Token::EQ_OP, "==" },
    { Token::NE_OP, "!=" },
    { Token::AND_OP, "&&" },
    { Token::OR_OP, "||" },
    { Token::MUL_ASSIGN, "*=" },
    { Token::DIV_ASSIGN, "/=" },
    { Token::MOD_ASSIGN, "%=" },
    { Token::ADD_ASSIGN, "+=" },
    { Token::SUB_ASSIGN, "-=" },
    { Token::LEFT_ASSIGN, "<<=" },
    { Token::RIGHT_ASSIGN, ">>=" },
    { Token::AND_ASSIGN, "&=" },
    { Token::XOR_ASSIGN, "^=" },
    { Token::OR_ASSIGN, "|=" },
    { Token::ELLIPSIS, "..." },

    { Token::AUTO, "auto" },
    { Token::BREAK, "break" },
    { Token::CASE, "case" },
    { Token::CHAR, "char" },
    { Token::CONST, "const" },
    { Token::CONTINUE, "continue" },
    { Token::DEFAULT, "default" },
    { Token::DO, "do" },
    { Token::DOUBLE, "double" },
    { Token::ELSE, "else" },
    { Token::ENUM, "enum" },
    { Token::EXTERN, "extern" },
    { Token::FLOAT, "float" },
    { Token::FOR, "for" },
    { Token::GOTO, "goto" },
    { Token::IF, "if" },
    { Token::INLINE, "inline" },
    { Token::INT, "int" },
    { Token::LONG, "long" },
    { Token::SIGNED, "signed" },
    { Token::UNSIGNED, "unsigned" },
    { Token::REGISTER, "register" },
    { Token::RETURN, "return" },
    { Token::SHORT, "short" },
    { Token::SIZEOF, "sizeof" },
    { Token::STATIC, "static" },
    { Token::STRUCT, "struct" },
    { Token::SWITCH, "switch" },
    { Token::TYPEDEF, "typedef" },
    { Token::UNION, "union" },
    { Token::VOID, "void" },
    { Token::VOLATILE, "volatile" },
    { Token::WHILE, "while" },
    { Token::ALIGNAS, "_Alignas" },
    { Token::ALIGNOF, "_Alignof" },
    { Token::ATOMIC, "_Atomic" },
    { Token::BOOL, "_Bool" },
    { Token::COMPLEX, "_Complex" },
    { Token::GENERIC, "_Generic" },
    { Token::IMAGINARY, "_Imaginary" },
    { Token::NORETURN, "_Noreturn" },
    { Token::STATIC_ASSERT, "_Static_assert" },
    { Token::THREAD, "_Thread_local" },

    { Token::END, "(eof)" },
    { Token::IDENTIFIER, "(identifier)" },
    { Token::CONSTANT, "(constant)" },
    { Token::STRING_LITERAL, "(string literal)" },
};


void PrintTokSeq(TokenSeq& tokSeq)
{
    auto iter = tokSeq._begin;
    for (; iter != tokSeq._end; iter++) {
        std::cout << iter->_tag << "\t";
        std::cout << iter->Str() << std::endl;
    }
    std::cout << std::endl;
}

void PrintTokList(TokenList& tokList)
{
    TokenSeq tokSeq(&tokList);
    PrintTokSeq(tokSeq);
}


/*
 * If this seq starts from the begin of a line.
 * Called only after we have saw '#' in the token sequence.
 */ 
bool TokenSeq::IsBeginOfLine(void) const
{
    if (_begin == _tokList->begin())
        return true;

    auto pre = _begin;
    --pre;

    return (_begin->_fileName != pre->_fileName
            ||  _begin->_line > pre->_line);
}

Token* TokenSeq::Peek(void)
{
    static Token eof;
    if (Empty()) {
        auto back = Back();
        eof = *back;
        eof._tag = Token::END;
        eof._begin = back->_end;
        eof._end = eof._begin + 1;
        return &eof;
    }
    return &(*_begin);
}

Token* TokenSeq::Expect(int expect)
{
    auto tok = Next();
    if (tok->Tag() != expect) {
        PutBack();
        Error(tok, "'%s' expected, but got '%s'",
                Token::Lexeme(expect), tok->Str().c_str());
    }
    return tok;
}
