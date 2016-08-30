#include "cpp.h"

#include "evaluator.h"
#include "parser.h"

#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>


extern std::string inFileName;
extern std::string outFileName;

typedef std::unordered_map<std::string, int> DirectiveMap;

static const DirectiveMap directiveMap = {
  {"if", Token::PP_IF},
  {"ifdef", Token::PP_IFDEF},
  {"ifndef", Token::PP_IFNDEF},
  {"elif", Token::PP_ELIF},
  {"else", Token::PP_ELSE},
  {"endif", Token::PP_ENDIF},
  {"include", Token::PP_INCLUDE},
  {"define", Token::PP_DEFINE},
  {"undef", Token::PP_UNDEF},
  {"line", Token::PP_LINE},
  {"error", Token::PP_ERROR},
  {"pragma", Token::PP_PRAGMA}
};


/*
 * @params:
 *     is: input token sequence
 *     os: output token sequence
 */
void Preprocessor::Expand(TokenSequence& os, TokenSequence& is, bool inCond)
{
  Macro* macro = nullptr;
  int direcitve;
  while (!is.Empty()) {
    auto tok = is.Peek();
    UpdateTokenLocation(tok);
    const auto& name = tok->str_;

    if ((direcitve = GetDirective(is)) != Token::INVALID) {
      ParseDirective(os, is, direcitve);
    } else if (!inCond && !NeedExpand()) {
      // Discards 
      is.Next();
    } else if (tok->hs_ && tok->hs_->find(name) != tok->hs_->end()) {
      os.InsertBack(is.Next());
    } else if ((macro = FindMacro(name))) {
      is.Next();
      
      if (name == "__FILE__") {
        HandleTheFileMacro(os, tok);
        continue;
      } else if (name == "__LINE__") {
        HandleTheLineMacro(os, tok);
        continue;
      }
      // FIXME(wgtdkp): dead loop when expand below macro
      // #define stderr stderr
      if (macro->ObjLike()) {

        // Make a copy, as subst will change repSeq
        auto repSeq = macro->RepSeq(tok->loc_._fileName, tok->loc_._line);
        
        // The first token of the replcement sequence should 
        // derived the ws_ property of the macro to be replaced.
        // And the subsequent tokens' ws_ property should keep.
        // It could be dangerous to modify any property of 
        // the token after being generated.
        // But, as ws_ is just for '#' opreator, and the replacement 
        // sequence of a macro is only used to replace the macro, 
        // it should be safe here we change the ws_ property.
        repSeq.Peek()->ws_ = tok->ws_;
 
        TokenList tokList;
        TokenSequence repSeqSubsted(&tokList);
        ParamMap paramMap;
        // TODO(wgtdkp): hideset is not right
        // Make a copy of hideset
        // HS U {name}
        auto hs = tok->hs_ ? new HideSet(*tok->hs_): new HideSet();
        hs->insert(name);
        Subst(repSeqSubsted, repSeq, hs, paramMap);

        is.InsertFront(repSeqSubsted);
      } else if (is.Test('(')) {
        is.Next();

        ParamMap paramMap;
        auto rpar = ParseActualParam(is, macro, paramMap);

        auto repSeq = macro->RepSeq(tok->loc_._fileName, tok->loc_._line);
        repSeq.Peek()->ws_ = tok->ws_;
        TokenList tokList;
        TokenSequence repSeqSubsted(&tokList);

        // (HS ^ HS') U {name}
        // Use HS' U {name} directly                
        auto hs = rpar->hs_ ? new HideSet(*rpar->hs_): new HideSet();
        hs->insert(name);
        Subst(repSeqSubsted, repSeq, hs, paramMap);

        is.InsertFront(repSeqSubsted);
      } else {
        Error(is.Peek(), "expect '(' for func-like macro '%s'",
            name.c_str());
      }
      //hs_.erase(name);
    } else {
      os.InsertBack(is.Next());
    }
  }
}


