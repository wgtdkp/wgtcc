#include "cpp.h"

#include "parser.h"

#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>


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
void Preprocessor::Expand(TokenSeq& os, TokenSeq& is)
{
    Macro* macro = nullptr;
    int direcitve;
    while (!is.Empty()) {
        auto name = is.Peek()->Str();
        if ((direcitve = GetDirective(is)) != Token::INVALID) {
            ParseDirective(os, is, direcitve);
        } else if (!NeedExpand()) {
            is.Next();
        } else if (Hidden(name)) {
            os.InsertBack(is.Peek());
            is.Next();
        } else if ((macro = FindMacro(name))) {
            auto tok = is.Next();
            _hs.insert(name);
            if (macro->ObjLike()) {

                // Make a copy, as subst will change repSeq
                TokenSeq repSeq = macro->RepSeq();
                
                // The first token of the replcement sequence should 
                // derived the _ws property of the macro to be replaced.
                // And the subsequent tokens' _ws property should keep.
                // It could be dangerous to modify any property of 
                // the token after being generated.
                // But, as _ws is just for '#' opreator, and the replacement 
                // sequence of a macro is only used to replace the macro, 
                // it should be safe here we change the _ws property.
                repSeq.Peek()->_ws = tok->_ws;
 
                TokenList tokList;
                TokenSeq repSeqSubsted(&tokList);
                ParamMap paramMap;
                // TODO(wgtdkp): hideset is not right
                Subst(repSeqSubsted, repSeq, _hs, paramMap);

                is.InsertFront(repSeqSubsted);
            } else if (is.Test('(')) {
                is.Next();
                auto tok = is.Peek();
                ParamMap paramMap;
                ParseActualParam(is, macro, paramMap);

                // Make a copy, as subst will change repSeq
                TokenSeq repSeq = macro->RepSeq();
                repSeq.Peek()->_ws = tok->_ws;

                TokenList tokList;
                TokenSeq repSeqSubsted(&tokList);
                
                // TODO(wgtdkp): hideset is not right
                Subst(repSeqSubsted, repSeq, _hs, paramMap);
                is.InsertFront(repSeqSubsted);
            } else {
                Error(is.Peek(), "expect '(' for func-like macro '%s'",
                        name.c_str());
            }
            _hs.erase(name);
        } else {
            os.InsertBack(is.Peek());
            is.Next();
        }
    }
}

static TokenSeq* FindActualParam(ParamMap& params, const std::string& fp)
{
    auto res = params.find(fp);
    if (res == params.end()) {
        return nullptr;
    }
    return &res->second;
}

void Preprocessor::Subst(TokenSeq& os, TokenSeq& is,
        HideSet& hs, ParamMap& params)
{
    TokenSeq* ap;
    if (is.Empty()) {
        return;
    } else if (is.Test('#')
            && (ap = FindActualParam(params, is.Peek2()->Str()))) {
        is.Next(); is.Next();

        auto tok = *ap->Peek();
        tok._tag = Token::STRING_LITERAL;
        Stringize(tok._begin, tok._end, *ap);

        os.InsertBack(&tok);
        Subst(os, is, hs, params);
    } else if (is.Test(Token::DSHARP)
            && (ap = FindActualParam(params, is.Peek2()->Str()))) {
        is.Next();
        is.Next();
        
        if (ap->Empty()) {
            Subst(os, is, hs, params);
        } else {
            Glue(os, *ap);
            Subst(os, is, hs, params);
        }
    } else if (is.Test(Token::DSHARP)) {
        is.Next();
        auto tok = is.Next();

        Glue(os, tok);
        Subst(os, is, hs, params);
    } else if (is.Peek2()->_tag == Token::DSHARP 
            && (ap = FindActualParam(params, is.Peek()->Str()))) {
        is.Next();

        if (ap->Empty()) {
            is.Next();
            if ((ap = FindActualParam(params, is.Peek()->Str()))) {
                is.Next();
                os.InsertBack(*ap);
                Subst(os, is, hs, params);
            } else {
                Subst(os, is, hs, params);
            }
        } else {
            os.InsertBack(*ap);
            Subst(os, is, hs, params);
        }
    } else if ((ap = FindActualParam(params, is.Peek()->Str()))) {
        is.Next();
        Expand(os, *ap);
        Subst(os, is, hs, params);
    } else {
        os.InsertBack(is.Peek());
        is.Next();
        Subst(os, is, hs, params);
    }
}

