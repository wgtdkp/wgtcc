#include "cpp.h"

#include "evaluator.h"
#include "parser.h"

#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>


extern std::string in_filename;
extern std::string out_filename;

using DirectiveMap = std::unordered_map<std::string, int>;

static const DirectiveMap directive_map {
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
 * param_list:
 *  is: input token sequence
 *  os: output token sequence
 */
void Preprocessor::Expand(TokenSequence& os, TokenSequence is, bool in_cond) {
  Macro* macro = nullptr;
  int direcitve;
  while (!is.Empty()) {
    UpdateFirstTokenLine(is);
    auto tok = is.Peek();
    const auto& name = tok->str();

    if ((direcitve = GetDirective(is)) != Token::INVALID) {
      ParseDirective(os, is, direcitve);
    } else if (!in_cond && !NeedExpand()) {
      // Discards the token
      is.Next();
    } else if (tok->hs() && tok->hs()->find(name) != tok->hs()->end()) {
      os.InsertBack(is.Next());
    } else if ((macro = FindMacro(name))) {
      is.Next();

      if (name == "__FILE__") {
        HandleTheFileMacro(os, tok);
      } else if (name == "__LINE__") {
        HandleTheLineMacro(os, tok);
      } else if (macro->obj_like()) {
        // Make a copy, as subst will change rep_seq
        auto rep_seq = macro->pep_seq(tok->loc().filename, tok->loc().line);

        TokenList tok_list;
        TokenSequence rep_seq_substed(&tok_list);
        ParamMap param_map;
        // TODO(wgtdkp): hideset is not right
        // Make a copy of hideset
        // HS U {name}
        auto hs = tok->hs() ? *tok->hs(): HideSet();
        hs.insert(name);
        Subst(rep_seq_substed, rep_seq, tok->ws(), hs, param_map);
        is.InsertFront(rep_seq_substed);
      } else if (is.Try('(')) {
        ParamMap param_map;
        auto rpar = ParseActualParam(is, macro, param_map);
        auto rep_seq = macro->pep_seq(tok->loc().filename, tok->loc().line);
        //const_cast<Token*>(rep_seq.Peek())->ws() = tok->ws();
        TokenList tok_list;
        TokenSequence rep_seq_substed(&tok_list);

        // (HS ^ HS') U {name}
        // Use HS' U {name} directly                
        auto hs = rpar->hs() ? *rpar->hs(): HideSet();
        hs.insert(name);
        Subst(rep_seq_substed, rep_seq, tok->ws(), hs, param_map);
        is.InsertFront(rep_seq_substed);
      } else {
        os.InsertBack(tok);
      }
    } else {
      os.InsertBack(is.Next());
    }
  }
}


static bool FindActualParam(TokenSequence& ap,
                            ParamMap& param_list,
                            const std::string& fp) {
  auto res = param_list.find(fp);
  if (res == param_list.end()) {
    return false;
  }
  ap.Copy(res->second);
  return true;
}


void Preprocessor::Subst(TokenSequence& os,
                         TokenSequence is,
                         bool leading_ws,
                         const HideSet& hs,
                         ParamMap& param_list) {
  TokenSequence ap;

  while (!is.Empty()) {
    if (is.Test('#') && FindActualParam(ap, param_list, is.Peek2()->str())) {
      is.Next(); is.Next();
      auto tok = Stringize(ap);
      os.InsertBack(tok);
    } else if (is.Test(Token::DSHARP)
        && FindActualParam(ap, param_list, is.Peek2()->str())) {
      is.Next(); is.Next();
      if (!ap.Empty())
        Glue(os, ap);
    } else if (is.Test(Token::DSHARP)) {
      is.Next();
      auto tok = is.Next();
      Glue(os, tok);
    } else if (is.Peek2()->tag() == Token::DSHARP 
        && FindActualParam(ap, param_list, is.Peek()->str())) {
      is.Next();

      if (ap.Empty()) {
        is.Next();
        if (FindActualParam(ap, param_list, is.Peek()->str())) {
          is.Next();
          os.InsertBack(ap);
        }
      } else {
        os.InsertBack(ap);
      }
    } else if (FindActualParam(ap, param_list, is.Peek()->str())) {
      auto tok = is.Next();
      const_cast<Token*>(ap.Peek())->set_ws(tok->ws());
      Expand(os, ap);
    } else {
      os.InsertBack(is.Peek());
      is.Next();
    }
  }

  os.FinalizeSubst(leading_ws, hs);
}


void Preprocessor::Glue(TokenSequence& os, const Token* tok) {
  TokenList tok_list {tok};
  TokenSequence is(&tok_list);
  Glue(os, is);
}


void Preprocessor::Glue(TokenSequence& os, TokenSequence is) {
  auto lhs = os.Back();
  auto rhs = is.Peek();

  auto str = new std::string(lhs->str() + rhs->str());
  TokenSequence ts;
  Scanner scanner(str, lhs->loc());
  scanner.Tokenize(ts);

  is.Next();

  if (ts.Empty()) {
    // TODO(wgtdkp): 
    // No new Token generated
    // How to handle it???
  } else {
    os.PopBack();
    auto new_tok = const_cast<Token*>(ts.Next());
    new_tok->set_ws(lhs->ws());
    new_tok->set_hs(const_cast<HideSet*>(lhs->hs()));
    os.InsertBack(new_tok);
  }

  if (!ts.Empty()) {
    Error(lhs, "macro expansion failed: cannot concatenate");
  }

  os.InsertBack(is);
}


/*
 * This is For the '#' operator in func-like macro
 */
const Token* Preprocessor::Stringize(TokenSequence is) {
  std::string str = "\"";
  while (!is.Empty()) {
    auto tok = is.Next();
    // Have preceding white space
    // and is not the first token of the sequence
    str.append(tok->ws() && str.size() > 1, ' ');
    if (tok->tag() == Token::LITERAL || tok->tag() == Token::C_CONSTANT) {
      for (auto c: tok->str()) {
        if (c == '"' || c == '\\')
          str.push_back('\\');
        str.push_back(c);
      }
    } else {
      str += tok->str();
    }
  }
  str.push_back('\"');

  auto ret = Token::New(*is.Peek());
  ret->set_tag(Token::LITERAL);
  ret->set_str(str);
  return ret;
}


void Preprocessor::Finalize(TokenSequence os) {
  while (!os.Empty()) {
    auto tok = os.Next();
    if (tok->tag() == Token::INVALID) {
      Error(tok, "stray token in program");
    } else if (tok->tag() == Token::IDENTIFIER) {
      auto tag = Token::KeyWordTag(tok->str());
      if (Token::IsKeyWord(tag)) {
        const_cast<Token*>(tok)->set_tag(tag);
      } else {
        const_cast<Token*>(tok)->set_str(Scanner(tok).ScanIdentifier());
      }
    }
    if (!tok->loc().filename) {
      assert(false);
    }
  }
}


// TODO(wgtdkp): add predefined macros
void Preprocessor::Process(TokenSequence& os) {
  TokenSequence is;

  // Add source file
  IncludeFile(is, &in_filename);

  // Becareful about the include order, as include file always puts
  // the file to the header of the token sequence
  auto wgtcc_header_file = SearchFile("wgtcc.h", true, false, in_filename);
  if (!wgtcc_header_file)
    Error("can't find header files, try reinstall wgtcc");
  IncludeFile(is, wgtcc_header_file);
  Expand(os, is);
  Finalize(os);
}


const Token* Preprocessor::ParseActualParam(TokenSequence& is,
                                            Macro* macro,
                                            ParamMap& param_map) {
  const Token* ret;
  if (macro->param_list().size() == 0 && !macro->variadic()) {
    ret = is.Next();
    if (ret->tag() != ')')
      Error(ret, "too many arguments");
    return ret;
  }

  //TokenSequence ts(is);
  //TokenSequence ap(is);
  auto fp = macro->param_list().begin();
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

      if (fp == macro->param_list().end()) {
        if (!macro->variadic())
          Error(is.Peek(), "too many arguments");
        if (cnt == 0)
          param_map.insert(std::make_pair("__VA_ARGS__", ap));
        else
          ap.InsertBack(is.Peek());
      } else {
        param_map.insert(std::make_pair(*fp, ap));
        ap = TokenSequence();
        ++fp;
      }
    } else {
      ap.InsertBack(is.Peek());
    }
    ret = is.Next();
  }

  if (fp != macro->param_list().end())
    Error(is.Peek(), "too few param_list");
  return ret;
}


