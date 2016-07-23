#include "cpp.h"

#include "parser.h"

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
            // TODO(wgtdkp):
            ParseDirective(is, direcitve);
        } else if (!_ppCondStack.empty() && !_ppCondStack.top().second) {
            // We are in conditional inclusion and the cond expr evals FALSE
            // We simply drop the input token sequence
            is.Next();
        } else if (Hidden(name)) {
            os.InsertBack(is.Peek());
            is.Next();
        } else if ((macro = FindMacro(name))) {
            is.Next();
            if (macro->ObjLike()) {
                TokenSeq& repSeq = macro->RepSeq();

                TokenList tokList;
                TokenSeq repSeqSubsted(&tokList);
                
                _hs.insert(name);
                ParamMap paramMap;
                Subst(repSeqSubsted, repSeq, _hs, paramMap);

                is.InsertFront(repSeqSubsted);
            } else if (is.Test('(')) {
                // TODO(wgtdkp): Check params and ')'
                // Then, substitute and expand
                is.Next();

                ParamMap paramMap;
                TokenList tokList;
                TokenSeq repSeqSubsted(&tokList);

                ParseActualParam(is, macro, paramMap);
                // TODO(wgtdkp): hideset is not right
                _hs.insert(name);
                Subst(repSeqSubsted, macro->RepSeq(), _hs, paramMap);
                is.InsertFront(repSeqSubsted);
            } else {
                // TODO(wgtdkp): error
            }
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
        // TODO(wgtdkp):
        //os.InsertBack(Stringize(*ap));
        Subst(os, is, hs, params);
    } else if (is.Test(Token::DSHARP)
            && (ap = FindActualParam(params, is.Peek2()->Str()))) {
        is.Next();
        is.Next();
        if (ap->Empty()) {
            Subst(os, is, hs, params);
        } else {
            // TODO(wgtdkp): glue()
            Glue(os, *ap);
            Subst(os, is, hs, params);
        }
    } else if (is.Test(Token::DSHARP)) {
        is.Next();
        auto tok = is.Peek();
        is.Next();

        Glue(os, tok);
        //os.push_back(is.Peek());
        is.Next();
        Subst(os, is, hs, params);
    } else if (is.Peek2()->_tag == Token::DSHARP 
            && (ap = FindActualParam(params, is.Peek()->Str()))) {
        is.Next();
        if (ap->Empty()) {
            is.Next();

            if ((ap = FindActualParam(params, is.Peek()->Str()))) {
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
        Expand(os, is);
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

    std::string str(lhs->Str() + rhs->Str());


    Lexer lexer;
    TokenSeq ts;
    lexer.ReadStr(str);
    lexer.Tokenize(ts);
    
    --os._end;
    is.Next();

    if (ts.Empty()) {
        // TODO(wgtdkp): 
        // No new Token generated
        // How to handle it???

    } else {
        // FIXME(wgtdkp): memory leakage
        Token* tok = new Token(*lhs);
        Token* newTok = ts.Next();

        tok->_tag = newTok->_tag;
        auto len = newTok->Size();
        tok->_begin = new char[len];
        memcpy(tok->_begin, newTok->_begin, len);
        tok->_end = tok->_begin + len;

        os.InsertBack(tok); // Copy once again in InsertBack() 
    }

    if (!ts.Empty()) {
        Error(lhs, "macro expansion failed: can't concatenate");
    }

    os.InsertBack(is);
}


void Preprocessor::Stringize(std::string& str, TokenSeq& is)
{
    auto ts = is; // Make a copy
    int line = 1;
    int maxLine = 1;
    int column = 1; 
    while (!ts.Empty()) {
        auto tok = ts.Peek();
        
        if (ts.IsBeginOfLine()) {
            if (tok->_line > maxLine) {
                str.push_back('\n');
                column = 1;
                maxLine = tok->_line;
            }
        }

        line = tok->_line;
        column = tok->_column;
        while (!ts.Empty() && ts.Peek()->_line == line) {
            tok = ts.Next();
            str.append(tok->_column - column, ' ');
            column = tok->_column;
            str.append(tok->_begin, tok->_end - tok->_begin);
            column += tok->_end - tok->_begin;
        }
    }
}

void HSAdd(TokenSeq& ts, HideSet& hs)
{
    // TODO(wgtdkp): expand hideset
}




// TODO(wgtdkp): add predefined macros
void Preprocessor::Process(TokenSeq& os, TokenSeq& is)
{
    std::string str;

    Stringize(str, is);
    std::cout << str << std::endl;

    Expand(os, is);

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
                // TODO(wgtdkp): error
            }
            paramMap.insert(std::make_pair(*fp, ap));

            ap._begin = ++ap._end;  
        }

        is.Next();
    }

    if (fp != macro->Params().end()) {
        // TODO(wgtdkp): error
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
#undef INC
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

void Preprocessor::ParseDirective(TokenSeq& is, int directive)
{
    switch(directive) {
    case Token::PP_IF:
        return ParseIf(is);
    case Token::PP_IFDEF:
        return ParseIfdef(is);
    case Token::PP_IFNDEF:
        return ParseIfndef(is);
    case Token::PP_ELIF:
        return ParseElif(is);
    case Token::PP_ELSE:
        return ParseElse(is);
    case Token::PP_ENDIF:
        return ParseEndif(is);
    case Token::PP_INCLUDE:
        return ParseInclude(is);
    case Token::PP_DEFINE:
        return ParseDef(is);
    case Token::PP_UNDEF:
        return ParseUndef(is);
    case Token::PP_LINE:
        return ParseLine(is);
    case Token::PP_ERROR:
        return ParseError(is);
    case Token::PP_PRAGMA:
        return ParsePragma(is);
    case Token::PP_EMPTY:
        //return ParseEmpty(is);
        break;
    default:
        assert(false);
    }
}

void Preprocessor::ParsePragma(TokenSeq& is)
{
    auto ls = GetLine(is);
    ls.Next();

    // Leave it
    // TODO(wgtdkp):
}

void Preprocessor::ParseError(TokenSeq& is)
{
    auto ls = GetLine(is);
    ls.Next();
    
    std::string msg;
    Stringize(msg, ls);
    Error(ls.Peek(), "%s", msg.c_str());
}

void Preprocessor::ParseLine(TokenSeq& is)
{
    auto ls = GetLine(is);
    ls.Next(); // Skip directive 'line'

    TokenSeq ts;
    Expand(ts, ls);

    auto tok = ts.Expect(Token::I_CONSTANT);

    long long line = atoi(tok->_begin);
    if (line <= 0 || line > 0x7fffffff) {
        Error(tok, "illegal line number");
    }
    _curLine = line;

    if (ts.Empty())
        return;

    tok = ts.Expect(Token::STRING_LITERAL);
    
    // Enusure "s-char-sequence"
    if (*tok->_begin != '"' || *(tok->_end - 1) != '"') {
        Error(tok, "expect s-char-sequence");
    }
    
    _curFileName = StrPair(tok->_begin + 1, tok->_end - 1);
}

void Preprocessor::ParseIf(TokenSeq& is)
{   
    auto ls = GetLine(is);
    ls.Next(); // Skip the directive

    TokenSeq ts;
    ReplaceDefOp(ls);
    Expand(ts, ls);
    ReplaceIdent(ts);

    

    auto begin = ts.Peek();
    Parser parser(ts);
    auto expr = parser.ParseExpr();
    auto cond = expr->EvalInteger(begin);
    // TODO(wgtdkp): delete expr

    _ppCondStack.push(std::make_pair(Token::PP_IF, cond));
}

void Preprocessor::ParseIfdef(TokenSeq& is)
{
    auto ts = GetLine(is);
    ts.Next();
    
    auto ident = ts.Expect(Token::IDENTIFIER);
    if (!ts.Peek()->IsEOF()) {
        Error(ts.Peek(), "expect new line");
    }

    int cond = FindMacro(ident->Str()) != nullptr;

    _ppCondStack.push(std::make_pair(Token::PP_IFDEF, cond));
}

void Preprocessor::ParseIfndef(TokenSeq& is)
{
    ParseIfdef(is);
    auto top = _ppCondStack.top();
    _ppCondStack.pop();
    top.first = Token::PP_IFNDEF;
    top.second = !top.second;
    
    _ppCondStack.push(top);
}

void Preprocessor::ParseElif(TokenSeq& is)
{
    auto directive = is.Peek();

    auto ls = GetLine(is);
    ls.Next(); // Skip the directive 

    TokenSeq ts;
    ReplaceDefOp(ls);
    Expand(ts, ls);
    ReplaceIdent(ts);

    

    auto begin = ts.Peek();
    Parser parser(ts);
    auto expr = parser.ParseExpr();
    auto cond = expr->EvalInteger(begin);

    if (_ppCondStack.empty()) {
        Error(directive, "unexpected 'elif' directive");
    }
    auto top = _ppCondStack.top();
    cond = cond && !top.second;

    _ppCondStack.push(std::make_pair(Token::PP_ELIF, cond));
}

void Preprocessor::ParseElse(TokenSeq& is)
{
    auto directive = is.Next();
    if (!is.IsBeginOfLine()) {
        Error(is.Peek(), "expect new line");
    }

    if (_ppCondStack.empty()) {
        Error(directive, "unexpected 'else' directive");
    }
    auto top = _ppCondStack.top();
    top.first = Token::PP_ELSE;
    top.second = !top.second;

    _ppCondStack.push(top);
}

void Preprocessor::ParseEndif(TokenSeq& is)
{  
    auto directive = is.Next();

    if (!is.IsBeginOfLine()) {
        Error(is.Peek(), "expect new line");
    }

    while ( !_ppCondStack.empty()) {
        auto top = _ppCondStack.top();
        _ppCondStack.pop();

        if (top.first == Token::PP_IF
                || top.first == Token::PP_IFDEF
                || top.first == Token::PP_IFNDEF) {
            return;
        }
    }

    if (_ppCondStack.empty()) {
        Error(directive, "unexpected 'endif' directive");
    }
}

// Have Read the '#' and 'include' 
void Preprocessor::ParseInclude(TokenSeq& is)
{
    auto ls = GetLine(is);
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
        IncludeFile(is, *fullPath);
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
        IncludeFile(is, *fullPath);
    } else {
        Error(tok, "expect filename(string or in '<>')");
    }
}

