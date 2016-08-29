#ifndef _WGTCC_SCANNER_H_
#define _WGTCC_SCANNER_H_

#include "error.h"
#include "encoding.h"
#include "token.h"

#include <string>
#include <cassert>


class Scanner {
public:
	explicit Scanner(std::string* text,
									 const std::string* fileName=nullptr,
									 unsigned line=1): _text(text)  {
		// TODO(wgtdkp): initialization


		_p = &(*_text)[0];

		_loc = {fileName, _p, line, 1};
		_tokBegin = _p;
	}

	virtual ~Scanner() {}
	Scanner(const Scanner& other) = delete;
	Scanner& operator=(const Scanner& other) = delete;

	// Scan plain text and generate tokens in ts.
	// The param 'ts' need not be empty, if so, the tokens
	// are inserted at the *header* of 'ts'.
	Token Scan();

private:
	Token ScanIdentifier();
	Token ScanNumber();
	Token ScanCharacter(Encoding enc);
	Token ScanLiteral(Encoding enc);
	Token MakeToken(int tag) const {
		return {tag, _loc, std::string(_tokBegin, _p)};
	}
	Encoding ScanEncoding(int c);
	int ScanEscaped();
	int ScanHexEscaped();
	int ScanOctEscaped();
	int ScanUCN(int len);
	void SkipWhiteSpace();
	void SkipComment();
	void UpdateLoc();
	bool IsUCN(int c) {
		return c == '\\' && (Test('u') || Test('U')); 
	}

	int XDigit(int c);
	bool IsOctal(int c) {
		return '0' <= c && c <= '7';
	}

	bool Empty() const { return *_p == 0; }
	int Peek() const { return *_p; }
	bool Test(int c) const { return Peek() == c; };
	int Next(void) { 
		++_loc._column;
		return *_p++;
	};
	
	void PutBack() {
		assert(_p >= &(*_text)[0]);
		--_loc._column; // Maybe handle '\n' ?
		--_p;
	}

	bool Try(int c) {
		if (Next() == c)
			return true;
		PutBack();
		return false;
	};

	void Mark() { _tokBegin = _p; };

	const std::string* _text;
	SourceLocation _loc;
	const char* _p;
	const char* _tokBegin;
};


std::string* ReadFile(const std::string& fileName);

#endif
