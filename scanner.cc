#include "scanner.h"

#include <cctype>
#include <climits>


Token Scanner::Scan(bool ws) {
  ws_ = ws;
  SkipWhiteSpace();

  Mark();

  if (Test('\n')) {
    const auto& ret = MakeNewLine();
    Next();
    return ret;
  }
  auto c = Next();
  switch (c) {
  case '#': return MakeToken(Try('#') ? Token::DSHARP: c);
  case ':': return MakeToken(Try('>') ? ']': c);
  case '(': case ')': case '[': case ']':
  case '?': case ',': case '{': case '}':
  case '~': case ';':
    return MakeToken(c);
  case '-':
    if (Try('>')) return MakeToken(Token::PTR);
    if (Try('-')) return MakeToken(Token::DEC);
    if (Try('=')) return MakeToken(Token::SUB_ASSIGN);
    return MakeToken(c);
  case '+':
    if (Try('+'))	return MakeToken(Token::INC);
    if (Try('=')) return MakeToken(Token::ADD_ASSIGN);
    return MakeToken(c);
  case '<':
    if (Try('<')) return MakeToken(Try('=') ? Token::LEFT_ASSIGN: Token::LEFT);
    if (Try('=')) return MakeToken(Token::LE);
    if (Try(':')) return MakeToken('[');
    if (Try('%')) return MakeToken('{');
    return MakeToken(c);
  case '%':
    if (Try('=')) return MakeToken(Token::MOD_ASSIGN);
    if (Try('>')) return MakeToken('}');
    if (Try(':')) {
      if (Try('%')) {
        if (Try(':'))	return MakeToken(Token::DSHARP);
        PutBack();
        return MakeToken('#');
      }
    }
    return MakeToken(c);
  case '>':
    if (Try('>')) return MakeToken(Try('=') ? Token::RIGHT_ASSIGN: Token::RIGHT);
    if (Try('=')) return MakeToken(Token::GE);
    return MakeToken(c);
  case '=': return MakeToken(Try('=') ? Token::EQ: c);
  case '!': return MakeToken(Try('=') ? Token::NE: c);
  case '&':
    if (Try('&')) return MakeToken(Token::LOGICAL_AND);
    if (Try('=')) return MakeToken(Token::AND_ASSIGN);
    return MakeToken(c);
  case '|':
    if (Try('|')) return MakeToken(Token::LOGICAL_OR);
    if (Try('=')) return MakeToken(Token::OR_ASSIGN);
    return MakeToken(c);
  case '*': return MakeToken(Try('=') ? Token::MUL_ASSIGN: c);
  case '/':
    if (Peek() == '/' || Peek() == '*') {
      SkipComment();
      return Scan(true);
    }
    return MakeToken(c);
  case '^': return MakeToken(Try('=') ? Token::XOR_ASSIGN: c);
  case '.':
    if (isdigit(Peek())) return ScanNumber();
    if (Try('.')) {
      if (Try('.')) return MakeToken(Token::ELLIPSIS);
      PutBack();
      return MakeToken('.');
    }
    return MakeToken(c);
  case '0' ... '9': return ScanNumber();	
  case 'u': case 'U': case 'L': {
    /*auto enc = */ScanEncoding(c);
    if (Try('\'')) return ScanCharacter();
    if (Try('\"')) return ScanLiteral();
    return ScanIdentifier();
  }
  case '\'': return ScanCharacter();
  case '\"': return ScanLiteral();
  case 'a' ... 't': case 'v' ... 'z': case 'A' ... 'K':
  case 'M' ... 'T': case 'V' ... 'Z': case '_': case '$':
  case 0x80 ... 0xfd:
    return ScanIdentifier();
  case '\\':
    // Universal character name is allowed in identifier
    if (Peek() == 'u' || Peek() == 'U')
      return ScanIdentifier();
    return MakeToken(Token::INVALID);
  case '\0': return MakeToken(Token::END);
  default: return MakeToken(Token::INVALID);
  }
}