static bool FindActualParam(TokenSequence& ap, ParamMap& params, const std::string& fp)
{
  auto res = params.find(fp);
  if (res == params.end()) {
    return false;
  }
  ap = res->second;
  return true;
}


void Preprocessor::Subst(TokenSequence& os, TokenSequence& is,
    HideSet* hs, ParamMap& params)
{
  TokenSequence ap;

  while (!is.Empty()) {
    if (is.Test('#') && FindActualParam(ap, params, is.Peek2()->str_)) {
      is.Next(); is.Next();

      auto tok = *(ap.Peek());
      tok.tag_ = Token::LITERAL;
      Stringize(tok.begin_, tok.end_, ap);

      os.InsertBack(&tok);
      //Subst(os, is, hs, params);
    } else if (is.Test(Token::DSHARP)
        && FindActualParam(ap, params, is.Peek2()->Str())) {
      is.Next();
      is.Next();
      
      if (ap.Empty()) {
        //Subst(os, is, hs, params);
      } else {
        Glue(os, ap);
        //Subst(os, is, hs, params);
      }
    } else if (is.Test(Token::DSHARP)) {
      is.Next();
      auto tok = is.Next();

      Glue(os, tok);
      //Subst(os, is, hs, params);
    } else if (is.Peek2()->tag_ == Token::DSHARP 
        && FindActualParam(ap, params, is.Peek()->Str())) {
      is.Next();

      if (ap.Empty()) {
        is.Next();
        if (FindActualParam(ap, params, is.Peek()->Str())) {
          is.Next();
          os.InsertBack(ap);
          //Subst(os, is, hs, params);
        } else {
          //Subst(os, is, hs, params);
        }
      } else {
        os.InsertBack(ap);
        //Subst(os, is, hs, params);
      }
    } else if (FindActualParam(ap, params, is.Peek()->Str())) {
      is.Next();
      Expand(os, ap);
      //Subst(os, is, hs, params);
    } else {
      os.InsertBack(is.Peek());
      is.Next();
      //Subst(os, is, hs, params);
    }
  }

  TokenSequence ts = os;
  while (!ts.Empty()) {
    ts.Next()->hs_ = hs;
  }
}


void Preprocessor::Glue(TokenSequence& os, Token* tok)
{
  TokenList tokList;
  tokList.push_back(*tok);
  TokenSequence is(&tokList);

  Glue(os, is);
}


void Preprocessor::Glue(TokenSequence& os, TokenSequence& is)
{
  auto lhs = os.Back();
  auto rhs = is.Peek();

  auto str = new std::string(lhs->Str() + rhs->Str());

  TokenSequence ts;
  Scanner lexer(str, lhs->_fileName, lhs->_line);
  lexer.Tokenize(ts);
  
  //--os.end_;
  is.Next();

  if (ts.Empty()) {
    // TODO(wgtdkp): 
    // No new Token generated
    // How to handle it???

  } else {
    // FIXME(wgtdkp): memory leakage
    //Token* tok = new Token(*lhs);
    Token* newTok = ts.Next();

    lhs->tag_ = newTok->tag_;
    lhs->begin_ = newTok->begin_;
    lhs->end_ = newTok->end_;
    //os.InsertBack(tok); // Copy once again in InsertBack() 
  }

  if (!ts.Empty()) {
    Error(lhs, "macro expansion failed: can't concatenate");
  }

  os.InsertBack(is);
}


/*
 * This is For the '#' operator in func-like macro
 */
std::string Preprocessor::Stringize(TokenSequence is)
{
  std::string str;
  while (!is.Empty()) {
    auto tok = is.Next();
    // Have preceding white space 
    // and is not the first token of the sequence
    str->append(tok->ws_ && str.size(), ' ');
    if (tok->tag_ == Token::LITERAL || tok->tag_) {
      if (tok->tag_ == Token::LITERAL)
        str.push_back('"');
      for (auto p = tok->begin_; p != tok->end_; p++) {
        if (*p == '"' || *p == '\\')
          str->push_back('\\');
        str->push_back(*p);
      }
      if 
    } else {
      str += tok->str_;
    }
  }
  return str;
}


