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
    _tokBegin = _p;
    tok_.loc_ = {fileName, _p, line, 1};
    ws_ = false;
  }

  virtual ~Scanner() {}
  Scanner(const Scanner& other) = delete;
  Scanner& operator=(const Scanner& other) = delete;

  // Scan plain text and generate tokens in ts.
  // The param 'ts' need not be empty, if so, the tokens
  // are inserted at the *header* of 'ts'.
  // The param 'ws' tells if there is leading white space
  // before this token, it is only SkipComment() that will 
  // set this param.
  Token Scan(bool ws=false);

private:
  Token ScanIdentifier();
  Token ScanNumber();
  //Token ScanCharacter(Encoding enc);
  //Token ScanLiteral(Encoding enc);
  Token ScanLiteral();
  Token ScanCharacter();
  Token MakeToken(int tag) const;
  Token MakeNewLine() const;
  Encoding ScanEncoding(int c);
  int ScanEscaped();
  int ScanHexEscaped();
  int ScanOctEscaped();
  int ScanUCN(int len);
  void SkipWhiteSpace();
  void SkipComment();
  bool IsUCN(int c) {
    return c == '\\' && (Test('u') || Test('U')); 
  }

  int XDigit(int c);
  bool IsOctal(int c) {
    return '0' <= c && c <= '7';
  }

  bool Empty() const { return *_p == 0; }
  int Peek(int offset=0) const {
    int c = _p[offset];
    if (c == '\\' && _p[offset + 1] == '\n')
      return Peek(offset + 2);
    return c;
  }

  bool Test(int c) const { return Peek() == c; };
  int Next(void);
  
  void PutBack();

  bool Try(int c) {
    if (Peek() == c) {
      Next();
      return true;
    }
    return false;
  };

  void Mark() {
    loc_._column = _p - loc_._lineBegin + 1;
  };

  const std::string* text_;
  const std::string* fileName_;
  const char* p_;
  const char* lineBegin_;
  unsigned column_;
  
};


std::string* ReadFile(const std::string& fileName);

#endif