void Scanner::SkipWhiteSpace(void) {
  while (isspace(Peek()) && Peek() != '\n') {
    ws_ = true;
    Next();
  }
}


void Scanner::SkipComment(void) {
  if (Try('/')) {
    // Line comment terminated an newline or eof
    while (!Empty()) {
      if (Peek() == '\n')
        return;
      Next();
    }
    return;
  } else if (Try('*')) {
    while (!Empty()) {
      auto c = Next();
      if (c  == '*' && Peek() == '/') {
        Next();
        return;
      }
    }
    Error(loc_, "unterminated block comment");
  }
  assert(false);
}

/*
Token Scanner::ScanIdentifier() {
  PutBack();
  std::string str;
  auto c = Next();
  while (isalnum(c)
       || (0x80 <= c && c <= 0xfd)
       || c == '_'
       || c == '$'
       || IsUCN(c)) {
    if (IsUCN(c)) {
      c = ScanEscaped(); // Call ScanUCN()
      // convert 'c' to utf8 characters
      AppendUCN(str, c);
      continue;
    }
    str.push_back(c);
    c = Next();
  }
  PutBack();
  return {Token::IDENTIFIER, loc_, str};
}
*/

Token Scanner::ScanIdentifier() {
  PutBack();
  auto c = Peek();
  while (isalnum(c)
       || (0x80 <= c && c <= 0xfd)
       || c == '_'
       || c == '$'
       || IsUCN(c)) {
    if (IsUCN(c))
      c = ScanEscaped(); // Just read it
    Next();
    c = Peek();
  }
  return MakeToken(Token::IDENTIFIER);
}

// Scan PP-Number 
Token Scanner::ScanNumber() {
  PutBack();
  int tag = Token::I_CONSTANT;	
  auto c = Peek();
  while (c == '.' || isdigit(c) || isalpha(c) || c == '_' || IsUCN(c)) {
    if (c == 'e' || c =='E' || c == 'p' || c == 'P') {
      if (!Try('-')) Try('+');
      tag = Token::F_CONSTANT;
    }  else if (IsUCN(c)) {
      ScanEscaped();
    } else if (c == '.') {
      tag = Token::F_CONSTANT;
    }
    Next();
    c = Peek();
  }
  return MakeToken(tag);
}

/*
// Encoding literal: |str|val|
Token Scanner::ScanLiteral(Encoding enc) {
  std::string str;
  auto c = Next();
  while (c != '\"' && c != '\n' && c != '\0') {
    bool isucn = IsUCN(c);
    if (c == '\\')
      c = ScanEscaped();
    if (isucn)
      AppendUCN(str, c);
    else
      str.push_back(c);
    c = Next();
  }
  if (c != '\"')
    Error(loc_, "unterminated string literal");
  str.push_back(static_cast<int>(enc));
  return {Token::LITERAL, loc_, str};
}
*/

Token Scanner::ScanLiteral() {
  auto c = Next();
  while (c != '\"' && c != '\n' && c != '\0') {
    if (c == '\\') Next();
    c = Next();
  }
  if (c != '\"')
    Error(loc_, "unterminated string literal");
  return MakeToken(Token::LITERAL);
}

/*
// Encode character: |val|enc|
Token Scanner::ScanCharacter(Encoding enc) {
  std::string str;
  int val = 0;
  auto c = Next();
  while (c != '\'' && c != '\n' && c != '\0') {
    if (c == '\\')
      c = ScanEscaped();
    if (enc == Encoding::NONE)
      val = (val << 8) + c;
    else
      val = c;
  }
  if (c != '\'')
    Error(loc_, "unterminated character constant");
  if (enc == Encoding::CHAR16)
    val &= USHRT_MAX;
  Append32BE(str, val);
  str.push_back(static_cast<int>(enc));
  return {Token::C_CONSTANT, loc_, str};
}
*/


Token Scanner::ScanCharacter() {
  auto c = Next();
  while (c != '\'' && c != '\n' && c != '\0') {
    if (c == '\\') Next();
    c = Next();
  }
  if (c != '\'')
    Error(loc_, "unterminated character constant");
  return MakeToken(Token::C_CONSTANT);
}


