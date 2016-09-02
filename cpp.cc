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
  // Non-standard GNU extension
  {"include_next", Token::PP_INCLUDE},
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
void Preprocessor::Expand(TokenSequence& os, TokenSequence is, bool inCond)
{
  Macro* macro = nullptr;
  int direcitve;
  while (!is.Empty()) {
    UpdateFirstTokenLine(is);
    auto tok = is.Peek();
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
        auto repSeq = macro->RepSeq(tok->loc_.fileName_, tok->loc_.line_);
        
        // The first token of the replcement sequence should 
        // derived the ws_ property of the macro to be replaced.
        // And the subsequent tokens' ws_ property should keep.
        // It could be dangerous to modify any property of 
        // the token after being generated.
        // But, as ws_ is just for '#' opreator, and the replacement 
        // sequence of a macro is only used to replace the macro, 
        // it should be safe here we change the ws_ property.
        
        // TODO(wgtdkp):
        //auto firstTok = Token::New(*repSeq.Next());
        
        const_cast<Token*>(repSeq.Peek())->ws_ = tok->ws_;
 
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

        auto repSeq = macro->RepSeq(tok->loc_.fileName_, tok->loc_.line_);
        const_cast<Token*>(repSeq.Peek())->ws_ = tok->ws_;
        TokenList tokList;
        TokenSequence repSeqSubsted(&tokList);

        // (HS ^ HS') U {name}
        // Use HS' U {name} directly                
        auto hs = rpar->hs_ ? new HideSet(*rpar->hs_): new HideSet();
        hs->insert(name);
        Subst(repSeqSubsted, repSeq, hs, paramMap);

        is.InsertFront(repSeqSubsted);
      } else {
        os.InsertBack(tok);
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
  ap.Copy(res->second);
  return true;
}


void Preprocessor::Subst(TokenSequence& os, TokenSequence is,
                         HideSet* hs, ParamMap& params)
{
  TokenSequence ap;

  while (!is.Empty()) {
    if (is.Test('#') && FindActualParam(ap, params, is.Peek2()->str_)) {
      is.Next(); is.Next();
      auto tok = Token::New(*ap.Peek());
      tok->tag_ = Token::LITERAL;
      tok->str_ = Stringize(ap);
      os.InsertBack(tok);
    } else if (is.Test(Token::DSHARP)
        && FindActualParam(ap, params, is.Peek2()->str_)) {
      is.Next(); is.Next();
      if (!ap.Empty())
        Glue(os, ap);
    } else if (is.Test(Token::DSHARP)) {
      is.Next();
      auto tok = is.Next();
      Glue(os, tok);
    } else if (is.Peek2()->tag_ == Token::DSHARP 
        && FindActualParam(ap, params, is.Peek()->str_)) {
      is.Next();

      if (ap.Empty()) {
        is.Next();
        if (FindActualParam(ap, params, is.Peek()->str_)) {
          is.Next();
          os.InsertBack(ap);
        }
      } else {
        os.InsertBack(ap);
      }
    } else if (FindActualParam(ap, params, is.Peek()->str_)) {
      is.Next();
      Expand(os, ap);
    } else {
      os.InsertBack(is.Peek());
      is.Next();
    }
  }

  os.UpdateHideSet(hs);
}


void Preprocessor::Glue(TokenSequence& os, const Token* tok)
{
  TokenList tokList {tok};
  TokenSequence is(&tokList);
  Glue(os, is);
}


void Preprocessor::Glue(TokenSequence& os, TokenSequence is)
{
  auto lhs = os.Back();
  auto rhs = is.Peek();

  auto str = new std::string(lhs->str_ + rhs->str_);

  TokenSequence ts;
  Scanner scanner(str, lhs->loc_);
  scanner.Tokenize(ts);
  
  //--os.end_;
  is.Next();

  if (ts.Empty()) {
    // TODO(wgtdkp): 
    // No new Token generated
    // How to handle it???
  } else {
    os.PopBack();
    os.InsertBack(ts.Next());
    //Token* newTok = ts.Next();
    //lhs->tag_ = newTok->tag_;
    //lhs->str_ = newTok->str_;
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
  std::string str = "\"";
  while (!is.Empty()) {
    auto tok = is.Next();
    // Have preceding white space 
    // and is not the first token of the sequence
    str.append(tok->ws_ && str.size(), ' ');
    if (tok->tag_ == Token::LITERAL || tok->tag_) {
      for (auto c: tok->str_) {
        if (c == '"' || c == '\\')
          str.push_back('\\');
        str.push_back(c);
      }
    } else {
      str += tok->str_;
    }
  }
  str.push_back('\"');
  return str;
}

/*
 * For debug: print a token sequence
 */
/*
std::string Preprocessor::Stringize(TokenSequence is)
{
  std::string str;
  unsigned line = 1;

  while (!is.Empty()) {
    auto tok = is.Peek();
    
    if (is.IsBeginOfLine()) {
      str.push_back('\n');
      str.append(std::max(is.Peek()->column_ - 2, 0U), ' ');
    }

    line = tok->line_;
    while (!is.Empty() && is.Peek()->line_ == line) {
      tok = is.Next();
      str.append(tok->ws_, ' ');
      str.append(tok->begin_, tok->Size());
    }
  }
  return str;
}
*/

void HSAdd(TokenSequence& ts, HideSet& hs)
{
  // TODO(wgtdkp): expand hideset
}


void Preprocessor::Finalize(TokenSequence os)
{
  while (!os.Empty()) {
    auto tok = os.Next();
    if (tok->tag_ == Token::INVALID) {
      Error(tok, "stray token in program");
    } else if (tok->tag_ == Token::IDENTIFIER) {
      auto tag = Token::KeyWordTag(tok->str_);
      if (Token::IsKeyWord(tag)) {
        const_cast<Token*>(tok)->tag_ = tag;
      } else {
        const_cast<Token*>(tok)->str_ = Scanner(tok).ScanIdentifier();
      }
    }
    if (!tok->loc_.fileName_) {
      assert(false);
    }
  }
}


// TODO(wgtdkp): add predefined macros
void Preprocessor::Process(TokenSequence& os)
{
  TokenSequence is;

  // Add source file
  IncludeFile(is, &inFileName);

  // Becareful about the include order, as include file always puts
  // the file to the header of the token sequence
  auto wgtccHeaderFile = SearchFile("wgtcc.h", true, false);
  IncludeFile(is, wgtccHeaderFile);

  //std::string str;
  //Stringize(str, is);
  //std::cout << str << std::endl;
  //is.Print();
  Expand(os, is);

  // Identify key word
  // TODO(wgtdkp): finalize
  
  Finalize(os);


  //str.resize(0);
  //Stringize(str, os);
  //std::cout << std::endl << "###### Preprocessed ######" << std::endl;
  //std::cout << str << std::endl << std::endl;
  //std::cout << std::endl << "###### End ######" << std::endl;
}


const Token* Preprocessor::ParseActualParam(TokenSequence& is,
    Macro* macro, ParamMap& paramMap)
{
  const Token* ret;
  if (macro->Params().size() == 0 && !macro->Variadic()) {
    ret = is.Next();
    if (ret->tag_ != ')')
      Error(ret, "too many arguments");
    return ret;
  }

  //TokenSequence ts(is);
  //TokenSequence ap(is);
  auto fp = macro->Params().begin();
  //ap.begin_ = is.begin_;
  TokenSequence ap;

  int cnt = 1;
  while (cnt > 0) {
    if (is.Empty())
      Error(is.Peek(), "premature end of input");
    else if (is.Test('('))
      ++cnt;
    else if (is.Test(')'))
      --cnt;
    
    if ((is.Test(',') && cnt == 1) || cnt == 0) {
      //ap.end_ = is.begin_;

      if (fp == macro->Params().end()) {
        if (!macro->Variadic())
          Error(is.Peek(), "too many arguments");
        if (cnt == 0)
          paramMap.insert(std::make_pair("__VA_ARGS__", ap));
      } else {
        paramMap.insert(std::make_pair(*fp, ap));
        //ap.begin_ = ++ap.end_;
        ap = TokenSequence();
        ++fp;
      }
    } else {
      ap.InsertBack(is.Peek());
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
  auto tmp = iter;                                      \
  iter = is.tokList_->erase(iter);                      \
  if (tmp == is.begin_) {                               \
    is.begin_ = iter;                                   \
  }                                                     \
  if (iter == is.end_) {                                \
    Error(&(*tmp), "unexpected end of line");           \
  }                                                     \
};

  TokenSequence os;
  while (!is.Empty()) {
    auto tok = is.Next();
    if (tok->tag_ == Token::IDENTIFIER && tok->str_ == "defined") {
      auto hasPar = false;
      if (is.Try('(')) hasPar = true;
      tok = is.Expect(Token::IDENTIFIER);
      auto cons = Token::New(*tok);
      if (hasPar) is.Expect(')');
      cons->tag_ = Token::I_CONSTANT;
      cons->str_ = FindMacro(tok->str_) ? "1": "0";
      os.InsertBack(cons);
    } else {
      os.InsertBack(tok);
    } 
  }

  is = os;
  /*
  for (auto iter = is.begin_; iter != is.end_; ++iter) {
    if (iter->tag_== Token::IDENTIFIER && iter->str_ == "defined") {
      ERASE(iter);
      bool hasPar = false;
      if (iter->tag_ == '(') {
        hasPar = true;
        ERASE(iter);
      }

      if (iter->tag_ != Token::IDENTIFIER) {
        Error(&(*iter), "expect identifier in 'defined' operator");
      }
      
      auto name = iter->str_;

      if (hasPar) {
        ERASE(iter);
        if (iter->tag_ != ')') {
          Error(&(*iter), "expect ')'");
        }
      }

      iter->tag_ = Token::I_CONSTANT;
      iter->str_ = FindMacro(name) ? "1": "0";
    }
  }
  */
#undef ERASE
}


void Preprocessor::ReplaceIdent(TokenSequence& is)
{
  TokenSequence os;
  while (!is.Empty()) {
    auto tok = is.Next();
    if (tok->tag_ == Token::IDENTIFIER) {
      auto cons = Token::New(*tok);
      cons->tag_ = Token::I_CONSTANT;
      cons->str_ = "0";
      os.InsertBack(cons);
    } else {
      os.InsertBack(tok);
    }
  }
  is = os;
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
    auto str = is.Peek()->str_;
    auto res = directiveMap.find(str);
    if (res == directiveMap.end()) {
      Error(is.Peek(), "'%s': unrecognized directive", str.c_str());
    }
    // Don't move on!!!!
    //is.Next();
    return res->second;
  } else {
    Error(is.Peek(), "'%s': unexpected directive",
        is.Peek()->str_.c_str());
  }
  return Token::INVALID;
}


void Preprocessor::ParseDirective(TokenSequence& os, TokenSequence& is, int directive)
{
  //while (directive != Token::INVALID) {
    if (directive == Token::PP_EMPTY)
      return;
    
    auto ls = is.GetLine();
    
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
  
  const auto& msg = Stringize(ls);
  Error(ls.Peek(), "%s", msg.c_str());
}


void Preprocessor::ParseLine(TokenSequence ls)
{
  auto directive = ls.Next(); // Skip directive 'line'
  TokenSequence ts;
  Expand(ts, ls);
  auto tok = ts.Expect(Token::I_CONSTANT);

  int line;
  size_t end;
  try {
    line = stoi(tok->str_, &end, 10);
  } catch (const std::out_of_range oor) {
    Error(tok, "line number out of range");
  }
  if (line == 0 || end != tok->str_.size()) {
    Error(tok, "illegal line number");
  }
  
  curLine_ = line;
  lineLine_ = directive->loc_.line_;
  if (ts.Empty())
    return;
  tok = ts.Expect(Token::LITERAL);
  
  // Enusure "s-char-sequence"
  if (tok->str_.front() != '"' || tok->str_.back() != '"') {
    Error(tok, "expect s-char-sequence");
  }
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

  Parser parser(ts);
  auto expr = parser.ParseExpr();
  int cond = Evaluator<long>().Eval(expr);
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

  int cond = FindMacro(ident->str_) != nullptr;

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
    Error(directive, "unexpected 'else' directive");
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
  bool next = ls.Next()->str_ == "include_next"; // Skip 'include'
  auto tok = ls.Next();
  if (tok->tag_ == Token::LITERAL) {
    if (!ls.Empty()) {
      Error(ls.Peek(), "expect new line");
    }
    std::string fileName;
    Scanner(tok).ScanLiteral(fileName);
    auto fullPath = SearchFile(fileName, false, next, tok->loc_.fileName_);
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

    const auto& fileName = Scanner::ScanHeadName(lhs, rhs);
    auto fullPath = SearchFile(fileName, true, next, tok->loc_.fileName_);
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

  RemoveMacro(ident->str_);
}


void Preprocessor::ParseDef(TokenSequence ls)
{
  ls.Next();
  auto ident = ls.Expect(Token::IDENTIFIER);

  auto tok = ls.Peek();
  if (tok->tag_ == '(' && !tok->ws_) {
    // There is no white space between ident and '('
    // Hence, we are defining function-like macro

    // Parse Identifier list
    ls.Next(); // Skip '('
    ParamList params;
    auto variadic = ParseIdentList(params, ls);
    AddMacro(ident->str_, Macro(variadic, params, ls));
  } else {
    AddMacro(ident->str_, Macro(ls));
  }
}


bool Preprocessor::ParseIdentList(ParamList& params, TokenSequence& is)
{
  const Token* tok;
  while (!is.Empty()) {
    tok = is.Next();
    if (tok->tag_ == ')') {
      return false;
    } else if (tok->tag_ == Token::ELLIPSIS) {
      is.Expect(')');
      return true;
    } else if (tok->tag_ != Token::IDENTIFIER) {
      Error(tok, "expect identifier");
    }

    for (const auto& param: params) {
      if (param == tok->str_)
        Error(tok, "duplicated param");
    }
    params.push_back(tok->str_);

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
  TokenSequence ts {is.tokList_, is.begin_, is.begin_};
  Scanner scanner(ReadFile(*fileName), fileName);
  scanner.Tokenize(ts);
  
  // We done including header file
  is.begin_ = ts.begin_;
}


std::string* Preprocessor::SearchFile(
    const std::string& name,
    bool libHeader,
    bool next,
    const std::string* curPath)
{
  PathList::iterator begin, end;
  if (libHeader && !next) {
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
        auto path = *iter + name;
        if (next) {
          assert(curPath);
          if (path != *curPath)
            continue;
          else 
            next = false;
        } else {
          return new std::string(path);
        }
      }
    }
  }

  return nullptr;
}


void Preprocessor::AddMacro(const std::string& name,
    std::string* text, bool preDef)
{
  TokenSequence ts;
  Scanner scanner(text);
  scanner.Tokenize(ts);
  Macro macro(ts, preDef);

  AddMacro(name, macro);
}


static std::string* Date()
{
  time_t t = time(NULL);
  struct tm* tm = localtime(&t);
  auto buf = new char[14];
  strftime(buf, 14, "\"%a %M %Y\"", tm);
  auto ret = new std::string(buf);
  delete[] buf;
  return ret;
}


void Preprocessor::Init()
{
  // Preinclude search paths
  AddSearchPath("/usr/local/wgtcc/include/");
  AddSearchPath("/usr/include/");
  AddSearchPath("/usr/include/linux/");
  AddSearchPath("/usr/include/x86_64-linux-gnu/");
  AddSearchPath("/usr/local/include/");
  
  // The __FILE__ and __LINE__ macro is empty
  // They are handled seperately
  AddMacro("__FILE__", Macro(TokenSequence(), true));
  AddMacro("__LINE__", Macro(TokenSequence(), true));

  AddMacro("__DATE__", Date(), true);
  AddMacro("__STDC__", new std::string("1"), true);
  AddMacro("__STDC__HOSTED__", new std::string("0"), true);
  AddMacro("__STDC_VERSION__", new std::string("201103L"), true);
}


void Preprocessor::HandleTheFileMacro(TokenSequence& os, const Token* macro)
{
  auto file = Token::New(*macro);
  file->tag_ = Token::LITERAL;
  file->str_ = "\"" + *macro->loc_.fileName_ + "\"";
  os.InsertBack(file);
}


void Preprocessor::HandleTheLineMacro(TokenSequence& os, const Token* macro)
{
  auto line = Token::New(*macro);
  line->tag_ = Token::I_CONSTANT;
  line->str_ = std::to_string(macro->loc_.line_);
  os.InsertBack(line);
}


void Preprocessor::UpdateFirstTokenLine(TokenSequence ts)
{
  //UpdateFirstTokenLine
  auto loc = ts.Peek()->loc_;
  loc.line_ = curLine_  + loc.line_ - lineLine_ - 1;
  ts.UpdateHeadLocation(loc);
}


TokenSequence Macro::RepSeq(const std::string* fileName, unsigned line)
{
  // Update line
  TokenList tl;
  TokenSequence ret(&tl);
  ret.Copy(repSeq_);
  auto ts = ret;
  while (!ts.Empty()) {
    auto loc = ts.Peek()->loc_;
    loc.fileName_ = fileName;
    loc.line_ = line;
    ts.UpdateHeadLocation(loc);
    ts.Next();
  }
  return ret;
}

/*
TokenSequence Macro::RepSeq(const SourceLocation& loc)
{
  auto ts = repSeq_;
  if (ts.Empty())
    return ts;
  auto beginLoc = ts.Peek()->loc_;
  while (!ts.Empty()) {
    auto tok = ts.Next();
    tok->loc_.fileName_ = loc.fileName_;
    tok->loc_.line_ = loc.line_;

  }
}
*/

void Preprocessor::AddSearchPath(const std::string& path)
{
  //if (path != "/" && path.back() == '/')
  //  path.pop_back();
  searchPathList_.push_back(path);
}