/*
 * For debug: print a token sequence
 */
std::string Preprocessor::Stringize(TokenSequence is)
{
  std::string str;
  unsigned line = 1;

  while (!is.Empty()) {
    auto tok = is.Peek();
    
    if (is.IsBeginOfLine()) {
      str.push_back('\n');
      str.append(std::max(is.Peek()->_column - 2, 0U), ' ');
    }

    line = tok->_line;
    while (!is.Empty() && is.Peek()->_line == line) {
      tok = is.Next();
      str.append(tok->ws_, ' ');
      str.append(tok->begin_, tok->Size());
    }
  }
  return str;
}


void HSAdd(TokenSequence& ts, HideSet& hs)
{
  // TODO(wgtdkp): expand hideset
}


// TODO(wgtdkp): add predefined macros
void Preprocessor::Process(TokenSequence& os)
{
  TokenSequence is;

  // Add predefined
  Init();

  // Add source file
  IncludeFile(is, &inFileName);

  // Becareful about the include order, as include file always puts
  // the file to the header of the token sequence
  auto wgtccHeaderFile = SearchFile("wgtcc.h", true);
  IncludeFile(is, wgtccHeaderFile);

  //std::string str;
  //Stringize(str, is);
  //std::cout << str << std::endl;

  Expand(os, is);

  // Identify key word
  auto ts = os;
  while (!ts.Empty()) {
    auto tok = ts.Next();
    auto tag = Token::KeyWordTag(tok->begin_, tok->end_);
    if (Token::IsKeyWord(tag)) {
      tok->tag_ = tag;
    }
  }

  //str.resize(0);
  //Stringize(str, os);
  //std::cout << std::endl << "###### Preprocessed ######" << std::endl;
  //std::cout << str << std::endl << std::endl;
  //std::cout << std::endl << "###### End ######" << std::endl;
}


Token* Preprocessor::ParseActualParam(TokenSequence& is,
    Macro* macro, ParamMap& paramMap)
{
  Token* ret;
  //TokenSequence ts(is);
  TokenSequence ap(is);
  auto fp = macro->Params().begin();
  ap.begin_ = is.begin_;

  int cnt = 1;
  while (cnt > 0) {
    if (is.Empty())
      Error(is.Peek(), "premature end of input");
    else if (is.Test('('))
      ++cnt;
    else if (is.Test(')'))
      --cnt;
    
    if ((is.Test(',') && cnt == 1) || cnt == 0) {
      ap.end_ = is.begin_;

      if (fp == macro->Params().end()) {
        Error(is.Peek(), "too many params");
      }
      
      paramMap.insert(std::make_pair(*fp, ap));
      ++fp;

      ap.begin_ = ++ap.end_;  
    }

    ret = is.Next();
  }

  if (fp != macro->Params().end()) {
    // TODO(wgtdkp): error
    Error(is.Peek(), "too few params");
  }

  return ret;
}


void Preprocessor::ReplaceDefOp(TokenSequence& is)
{
  //assert(is.begin_ == is.tokList_->begin()
  //        && is.end_ == is.tokList_->end());

#define ERASE(iter) {                                   \
  auto tmp = iter;                                    \
  iter = is.tokList_->erase(iter);                    \
  if (tmp == is.begin_) {                             \
    is.begin_ = iter;                               \
  }                                                   \
  if (iter == is.end_) {                              \
    Error(&(*tmp), "unexpected end of line");       \
  }                                                   \
}

  for (auto iter = is.begin_; iter != is.end_; iter++) {
    if (iter->tag_== Token::IDENTIFIER && iter->Str() == "defined") {
      ERASE(iter);
      bool hasPar = false;
      if (iter->tag_ == '(') {
        hasPar = true;
        ERASE(iter);
      }

      if (iter->tag_ != Token::IDENTIFIER) {
        Error(&(*iter), "expect identifer in 'defined' operator");
      }
      
      auto name = iter->Str();

      if (hasPar) {
        ERASE(iter);
        if (iter->tag_ != ')') {
          Error(&(*iter), "expect ')'");
        }
      }

      iter->tag_ = Token::I_CONSTANT;
      iter->begin_ = const_cast<char*>(FindMacro(name) ? "1": "0");
      iter->end_ = iter->begin_ + 1;
    }
  }