void Preprocessor::ReplaceDefOp(TokenSequence& is) {
  TokenSequence os;
  while (!is.Empty()) {
    auto tok = is.Next();
    if (tok->tag() == Token::IDENTIFIER && tok->str() == "defined") {
      auto hasPar = false;
      if (is.Try('(')) hasPar = true;
      tok = is.Expect(Token::IDENTIFIER);
      auto cons = Token::New(*tok);
      if (hasPar) is.Expect(')');
      cons->set_tag(Token::I_CONSTANT);
      cons->set_str(FindMacro(tok->str()) ? "1": "0");
      os.InsertBack(cons);
    } else {
      os.InsertBack(tok);
    } 
  }
  is = os;
}


void Preprocessor::ReplaceIdent(TokenSequence& is) {
  TokenSequence os;
  while (!is.Empty()) {
    auto tok = is.Next();
    if (tok->tag() == Token::IDENTIFIER) {
      auto cons = Token::New(*tok);
      cons->set_tag(Token::I_CONSTANT);
      cons->set_str("0");
      os.InsertBack(cons);
    } else {
      os.InsertBack(tok);
    }
  }
  is = os;
}


int Preprocessor::GetDirective(TokenSequence& is) {
  if (!is.Test('#') || !is.IsBeginOfLine())
    return Token::INVALID;

  is.Next();
  if (is.IsBeginOfLine())
    return Token::PP_EMPTY;

  auto tag = is.Peek()->tag();
  if (tag == Token::IDENTIFIER || Token::IsKeyWord(tag)) {
    auto str = is.Peek()->str();
    auto res = directive_map.find(str);
    if (res == directive_map.end())
      return Token::PP_NONE;
    return res->second;
  }
  return Token::PP_NONE;
}