void Preprocessor::Glue(TokenSeq& os, Token* tok)
{
    TokenList tokList;
    tokList.push_back(*tok);
    TokenSeq is(&tokList);

    Glue(os, is);
}

void Preprocessor::Glue(TokenSeq& os, TokenSeq& is)
{
    auto lhs = os.Back();
    auto rhs = is.Peek();

    auto str = new std::string(lhs->Str() + rhs->Str());

    TokenSeq ts;
    Lexer lexer(str, lhs->_fileName, lhs->_line);
    lexer.Tokenize(ts);
    
    //--os._end;
    is.Next();

    if (ts.Empty()) {
        // TODO(wgtdkp): 
        // No new Token generated
        // How to handle it???

    } else {
        // FIXME(wgtdkp): memory leakage
        //Token* tok = new Token(*lhs);
        Token* newTok = ts.Next();

        lhs->_tag = newTok->_tag;
        lhs->_begin = newTok->_begin;
        lhs->_end = newTok->_end;
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
void Preprocessor::Stringize(char*& begin, char*& end, TokenSeq is)
{
    auto str = new std::string();    
    str->push_back('"');

    while (!is.Empty()) {
        auto tok = is.Next();
        // Have preceding white space 
        // and is not the first token of the sequence
        str->append(tok->_ws && str->size() > 1, ' ');
        if (tok->_tag == Token::STRING_LITERAL) {
            for (auto p = tok->_begin; p != tok->_end; p++) {
                if (*p == '"' || *p == '\\')
                    str->push_back('\\');
                str->push_back(*p);
            }
        } else {
            str->append(tok->_begin, tok->Size());
        }
    }
    str->push_back('"');

    begin = const_cast<char*>(str->c_str());
    end = begin + str->size();
}

/*
 * For debug: print a token sequence
 */
void Preprocessor::Stringize(std::string& str, TokenSeq is)
{
    unsigned line = 1;
    unsigned maxLine = 1;

    while (!is.Empty()) {
        auto tok = is.Peek();
        
        if (is.IsBeginOfLine()) {
            if (tok->_line > maxLine) {
                str.push_back('\n');
                maxLine = tok->_line;
            }
        }

        line = tok->_line;
        while (!is.Empty() && is.Peek()->_line == line) {
            tok = is.Next();
            str.append(tok->_ws, ' ');
            str.append(tok->_begin, tok->Size());
        }
    }
}

void HSAdd(TokenSeq& ts, HideSet& hs)
{
    // TODO(wgtdkp): expand hideset
}

// TODO(wgtdkp): add predefined macros
void Preprocessor::Process(TokenSeq& os)
{
    TokenSeq is;
    IncludeFile(is, _curFileName);

    std::string str;
    Stringize(str, is);
    std::cout << str << std::endl;

    Expand(os, is);

    // TODO(wgtdkp): Identify key word
    auto ts = os;
    while (!ts.Empty()) {
        auto tok = ts.Next();
        auto tag = Token::KeyWordTag(tok->_begin, tok->_end);
        if (Token::IsKeyWord(tag)) {
            tok->_tag = tag;
        }
    }

    str.resize(0);
    Stringize(str, os);
    std::cout << std::endl << "###### Preprocessed ######" << std::endl;
    std::cout << str << std::endl << std::endl;
    std::cout << std::endl << "###### End ######" << std::endl;
}


void Preprocessor::ParseActualParam(TokenSeq& is,
        Macro* macro, ParamMap& paramMap)
{
    //TokenSeq ts(is);
    TokenSeq ap(is);
    auto fp = macro->Params().begin();

    ap._begin = is._begin;

    int cnt = 1;
    
    while (cnt > 0) {
        if (is.Test('('))
            ++cnt;
        else if (is.Test(')'))
            --cnt;
        
        if ((is.Test(',') && cnt == 1) || cnt == 0) {
            ap._end = is._begin;

            if (fp == macro->Params().end()) {
                Error(is.Peek(), "too many params");
            }
            
            paramMap.insert(std::make_pair(*fp, ap));
            ++fp;

            ap._begin = ++ap._end;  
        }

        is.Next();
    }

    if (fp != macro->Params().end()) {
        // TODO(wgtdkp): error
        Error(is.Peek(), "too few params");
    }
}

void Preprocessor::ReplaceDefOp(TokenSeq& is)
{
    //assert(is._begin == is._tokList->begin()
    //        && is._end == is._tokList->end());

#define ERASE(iter) {                                   \
    auto tmp = iter;                                    \
    iter = is._tokList->erase(iter);                    \
    if (tmp == is._begin) {                             \
        is._begin = iter;                               \
    }                                                   \
    if (iter == is._end) {                              \
        Error(&(*tmp), "unexpected end of line");       \
    }                                                   \
}

    for (auto iter = is._begin; iter != is._end; iter++) {
        if (iter->_tag== Token::IDENTIFIER && iter->Str() == "defined") {
            ERASE(iter);
            bool hasPar = false;
            if (iter->_tag == '(') {
                hasPar = true;
                ERASE(iter);
            }

            if (iter->_tag != Token::IDENTIFIER) {
                Error(&(*iter), "expect identifer in 'defined' operator");
            }
            
            auto name = iter->Str();

            if (hasPar) {
                ERASE(iter);
                if (iter->_tag != ')') {
                    Error(&(*iter), "expect ')'");
                }
            }

            iter->_tag = Token::I_CONSTANT;
            iter->_begin = const_cast<char*>(FindMacro(name) ? "1": "0");
            iter->_end = iter->_begin + 1;
        }
    }
#undef ERASE
}

void Preprocessor::ReplaceIdent(TokenSeq& is)
{
    for (auto iter = is._begin; iter != is._end; iter++) {
        if (iter->_tag == Token::IDENTIFIER) {
            iter->_tag = Token::I_CONSTANT;
            *iter->_begin = '0';
            iter->_end = iter->_begin + 1;
        }
    }
}


TokenSeq Preprocessor::GetLine(TokenSeq& is)
{
    auto begin = is._begin;
    while (!is.Empty() && is._begin->_line == begin->_line)
        is.Next();
    auto end = is._begin;
    return  TokenSeq(is._tokList, begin, end);
}

int Preprocessor::GetDirective(TokenSeq& is)
{
    if (!is.Test('#') || !is.IsBeginOfLine())
        return Token::INVALID;

    is.Next();

    if (is.IsBeginOfLine())
        return Token::PP_EMPTY;

    auto tag = is.Peek()->_tag;
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

void Preprocessor::ParseDirective(TokenSeq& os, TokenSeq& is, int directive)
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

        if (NeedExpand())
            os.InsertBack(ls);

        //if (NeedExpand())
        //    break;
        //while (!is.Empty()) {
        //    if ((direcitve = GetDirective(is)) != Token::INVALID)
        //        break;
        //    is.Next();
        //}
    //}
}

void Preprocessor::ParsePragma(TokenSeq ls)
{
    ls.Next();

    // Leave it
    // TODO(wgtdkp):
}

void Preprocessor::ParseError(TokenSeq ls)
{
    ls.Next();
    
    std::string msg;
    Stringize(msg, ls);
    Error(ls.Peek(), "%s", msg.c_str());
}

void Preprocessor::ParseLine(TokenSeq ls)
{
    auto directive = ls.Next(); // Skip directive 'line'

    TokenSeq ts;
    Expand(ts, ls);

    auto tok = ts.Expect(Token::I_CONSTANT);

    long long line = atoi(tok->_begin);
    if (line <= 0 || line > 0x7fffffff) {
        Error(tok, "illegal line number");
    }
    
    _curLine = line;
    _lineLine = directive->_line;

    if (ts.Empty())
        return;

    tok = ts.Expect(Token::STRING_LITERAL);
    
    // Enusure "s-char-sequence"
    if (*tok->_begin != '"' || *(tok->_end - 1) != '"') {
        Error(tok, "expect s-char-sequence");
    }
    
    //_curFileName = std::string(tok->_begin, tok->_end);
}

void Preprocessor::ParseIf(TokenSeq ls)
{
    if (!NeedExpand()) {
        _ppCondStack.push({Token::PP_IF, false, false});
        return;
    }

    auto tok = ls.Next(); // Skip the directive

    if (ls.Empty()) {
        Error(tok, "expect expression in 'if' directive");
    }

    TokenSeq ts;
    ReplaceDefOp(ls);
    TokenSeq tmp = ls;
    Expand(ts, ls);
    ReplaceIdent(ts);

    if (ts.Empty()) {
        Expand(ts, tmp);
        std::cout << "go hell" << std::endl;
    }
    
    //assert(!ts.Empty());
    auto begin = ts.Peek();
    Parser parser(ts);
    auto expr = parser.ParseExpr();
    auto cond = expr->EvalInteger(begin);
    // TODO(wgtdkp): delete expr

    _ppCondStack.push({Token::PP_IF, NeedExpand(), cond});
}

void Preprocessor::ParseIfdef(TokenSeq ls)
{
    if (!NeedExpand()) {
        _ppCondStack.push({Token::PP_IFDEF, false, false});
        return;
    }

    ls.Next();
    
    auto ident = ls.Expect(Token::IDENTIFIER);
    if (!ls.Peek()->IsEOF()) {
        Error(ls.Peek(), "expect new line");
    }

    int cond = FindMacro(ident->Str()) != nullptr;

    _ppCondStack.push({Token::PP_IFDEF, NeedExpand(), cond});
}

void Preprocessor::ParseIfndef(TokenSeq ls)
{
    ParseIfdef(ls);
    auto top = _ppCondStack.top();
    _ppCondStack.pop();
    top._tag = Token::PP_IFNDEF;
    top._cond = !top._cond;
    
    _ppCondStack.push(top);
}

void Preprocessor::ParseElif(TokenSeq ls)
{
    if (!NeedExpand()) {
        _ppCondStack.push({Token::PP_ELIF, false, false});
        return;
    }

    auto directive = ls.Next(); // Skip the directive

    if (ls.Empty()) {
        Error(ls.Peek(), "expect expression in 'elif' directive");
    }

    TokenSeq ts;
    ReplaceDefOp(ls);
    Expand(ts, ls);
    ReplaceIdent(ts);

    if (ts.Empty()) {
        std::cout << "go hell" << std::endl;
    }

    auto begin = ts.Peek();
    Parser parser(ts);
    auto expr = parser.ParseExpr();
    auto cond = expr->EvalInteger(begin);

    if (_ppCondStack.empty()) {
        Error(directive, "unexpected 'elif' directive");
    }
    auto top = _ppCondStack.top();
    if (top._tag == Token::PP_ELSE) {
        Error(directive, "unexpected 'elif' directive");
    }

    cond = cond && !top._cond;
    auto enabled = top._enabled;
    _ppCondStack.push({Token::PP_ELIF, enabled, cond});
}

void Preprocessor::ParseElse(TokenSeq ls)
{
    auto directive = ls.Next();
    if (!ls.IsBeginOfLine()) {
        Error(ls.Peek(), "expect new line");
    }

    if (_ppCondStack.empty()) {
        Error(directive, "unexpected 'else' directive");
    }
    auto top = _ppCondStack.top();
    if (top._tag == Token::PP_ELSE) {
        Error(directive, "unexpected 'elif' directive");
    }

    auto cond = !top._cond;
    auto enabled = top._enabled;
    _ppCondStack.push({Token::PP_ELSE, enabled, cond});
}

void Preprocessor::ParseEndif(TokenSeq ls)
{  
    auto directive = ls.Next();

    if (!ls.IsBeginOfLine()) {
        Error(ls.Peek(), "expect new line");
    }

    while ( !_ppCondStack.empty()) {
        auto top = _ppCondStack.top();
        _ppCondStack.pop();

        if (top._tag == Token::PP_IF
                || top._tag == Token::PP_IFDEF
                || top._tag == Token::PP_IFNDEF) {
            return;
        }
    }

    if (_ppCondStack.empty()) {
        Error(directive, "unexpected 'endif' directive");
    }
}

// Have Read the '#'
void Preprocessor::ParseInclude(TokenSeq& is, TokenSeq ls)
{
    ls.Next(); // Skip 'include'
    auto tok = ls.Next();
    if (tok->_tag == Token::STRING_LITERAL) {
        if (!ls.Peek()->IsEOF()) {
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
    } else if (tok->_tag == '<') {
        auto lhs = tok;
        auto rhs = tok;
        int cnt = 1;
        while (!(rhs = ls.Next())->IsEOF()) {
            if (rhs->_tag == '<')
                ++cnt;
            else if (rhs->_tag == '>')
                --cnt;
            if (cnt == 0)
                break;
        }
        if (cnt != 0) {
            Error(rhs, "expect '>'");
        }
        if (!ls.Peek()->IsEOF()) {
            Error(ls.Peek(), "expect new line");
        }

        auto fileName = std::string(lhs->_end, rhs->_begin);
        auto fullPath = SearchFile(fileName, true);
        if (fullPath == nullptr) {
            Error(tok, "%s: No such file or directory", fileName.c_str());
        }
        IncludeFile(is, fullPath);
    } else {
        Error(tok, "expect filename(string or in '<>')");
    }
}

void Preprocessor::ParseUndef(TokenSeq ls)
{
    ls.Next(); // Skip directive

    auto ident = ls.Expect(Token::IDENTIFIER);
    if (!ls.Empty()) {
        Error(ls.Peek(), "expect new line");
    }

    RemoveMacro(ident->Str());
}

void Preprocessor::ParseDef(TokenSeq ls)
{
    ls.Next();
    auto ident = ls.Expect(Token::IDENTIFIER);

    auto tok = ls.Peek();
    if (tok->_tag == '(' && tok->_begin == ident->_end) {
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

bool Preprocessor::ParseIdentList(ParamList& params, TokenSeq& is)
{
    Token* tok;
    while (!is.Empty()) {
        tok = is.Next();
        if (tok->_tag == Token::ELLIPSIS) {
            is.Expect(')');
            return true;
        } else if (tok->_tag != Token::IDENTIFIER) {
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

void Preprocessor::IncludeFile(TokenSeq& is, const std::string* fileName)
{
    TokenSeq ts(is._tokList, is._begin, is._begin);
    Lexer lexer(ReadFile(*fileName), fileName);
    lexer.Tokenize(ts);
    
    // We done including header file
    is._begin = ts._begin;

    _curFileName = fileName;
    auto tmp = std::string("\"") + *fileName + "\"";
    AddMacro(*fileName, new std::string(tmp), true);
}

std::string* Preprocessor::SearchFile(const std::string& name, bool libHeader)
{
    PathList::iterator begin, end;
    if (libHeader) {
        auto iter = _searchPathList.begin();
        for (; iter != _searchPathList.end(); iter++) {
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
        auto fd = open(name.c_str(), O_RDONLY);
        if (fd != -1) {
            close(fd);
            return new std::string(name);
        }

        auto iter = _searchPathList.rbegin();
        for (; iter != _searchPathList.rend(); iter++) {
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
    TokenSeq ts;
    Lexer lexer(text);
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
    AddSearchPath("/usr/include");
    AddSearchPath("/usr/include/linux");
    AddSearchPath("/usr/include/x86_64-linux-gnu");
    AddSearchPath("/usr/local/include");
    AddSearchPath("/home/wgtdkp/wgtcc/include");


    AddMacro("__FILE__", new std::string("\"" + *_curFileName + "\""), true);
    AddMacro("__LINE__", new std::string(std::to_string(_curLine)), true);
    AddMacro("__DATE__", Date(), true);
    AddMacro("__STDC__", new std::string("1"), true);
    AddMacro("__STDC__HOSTED__", new std::string("0"), true);
    AddMacro("__STDC_VERSION__", new std::string("201103L"), true);

    AddMacro("__x86_64__", new std::string("1"), true);
    AddMacro("__LP64__", new std::string("1"), true);
    
}