#undef ERASE
}


void Preprocessor::ReplaceIdent(TokenSequence& is)
{
  for (auto iter = is.begin_; iter != is.end_; iter++) {
    if (iter->tag_ == Token::IDENTIFIER) {
      iter->tag_ = Token::I_CONSTANT;
      *iter->begin_ = '0';
      iter->end_ = iter->begin_ + 1;
    }
  }
}


TokenSequence Preprocessor::GetLine(TokenSequence& is)
{
  auto begin = is.begin_;
  while (!is.Empty() && is.begin_->_line == begin->_line)
    is.Next();
  auto end = is.begin_;
  return  TokenSequence(is.tokList_, begin, end);
}


int Preprocessor::GetDirective(TokenSequence& is)
{
  if (!is.Test('#') || !is.IsBeginOfLine())
    return Token::INVALID;

  is.Next();

  if (is.IsBeginOfLine())
    return Token::PP_EMPTY;

  auto tag = is.Peek()->tag_;
  if (tag == Token::IDENTIFIER || Token::IsKeyWord(tag)) {
    auto str = is.Peek()->Str();
    auto res = directiveMap.find(str);
    if (res == directiveMap.end()) {
      Error(is.Peek(), "'%s': unrecognized directive", str.c_str());
    }
    // Don't move on!!!!
    //is.Next();
    return res->second;
  } else {
    Error(is.Peek(), "'%s': unexpected directive",
        is.Peek()->Str().c_str());
  }
  return Token::INVALID;
}


void Preprocessor::ParseDirective(TokenSequence& os, TokenSequence& is, int directive)
{
  //while (directive != Token::INVALID) {
    if (directive == Token::PP_EMPTY)
      return;
    
    auto ls = GetLine(is);
    
    switch(directive) {
    case Token::PP_IF:
      ParseIf(ls); break;
    case Token::PP_IFDEF:
      ParseIfdef(ls); break;
    case Token::PP_IFNDEF:
      ParseIfndef(ls); break;
    case Token::PP_ELIF:
      ParseElif(ls); break;
    case Token::PP_ELSE:
      ParseElse(ls); break;
    case Token::PP_ENDIF:
      ParseEndif(ls); break;

    case Token::PP_INCLUDE:
      if (NeedExpand())
        ParseInclude(is, ls);
      break;
    case Token::PP_DEFINE:
      if (NeedExpand())
        ParseDef(ls);
      break;
    case Token::PP_UNDEF:
      if (NeedExpand())
        ParseUndef(ls);
      break;
    case Token::PP_LINE:
      if (NeedExpand())
        ParseLine(ls);
      break;
    case Token::PP_ERROR:
      if (NeedExpand())
        ParseError(ls);
      break;
    case Token::PP_PRAGMA:
      if (NeedExpand())
        ParsePragma(ls);
      break;

    default:
      assert(false);
    }

    //if (NeedExpand())
    //    os.InsertBack(ls);
}


void Preprocessor::ParsePragma(TokenSequence ls)
{
  ls.Next();

  // Leave it
  // TODO(wgtdkp):
}


void Preprocessor::ParseError(TokenSequence ls)
{
  ls.Next();
  
  std::string msg;
  Stringize(msg, ls);
  Error(ls.Peek(), "%s", msg.c_str());
}