void Preprocessor::ParseUndef(TokenSeq& is)
{
    auto ls = GetLine(is);
    ls.Next(); // Skip directive

    auto ident = ls.Expect(Token::IDENTIFIER);
    if (!ls.Empty()) {
        Error(ls.Peek(), "expect new line");
    }

    RemoveMacro(ident->Str());
}

void Preprocessor::ParseDef(TokenSeq& is)
{
    auto ls = GetLine(is);
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

        if (!is.Try(',')) {
            is.Expect(')');
            return false;
        }
    }

    Error(tok, "unexpected end of line");
    return false; // Make compiler happy
}

void Preprocessor::IncludeFile(TokenSeq& is, const std::string& fileName)
{
    Lexer lexer;
    lexer.ReadFile(fileName.c_str());
    TokenSeq ts(is._tokList, is._begin, is._begin);
    lexer.Tokenize(ts);

    PrintTokSeq(ts);
    // We done including header file
    is._begin = ts._begin;
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

void Preprocessor::Init(void)
{
    // Preinclude search paths
    AddSearchPath("/usr/include");
    AddSearchPath("/usr/include/linux");
    AddSearchPath("/usr/include/x86_64-linux-gnu");
    AddSearchPath("/usr/local/include");
    


    // TODO(wgtdkp): Predefined macros
    /*
    AddMacro("__FILE__", );
    AddMacro("__LINE__", );
    AddMacro("__DATE__", );
    AddMacro("__STDC__", );
    AddMacro("__STDC__HOSTED__", );
    AddMacro("__STDC_VERSION__", );
    */

}