void Preprocessor::ParseDirective(TokenSequence& os,
                                  TokenSequence& is,
                                  int directive) {
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
  case Token::PP_NONE:
    break;
  default:
    assert(false);
  }
}


void Preprocessor::ParsePragma(TokenSequence ls) {
  // TODO(wgtdkp):  
  ls.Next();
}


void Preprocessor::ParseError(TokenSequence ls) {
  ls.Next();
  const auto& literal = Stringize(ls);
  std::string msg;
  Scanner(literal).ScanLiteral(msg);
  Error(ls.Peek(), "%s", msg.c_str());
}


void Preprocessor::ParseLine(TokenSequence ls) {
  auto directive = ls.Next(); // Skip directive 'line'
  TokenSequence ts;
  Expand(ts, ls);
  auto tok = ts.Expect(Token::I_CONSTANT);

  int line = 0;
  size_t end = 0;
  try {
    line = stoi(tok->str(), &end, 10);
  } catch (const std::out_of_range oor) {
    Error(tok, "line number out of range");
  }
  if (line == 0 || end != tok->str().size()) {
    Error(tok, "illegal line number");
  }
  
  cur_line_ = line;
  line_line_ = directive->loc().line;
  if (ts.Empty())
    return;
  tok = ts.Expect(Token::LITERAL);
  
  // Enusure "s-char-sequence"
  if (tok->str().front() != '"' || tok->str().back() != '"') {
    Error(tok, "expect s-char-sequence");
  }
}