void Preprocessor::ParseLine(TokenSequence ls)
{
  auto directive = ls.Next(); // Skip directive 'line'

  TokenSequence ts;
  Expand(ts, ls);

  auto tok = ts.Expect(Token::I_CONSTANT);

  int line = atoi(tok->begin_);
  if (line <= 0 || line > 0x7fffffff) {
    Error(tok, "illegal line number");
  }
  
  curLine_ = line;
  lineLine_ = directive->_line;

  if (ts.Empty())
    return;

  tok = ts.Expect(Token::LITERAL);
  
  // Enusure "s-char-sequence"
  if (*tok->begin_ != '"' || *(tok->end_ - 1) != '"') {
    Error(tok, "expect s-char-sequence");
  }
  
  //_curFileName = std::string(tok->begin_, tok->end_);
}


void Preprocessor::ParseIf(TokenSequence ls)
{
  if (!NeedExpand()) {
    ppCondStack_.push({Token::PP_IF, false, false});
    return;
  }

  auto tok = ls.Next(); // Skip the directive

  if (ls.Empty()) {
    Error(tok, "expect expression in 'if' directive");
  }

  TokenSequence ts;
  ReplaceDefOp(ls);
  Expand(ts, ls, true);
  ReplaceIdent(ts);

  //assert(!ts.Empty());
  //auto begin = ts.Peek();
  Parser parser(ts);
  auto expr = parser.ParseExpr();
  int cond = Evaluator<long>().Eval(expr);
  // TODO(wgtdkp): delete expr

  ppCondStack_.push({Token::PP_IF, NeedExpand(), cond});
}


void Preprocessor::ParseIfdef(TokenSequence ls)
{
  if (!NeedExpand()) {
    ppCondStack_.push({Token::PP_IFDEF, false, false});
    return;
  }

  ls.Next();
  
  auto ident = ls.Expect(Token::IDENTIFIER);
  if (!ls.Empty()) {
    Error(ls.Peek(), "expect new line");
  }

  int cond = FindMacro(ident->Str()) != nullptr;

  ppCondStack_.push({Token::PP_IFDEF, NeedExpand(), cond});
}


void Preprocessor::ParseIfndef(TokenSequence ls)
{
  ParseIfdef(ls);
  auto top = ppCondStack_.top();
  ppCondStack_.pop();
  top.tag_ = Token::PP_IFNDEF;
  top.cond_ = !top.cond_;
  
  ppCondStack_.push(top);
}


void Preprocessor::ParseElif(TokenSequence ls)
{
  auto directive = ls.Next(); // Skip the directive

  if (ppCondStack_.empty()) {
    Error(directive, "unexpected 'elif' directive");
  }

  auto top = ppCondStack_.top();
  if (top.tag_ == Token::PP_ELSE) {
    Error(directive, "unexpected 'elif' directive");
  }
  auto enabled = top.enabled_;
  if (!enabled) {
    ppCondStack_.push({Token::PP_ELIF, false, false});
    return;
  }

  if (ls.Empty()) {
    Error(ls.Peek(), "expect expression in 'elif' directive");
  }

  TokenSequence ts;
  ReplaceDefOp(ls);
  Expand(ts, ls, true);
  ReplaceIdent(ts);

  //auto begin = ts.Peek();
  Parser parser(ts);
  auto expr = parser.ParseExpr();
  int cond = Evaluator<long>().Eval(expr);

  cond = cond && !top.cond_;
  ppCondStack_.push({Token::PP_ELIF, true, cond});
}


void Preprocessor::ParseElse(TokenSequence ls)
{
  auto directive = ls.Next();
  if (!ls.Empty()) {
    Error(ls.Peek(), "expect new line");
  }

  if (ppCondStack_.empty()) {
    Error(directive, "unexpected 'else' directive");
  }
  auto top = ppCondStack_.top();
  if (top.tag_ == Token::PP_ELSE) {
    Error(directive, "unexpected 'elif' directive");
  }

  auto cond = !top.cond_;
  auto enabled = top.enabled_;
  ppCondStack_.push({Token::PP_ELSE, enabled, cond});
}