int Scanner::ScanEscaped() {
  auto c = Next();
  switch (c) {
  case '\\': case '\'': case '\"': case '\?':
    return c;
  case 'a': return '\a'; 
  case 'b': return '\b';
  case 'f': return '\f';
  case 'n': return '\n';
  case 'r': return '\r';
  case 't': return '\t';
  case 'v': return '\v';
  // Non-standard GCC extention
  case 'e': return '\033';
  case 'x': return ScanHexEscaped();
  case '0' ... '7': return ScanOctEscaped();
  case 'u': return ScanUCN(4);
  case 'U': return ScanUCN(8);
  default: Error(loc_, "unrecognized escape character '%c'", c);
  }
  return c; // Make compiler happy
}


int Scanner::ScanHexEscaped() {
  int val = 0, c = Peek();
  if (!isxdigit(c))
    Error(loc_, "expect xdigit, but got '%c'", c);
  while (isxdigit(c)) {
    val = (val << 4) + XDigit(c);
    Next();
    c = Peek();
  }
  return val;
}


int Scanner::ScanOctEscaped() {
  int val = 0, c = Peek();
  if (!IsOctal(c))
    Error(loc_, "expect octal, but got '%c'", c);
  val = (val << 3) + XDigit(c);
  Next();
  c = Peek();
  if (!IsOctal(c)) {
    return val;
  }
  val = (val << 3) + XDigit(c);
  Next();
  c = Peek();
  if (!IsOctal(c)) {
    return val;
  }
  val = (val << 3) + XDigit(c);
  Next();
  return val;
}


int Scanner::ScanUCN(int len) {
  assert(len == 4 || len == 8);
  int val = 0;
  for (auto i = 0; i < len; ++i) {
    auto c = Next();
    if (!isxdigit(c))
      Error(loc_, "expect xdigit, but got '%c'", c);
    val = (val << 4) + XDigit(c);
  }
  return val;
}


int Scanner::XDigit(int c) {
  switch (c) {
  case '0' ... '9': return c - '0';
  case 'a' ... 'z': return c - 'a';
  case 'A' ... 'Z': return c - 'A';
  default: assert(false); return c;
  }
}


Encoding Scanner::ScanEncoding(int c) {
  switch (c) {
  case 'u': return Try('8') ? Encoding::UTF8: Encoding::CHAR16;
  case 'U': return Encoding::CHAR32;
  case 'L': return Encoding::WCHAR;
  default: assert(false); return Encoding::NONE;
  }
}


std::string* ReadFile(const std::string& fileName) {
  FILE* f = fopen(fileName.c_str(), "r");
  if (!f) Error("%s: No such file or directory", fileName.c_str());
  auto text = new std::string;
  int c;
  while (EOF != (c = fgetc(f)))
      text->push_back(c);
  return text;
}


int Scanner::Next(void) {
  int c = *_p++;
  if (c == '\\' && *_p == '\n') {
    ++loc_._line;
    loc_._lineBegin = ++_p;
    return Next();
  } else if (c == '\n') {
    ++loc_._line;
    loc_._lineBegin = _p;
  }
  return c;
}

// There couldn't be more than one PutBack() that
// cross two line, so just leave lineBegin, because
// we never care about the pos of newline token
void Scanner::PutBack() {
  int c = *--_p;
  if (c == '\n' && _p[-1] == '\\') {
    --loc_._line;
    // lineBegin
    --_p;
    return PutBack();
  } else if (c == '\n') {
    --loc_._line;
  }
}


Token Scanner::MakeToken(int tag) const {
  auto begin = loc_._column - 1 + loc_._lineBegin;
  return {tag, loc_, std::string(begin, _p), ws_};
}


// New line is special
// It is generated before reading the character '\n'
Token Scanner::MakeNewLine() const {
  return {'\n', loc_, std::string(_p, _p + 1), ws_};
}