void Preprocessor::ParseIf(TokenSequence ls) {
  if (!NeedExpand()) {
    pp_cond_stack_.push({Token::PP_IF, false, false});
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
  pp_cond_stack_.push({Token::PP_IF, NeedExpand(), cond});
}


void Preprocessor::ParseIfdef(TokenSequence ls) {
  if (!NeedExpand()) {
    pp_cond_stack_.push({Token::PP_IFDEF, false, false});
    return;
  }

  ls.Next();
  auto ident = ls.Expect(Token::IDENTIFIER);
  if (!ls.Empty()) {
    Error(ls.Peek(), "expect new line");
  }

  int cond = FindMacro(ident->str()) != nullptr;

  pp_cond_stack_.push({Token::PP_IFDEF, NeedExpand(), cond});
}


void Preprocessor::ParseIfndef(TokenSequence ls) {
  ParseIfdef(ls);
  auto top = pp_cond_stack_.top();
  pp_cond_stack_.pop();
  top.tag_ = Token::PP_IFNDEF;
  top.cond_ = !top.cond_;
  
  pp_cond_stack_.push(top);
}


void Preprocessor::ParseElif(TokenSequence ls) {
  auto directive = ls.Next(); // Skip the directive

  if (pp_cond_stack_.empty())
    Error(directive, "unexpected 'elif' directive");
  auto top = pp_cond_stack_.top();
  if (top.tag_ == Token::PP_ELSE)
    Error(directive, "unexpected 'elif' directive");

  while (!pp_cond_stack_.empty()) {
    top = pp_cond_stack_.top();
    if (top.tag_ == Token::PP_IF ||
        top.tag_ == Token::PP_IFDEF ||
        top.tag_ == Token::PP_IFNDEF ||
        top.cond_) {
      break;
    }
    pp_cond_stack_.pop();
  }
  if (pp_cond_stack_.empty())
    Error(directive, "unexpected 'elif' directive");
  auto enabled = top.enabled_;
  if (!enabled) {
    pp_cond_stack_.push({Token::PP_ELIF, false, false});
    return;
  }

  if (ls.Empty()) {
    Error(ls.Peek(), "expect expression in 'elif' directive");
  }

  TokenSequence ts;
  ReplaceDefOp(ls);
  Expand(ts, ls, true);
  ReplaceIdent(ts);

  Parser parser(ts);
  auto expr = parser.ParseExpr();
  int cond = Evaluator<long>().Eval(expr);

  cond = cond && !top.cond_;
  pp_cond_stack_.push({Token::PP_ELIF, true, cond});
}


void Preprocessor::ParseElse(TokenSequence ls) {
  auto directive = ls.Next();
  if (!ls.Empty())
    Error(ls.Peek(), "expect new line");

  if (pp_cond_stack_.empty())
    Error(directive, "unexpected 'else' directive");
  auto top = pp_cond_stack_.top();  
  if (top.tag_ == Token::PP_ELSE)
    Error(directive, "unexpected 'else' directive");
  
  while (!pp_cond_stack_.empty()) {
    top = pp_cond_stack_.top();
    if (top.tag_ == Token::PP_IF ||
        top.tag_ == Token::PP_IFDEF ||
        top.tag_ == Token::PP_IFNDEF ||
        top.cond_) {
      break;
    }
    pp_cond_stack_.pop();
  }
  if (pp_cond_stack_.empty())
    Error(directive, "unexpected 'else' directive");

  auto cond = !top.cond_;
  auto enabled = top.enabled_;
  pp_cond_stack_.push({Token::PP_ELSE, enabled, cond});
}


void Preprocessor::ParseEndif(TokenSequence ls) {
  auto directive = ls.Next();
  if (!ls.Empty())
    Error(ls.Peek(), "expect new line");

  while ( !pp_cond_stack_.empty()) {
    auto top = pp_cond_stack_.top();
    pp_cond_stack_.pop();

    if (top.tag_ == Token::PP_IF
        || top.tag_ == Token::PP_IFDEF
        || top.tag_ == Token::PP_IFNDEF) {
      return;
    }
  }

  if (pp_cond_stack_.empty())
    Error(directive, "unexpected 'endif' directive");
}


// Have Read the '#'
void Preprocessor::ParseInclude(TokenSequence& is, TokenSequence ls) {
  bool next = ls.Next()->str() == "include_next"; // Skip 'include'
  if (!ls.Test(Token::LITERAL) && !ls.Test('<')) {
    TokenSequence ts;
    Expand(ts, ls, true);
    ls = ts;
  }
  
  auto tok = ls.Next();
  if (tok->tag() == Token::LITERAL) {
    if (!ls.Empty()) {
      Error(ls.Peek(), "expect new line");
    }
    std::string filename;
    Scanner(tok).ScanLiteral(filename);
    auto full_path = SearchFile(filename, false, next, *tok->loc().filename);
    if (full_path == nullptr)
      Error(tok, "%s: No such file or directory", filename.c_str());

    IncludeFile(is, full_path);
  } else if (tok->tag() == '<') {
    auto lhs = tok;
    auto rhs = tok;
    int cnt = 1;
    while (!(rhs = ls.Next())->IsEOF()) {
      if (rhs->tag() == '<')
        ++cnt;
      else if (rhs->tag() == '>')
        --cnt;
      if (cnt == 0)
        break;
    }
    if (cnt != 0)
      Error(rhs, "expect '>'");
    if (!ls.Empty())
      Error(ls.Peek(), "expect new line");

    const auto& filename = Scanner::ScanHeadName(lhs, rhs);
    auto full_path = SearchFile(filename, true, next, *tok->loc().filename);
    if (full_path == nullptr) {
      Error(tok, "%s: No such file or directory", filename.c_str());
    }
    IncludeFile(is, full_path);
  } else {
    Error(tok, "expect filename(string or in '<>')");
  }
}


void Preprocessor::ParseUndef(TokenSequence ls) {
  ls.Next(); // Skip directive

  auto ident = ls.Expect(Token::IDENTIFIER);
  if (!ls.Empty())
    Error(ls.Peek(), "expect new line");

  RemoveMacro(ident->str());
}


void Preprocessor::ParseDef(TokenSequence ls) {
  ls.Next();
  auto ident = ls.Expect(Token::IDENTIFIER);

  auto tok = ls.Peek();
  if (tok->tag() == '(' && !tok->ws()) {
    // There is no white space between ident and '('
    // Hence, we are defining function-like macro

    // Parse Identifier list
    ls.Next(); // Skip '('
    ParamList param_list;
    auto variadic = ParseIdentList(param_list, ls);
    AddMacro(ident->str(), Macro(variadic, param_list, ls));
  } else {
    AddMacro(ident->str(), Macro(ls));
  }
}


bool Preprocessor::ParseIdentList(ParamList& param_list, TokenSequence& is) {
  const Token* tok = is.Peek();
  while (!is.Empty()) {
    tok = is.Next();
    if (tok->tag() == ')') {
      return false;
    } else if (tok->tag() == Token::ELLIPSIS) {
      is.Expect(')');
      return true;
    } else if (tok->tag() != Token::IDENTIFIER) {
      Error(tok, "expect identifier");
    }

    for (const auto& param: param_list) {
      if (param == tok->str())
        Error(tok, "duplicated param");
    }
    param_list.push_back(tok->str());

    if (!is.Try(',')) {
      is.Expect(')');
      return false;
    }
  }

  Error(tok, "unexpected end of line");
  return false; // Make compiler happy
}


void Preprocessor::IncludeFile(TokenSequence& is,
                               const std::string* filename) {
  TokenSequence ts {is.tok_list_, is.begin_, is.begin_};
  Scanner scanner(ReadFile(*filename), filename);
  scanner.Tokenize(ts);
  
  // We done including header file
  is.begin_ = ts.begin_;
}


static std::string GetDir(const std::string& path) {
  auto pos = path.rfind('/');
  if (pos == std::string::npos)
    return "./";
  return path.substr(0, pos + 1);
}


std::string* Preprocessor::SearchFile(const std::string& name,
                                      const bool lib_header,
                                      bool next,
                                      const std::string& cur_path) {
  if (lib_header && !next) {
    path_list_.push_back(GetDir(cur_path));
  } else {
    path_list_.push_front(GetDir(cur_path));
  }

  PathList::iterator begin, end;
  auto iter = path_list_.begin();
  for (; iter != path_list_.end(); ++iter) {
    auto dd = open(iter->c_str(), O_RDONLY);
    if (dd == -1) // TODO(wgtdkp): or ensure it before preprocessing
      continue;
    auto fd = openat(dd, name.c_str(), O_RDONLY);
    close(dd);
    if (fd != -1) {
      // Intentional, so that recursive include
      // will result in running out of file descriptor
      //close(fd);
      auto path = *iter + name;
      if (next) {
        if (path != cur_path)
          continue;
        else
          next = false;
      } else {
        if (path == cur_path)
          continue;
        if (lib_header && !next)
          path_list_.pop_back();
        else
          path_list_.pop_front();
        return new std::string(path);
      }
    } else if (errno == EMFILE) {
      Error("may recursive include");
    }
  }
  return nullptr;
}


void Preprocessor::AddMacro(const std::string& name,
                            std::string* text,
                            bool pre_def) {
  TokenSequence ts;
  Scanner scanner(text);
  scanner.Tokenize(ts);
  Macro macro(ts, pre_def);

  AddMacro(name, macro);
}


static std::string* Date() {
  time_t t = time(NULL);
  struct tm* tm = localtime(&t);
  auto buf = new char[14];
  strftime(buf, 14, "\"%a %M %Y\"", tm);
  auto ret = new std::string(buf);
  delete[] buf;
  return ret;
}


void Preprocessor::Init() {
  // Preinclude search paths
  AddSearchPath("/usr/local/include/");
  AddSearchPath("/usr/include/x86_64-linux-gnu/");
  AddSearchPath("/usr/include/linux/");
  AddSearchPath("/usr/include/");
  AddSearchPath("/usr/local/wgtcc/include/");
  
  // The __FILE__ and __LINE__ macro is empty
  // They are handled seperately
  AddMacro("__FILE__", Macro(TokenSequence(), true));
  AddMacro("__LINE__", Macro(TokenSequence(), true));

  AddMacro("__DATE__", Date(), true);
  AddMacro("__STDC__", new std::string("1"), true);
  AddMacro("__STDC__HOSTED__", new std::string("0"), true);
  AddMacro("__STDC_VERSION__", new std::string("201103L"), true);
}


void Preprocessor::HandleTheFileMacro(TokenSequence& os, const Token* macro) {
  auto file = Token::New(*macro);
  file->set_tag(Token::LITERAL);
  file->set_str("\"" + *macro->loc().filename + "\"");
  os.InsertBack(file);
}


void Preprocessor::HandleTheLineMacro(TokenSequence& os, const Token* macro) {
  auto line = Token::New(*macro);
  line->set_tag(Token::I_CONSTANT);
  line->set_str(std::to_string(macro->loc().line));
  os.InsertBack(line);
}


void Preprocessor::UpdateFirstTokenLine(TokenSequence ts) {
  auto loc = ts.Peek()->loc();
  loc.line = cur_line_  + loc.line - line_line_ - 1;
  ts.UpdateHeadLocation(loc);
}


TokenSequence Macro::pep_seq(const std::string* filename, unsigned line) {
  // Update line
  TokenList tl;
  TokenSequence ret(&tl);
  ret.Copy(rep_seq_);
  auto ts = ret;
  while (!ts.Empty()) {
    auto loc = ts.Peek()->loc();
    loc.filename = filename;
    loc.line = line;
    ts.UpdateHeadLocation(loc);
    ts.Next();
  }
  return ret;
}


void Preprocessor::AddSearchPath(std::string path) {
  if (path.back() != '/')
    path += "/";
  if (path[0] != '/')
    path = "./" + path;
  path_list_.push_front(path);
}