void Preprocessor::ParseEndif(TokenSequence ls)
{  
  auto directive = ls.Next();

  if (!ls.Empty()) {
    Error(ls.Peek(), "expect new line");
  }

  while ( !ppCondStack_.empty()) {
    auto top = ppCondStack_.top();
    ppCondStack_.pop();

    if (top.tag_ == Token::PP_IF
        || top.tag_ == Token::PP_IFDEF
        || top.tag_ == Token::PP_IFNDEF) {
      return;
    }
  }

  if (ppCondStack_.empty()) {
    Error(directive, "unexpected 'endif' directive");
  }
}


// Have Read the '#'
void Preprocessor::ParseInclude(TokenSequence& is, TokenSequence ls)
{
  ls.Next(); // Skip 'include'
  auto tok = ls.Next();
  if (tok->tag_ == Token::LITERAL) {
    if (!ls.Empty()) {
      Error(ls.Peek(), "expect new line");
    }
    // TODO(wgtdkp): include file
    auto fileName = tok->Literal();
    // FIXME(wgtdkp): memory leakage
    auto fullPath = SearchFile(fileName, false);
    if (fullPath == nullptr) {
      Error(tok, "%s: No such file or directory", fileName.c_str());
    }
    IncludeFile(is, fullPath);
  } else if (tok->tag_ == '<') {
    auto lhs = tok;
    auto rhs = tok;
    int cnt = 1;
    while (!(rhs = ls.Next())->IsEOF()) {
      if (rhs->tag_ == '<')
        ++cnt;
      else if (rhs->tag_ == '>')
        --cnt;
      if (cnt == 0)
        break;
    }
    if (cnt != 0) {
      Error(rhs, "expect '>'");
    }
    if (!ls.Empty()) {
      Error(ls.Peek(), "expect new line");
    }

    auto fileName = std::string(lhs->end_, rhs->begin_);
    auto fullPath = SearchFile(fileName, true);
    if (fullPath == nullptr) {
      Error(tok, "%s: No such file or directory", fileName.c_str());
    }
    IncludeFile(is, fullPath);
  } else {
    Error(tok, "expect filename(string or in '<>')");
  }
}


void Preprocessor::ParseUndef(TokenSequence ls)
{
  ls.Next(); // Skip directive

  auto ident = ls.Expect(Token::IDENTIFIER);
  if (!ls.Empty()) {
    Error(ls.Peek(), "expect new line");
  }

  RemoveMacro(ident->Str());
}


void Preprocessor::ParseDef(TokenSequence ls)
{
  ls.Next();
  auto ident = ls.Expect(Token::IDENTIFIER);

  auto tok = ls.Peek();
  if (tok->tag_ == '(' && tok->begin_ == ident->end_) {
    // There is no white space between ident and '('
    // Hence, we are defining function-like macro

    // Parse Identifier list
    ls.Next(); // Skip '('
    ParamList params;
    auto variadic = ParseIdentList(params, ls);
    AddMacro(ident->Str(), Macro(variadic, params, ls));
  } else {
    AddMacro(ident->Str(), Macro(ls));
  }
}


bool Preprocessor::ParseIdentList(ParamList& params, TokenSequence& is)
{
  Token* tok;
  while (!is.Empty()) {
    tok = is.Next();
    if (tok->tag_ == Token::ELLIPSIS) {
      is.Expect(')');
      return true;
    } else if (tok->tag_ != Token::IDENTIFIER) {
      Error(tok, "expect indentifier");
    }
    
    params.push_back(tok->Str());

    if (!is.Try(',')) {
      is.Expect(')');
      return false;
    }
  }

  Error(tok, "unexpected end of line");
  return false; // Make compiler happy
}


