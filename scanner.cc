#include "scanner.h"

#include <cctype>
#include <climits>


Token Scanner::Scan() {
	SkipWhiteSpace();

	Mark();
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
			return Scan();
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
	case 'u': case 'U': case 'L': {
		auto enc = ScanEncoding(c);
		if (Try('\'')) return ScanCharacter(enc);
		if (Try('\"')) return ScanLiteral(enc);
		return ScanIdentifier();
	}
	case '\'': return ScanCharacter(Encoding::NONE);
	case '\"': return ScanLiteral(Encoding::NONE);
	case 'a' ... 't': case 'v' ... 'z': case 'A' ... 'K':
	case 'M' ... 'T': case 'V' ... 'Z': case '_': case '$':
	case 0x80 ... 0xfd:
		return ScanIdentifier();
	case '\\':
		// Drop back slash and newline
		//if (Try('\n')) return Scan();
		// Universal character name is allowed in identifier
		if (Peek() == 'u' || Peek() == 'U')
			return ScanIdentifier();
		Error(_loc, "stray '%c' in program", c);
	case '0' ... '9': return ScanNumber();
	case EOF:	return MakeToken(Token::END);
	default: Error(_loc, "stray '%c' in program", c);
	}
	return MakeToken(Token::END); // Make Compiler happy
}


void Scanner::SkipWhiteSpace(void) {
	while (isspace(Peek())) {
		if (Peek() == '\n') { 
			UpdateLoc();
		}
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
	}

	while (!Empty()) {
		auto c = Next();
		if (c  == '*' && Peek() == '/') {
			Next();
			return;
		} else if (c == '\n') {
			UpdateLoc();
		}
	}
	Error(_loc, "unterminated block comment");
}


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
	return {Token::LITERAL, _loc, str};
}

// Scan PP-Number 
Token Scanner::ScanNumber() {
	int tag = Token::I_CONSTANT;	
	auto c = Next();
	while (c == '.' || isdigit(c) || isalpha(c) || c == '_' || IsUCN(c)) {
		if (c == 'e' || c =='E' || c == 'p' || c == 'P') {
			Try('-');
			Try('+');
			tag = Token::F_CONSTANT;
		}  else if (IsUCN(c)) {
			ScanEscaped();
		} else if (c == '.') {
			tag = Token::F_CONSTANT;
		}
		c = Next();
	}
	PutBack();
	return MakeToken(tag);
}

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
		Error(_loc, "unterminated string literal");
	str.push_back(static_cast<int>(enc));
	return {Token::LITERAL, _loc, str};
}

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
		Error(_loc, "unterminated character constant");
	if (enc == Encoding::CHAR16)
		val &= USHRT_MAX;
	Append32(str, val);
	str.push_back(static_cast<int>(enc));
	return {Token::C_CONSTANT, _loc, str};
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
	default: Error(_loc, "unrecognized escape character '%c'", c);
	}
	return c; // Make compiler happy
}


int Scanner::ScanHexEscaped() {
	int val = 0, c = Next();
	if (!isxdigit(c))
		Error(_loc, "expect xdigit, but got '%c'", c);
	while (isxdigit(c)) {
		val = (val << 4) + XDigit(c);
		c = Next();
	}
	PutBack();
	return val;
}


int Scanner::ScanOctEscaped() {
	int val = 0, c = Next();
	if (!IsOctal(c))
		Error(_loc, "expect octal, but got '%c'", c);
	val = (val << 3) + XDigit(c);
	c = Next();
	if (!IsOctal(c)) {
		PutBack();
		return val;
	}
	val = (val << 3) + XDigit(c);
	c = Next();
	if (!IsOctal(c)) {
		PutBack();
		return val;
	}
	val = (val << 3) + XDigit(c);
	return val;
}


int Scanner::ScanUCN(int len) {
	assert(len == 4 || len == 8);
	int val = 0;
	for (auto i = 0; i < len; ++i) {
		auto c = Next();
		if (!isxdigit(c))
			Error(_loc, "expect xdigit, but got '%c'", c);
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


void Scanner::UpdateLoc() {
		++_loc._line;
		_loc._column = 1;
		_loc._lineBegin = _p + 1;
}


std::string* ReadFile(const std::string& fileName) {
	FILE* f = fopen(fileName.c_str(), "r");
	if (!f) Error("%s: No such file or directory", fileName.c_str());
	auto text = new std::string;
	int c;
	while (EOF != (c = fgetc(f))) {
		if (c == '\n' && text->size() && text->back() == '\\')
			text->pop_back();
		else 
			text->push_back(c);
	}
	return text;
}
