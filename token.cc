#include "token.h"

#include "parser.h"


const std::unordered_map<std::string, int> Token::kwTypeMap_ {
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

const std::unordered_map<int, const char*> Token::TagLexemeMap_ {
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
  { Token::PTR, "->" },
  { Token::INC, "++" },
  { Token::DEC, "--" },
  { Token::LEFT, "<<" },
  { Token::RIGHT, ">>" },
  { Token::LE, "<=" },
  { Token::GE, ">=" },
  { Token::EQ, "==" },
  { Token::NE, "!=" },
  { Token::AND, "&&" },
  { Token::OR, "||" },
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
  { Token::LITERAL, "(string literal)" },
};


void PrintTokSeq(TokenSequence& tokSeq)
{
  auto iter = tokSeq.begin_;
  for (; iter != tokSeq.end_; iter++) {
    std::cout << iter->tag_ << "\t";
    std::cout << iter->str_ << std::endl;
  }
  std::cout << std::endl;
}


void PrintTokList(TokenList& tokList)
{
  TokenSequence tokSeq(&tokList);
  PrintTokSeq(tokSeq);
}


/*
 * If this seq starts from the begin of a line.
 * Called only after we have saw '#' in the token sequence.
 */ 
bool TokenSequence::IsBeginOfLine(void) const
{
  if (begin_ == tokList_->begin())
    return true;

  auto pre = begin_;
  --pre;

  return (begin_->loc_.fileName_ != pre->loc_.fileName_
      ||  begin_->loc_.line_ > pre->loc_.line_);
}

Token* TokenSequence::Peek(void)
{
  static Token eof;
  if (Empty()) {
    auto back = Back();
    eof = *back;
    eof.tag_ = Token::END;
    //eof.begin_ = back->end_;
    //eof.end_ = eof.begin_ + 1;
    return &eof;
  } else if (parser_ && begin_->tag_ == Token::IDENTIFIER
      && begin_->str_ == "__func__") {
    begin_->tag_ = Token::LITERAL;

    //auto curFunc = parser_->CurFunc();
    begin_->str_ = parser_->CurFunc()->Name();
    //std::string* name;
    //if(curFunc)
    //    name = new std::string("\"" + curFunc->Name() + "\"");
    //else
    //    name = new std::string("\"\"");
    //begin_->begin_ = const_cast<char*>(name->c_str());
    //begin_->end_ = begin_->begin_ + name->size();
  }
  return &(*begin_);
}

Token* TokenSequence::Expect(int expect)
{
  auto tok = Peek();
  if (!Try(expect)) {
    Error(tok, "'%s' expected, but got '%s'",
        Token::Lexeme(expect), tok->str_.c_str());
  }
  return tok;
}