void Preprocessor::IncludeFile(TokenSequence& is, const std::string* fileName)
{
  TokenSequence ts(is.tokList_, is.begin_, is.begin_);
  Scanner lexer(ReadFile(*fileName), fileName);
  lexer.Tokenize(ts);
  
  // We done including header file
  is.begin_ = ts.begin_;
}


std::string* Preprocessor::SearchFile(const std::string& name, bool libHeader)
{
  PathList::iterator begin, end;
  if (libHeader) {
    auto iter = searchPathList_.begin();
    for (; iter != searchPathList_.end(); iter++) {
      auto dd = open(iter->c_str(), O_RDONLY);
      if (dd == -1) // TODO(wgtdkp): or ensure it before preprocessing
        continue;
      auto fd = openat(dd, name.c_str(), O_RDONLY);
      if (fd != -1) {
        close(fd);
        return new std::string(*iter + "/" + name);
      }
    }
  } else {
    auto iter = searchPathList_.rbegin();
    for (; iter != searchPathList_.rend(); iter++) {
      auto dd = open(iter->c_str(), O_RDONLY);
      if (dd == -1) // TODO(wgtdkp): or ensure it before preprocessing
        continue;
      auto fd = openat(dd, name.c_str(), O_RDONLY);
      if (fd != -1) {
        close(fd);
        return new std::string(*iter + "/" + name);
      }
    }
  }

  return nullptr;
}


void Preprocessor::AddMacro(const std::string& name,
    std::string* text, bool preDef)
{
  TokenSequence ts;
  Scanner lexer(text);
  lexer.Tokenize(ts);
  Macro macro(ts, preDef);

  AddMacro(name, macro);
}


static std::string* Date(void)
{
  time_t t = time(NULL);
  struct tm* tm = localtime(&t);
  auto buf = new char[14];
  strftime(buf, 14, "\"%a %M %Y\"", tm);
  auto ret = new std::string(buf);
  delete[] buf;
  return ret;
}


void Preprocessor::Init(void)
{
  // Preinclude search paths
  AddSearchPath("/usr/local/wgtcc/include");
  AddSearchPath("/usr/include");
  AddSearchPath("/usr/include/linux");
  AddSearchPath("/usr/include/x86_64-linux-gnu");
  AddSearchPath("/usr/local/include");
  
  // The __FILE__ and __LINE__ macro is empty
  // They are handled seperately
  AddMacro("__FILE__", Macro(TokenSequence(), true));
  AddMacro("__LINE__", Macro(TokenSequence(), true));

  AddMacro("__DATE__", Date(), true);
  AddMacro("__STDC__", new std::string("1"), true);
  AddMacro("__STDC__HOSTED__", new std::string("0"), true);
  AddMacro("__STDC_VERSION__", new std::string("201103L"), true);
}


void Preprocessor::HandleTheFileMacro(TokenSequence& os, Token* macro)
{
  Token file(*macro);
  file.tag_ = Token::LITERAL;

  auto str = new std::string("\"" + *macro->_fileName + "\"");
  file.begin_ = const_cast<char*>(str->c_str());
  file.end_ = file.begin_ + str->size();
  os.InsertBack(&file);
}


void Preprocessor::HandleTheLineMacro(TokenSequence& os, Token* macro)
{
  Token line(*macro);
  line.tag_ = Token::I_CONSTANT;
  auto str = new std::string(std::to_string(macro->_line));
  line.begin_ = const_cast<char*>(str->c_str());
  line.end_ = line.begin_ + str->size();
  os.InsertBack(&line);
}


void Preprocessor::UpdateTokenLocation(Token* tok)
{
  tok->_line = curLine_  + tok->_line - lineLine_ - 1;
}


TokenSequence Macro::RepSeq(const std::string* fileName, unsigned line)
{
  // Update line of replce token sequence
  TokenSequence ts = repSeq_;
  while (!ts.Empty()) {
    auto tok = ts.Next();
    tok->_fileName = fileName;
    tok->_line = line;
  }
  return repSeq_;
}
