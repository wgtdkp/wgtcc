#include "cpp.h"

#include "parser.h"

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
 * If this seq starts from the begin of a line
 */ 
bool TokenSeq::IsBeginOfLine(void) const
{
    if (_begin == _tokList->begin())
        return true;

    auto pre = _begin;
    --pre;
    return _begin->_line > pre->_line;
}

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
        auto name = is.Front().Str();
        if ((direcitve = GetDirective(is)) != Token::INVALID) {
            // TODO(wgtdkp):
            ParseDirective(is, direcitve);
        } else if (!_ppCondStack.empty() && !_ppCondStack.top().second) {
            // We are in conditional inclusion and the cond expr evals FALSE
            // We simply drop the input token sequence
            is.Forward();
        } else if (Hidden(name)) {
            os.InsertBack(is.Front());
            is.Forward();
        } else if ((macro = FindMacro(name))) {
            is.Forward();
            if (macro->ObjLike()) {
                TokenSeq& repSeq = macro->RepSeq();

                TokenList tokList;
                TokenSeq repSeqSubsted(&tokList);
                
                _hs.insert(name);
                ParamMap paramMap;
                Subst(repSeqSubsted, repSeq, _hs, paramMap);

                is.InsertFront(repSeqSubsted);
            } else if (is.Front().Tag() == '(') {
                // TODO(wgtdkp): Check params and ')'
                // Then, substitute and expand
                is.Forward();

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
            os.InsertBack(is.Front());
            is.Forward();
        }
    }
}

static TokenSeq* FindActualParam(ParamMap& params, std::string fp)
{
    auto res = params.find(fp);
    if (res == params.end())
        return nullptr;
    return &res->second;
}

void Preprocessor::Subst(TokenSeq& os, TokenSeq& is,
        HideSet& hs, ParamMap& params)
{
    TokenSeq* ap;
    if (is.Empty()) {
        return;
    } else if (is.Front().Tag() == '#'
            && (ap = FindActualParam(params, is.Second().Str()))) {
        is.Forward();
        is.Forward();
        os.InsertBack(Stringize(ap));
        Subst(os, is, hs, params);
    } else if (is.Front().Tag() == Token::DSHARP
            && (ap = FindActualParam(params, is.Second().Str()))) {
        is.Forward();
        is.Forward();
        if (ap->Empty()) {
            Subst(os, is, hs, params);
        } else {
            // TODO(wgtdkp): glue()
            Glue(os, *ap);
            Subst(os, is, hs, params);
        }
    } else if (is.Front().Tag() == Token::DSHARP) {
        is.Forward();
        auto tok = is.Front();
        is.Forward();

        Glue(os, tok);
        //os.push_back(is.Front());
        is.Forward();
        Subst(os, is, hs, params);
    } else if (is.Second().Tag() == Token::DSHARP 
            && (ap = FindActualParam(params, is.Front().Str()))) {
        is.Forward();
        if (ap->Empty()) {
            is.Forward();

            if ((ap = FindActualParam(params, is.Front().Str()))) {
                os.InsertBack(*ap);
                Subst(os, is, hs, params);
            } else {
                Subst(os, is, hs, params);
            }
        } else {
            os.InsertBack(*ap);
            Subst(os, is, hs, params);
        }
    } else if ((ap = FindActualParam(params, is.Front().Str()))) {
        is.Forward();
        Expand(os, is);
        Subst(os, is, hs, params);
    } else {
        os.InsertBack(is.Front());
        is.Forward();
        Subst(os, is, hs, params);
    }
}

void Preprocessor::Glue(TokenSeq& os, Token& tok)
{
    TokenList tokList;
    tokList.push_back(tok);
    TokenSeq is(&tokList);

    Glue(os, is);
}

void Preprocessor::Glue(TokenSeq& os, TokenSeq& is)
{
    auto lhs = os.Back();
    auto rhs = is.Front();

    std::string str(lhs.Str() + rhs.Str());


    Lexer lexer;
    TokenSeq ts;
    lexer.ReadStr(str);
    lexer.Tokenize(ts);
    
    --os._end;
    is.Forward();

    auto size = ts.Size();
    if (size > 1) {
        // TODO(wgtdkp): error

    } else if (size == 1) {
        Token tok = lhs;
        tok._tag = ts.Front()._tag;
        auto len = ts.Front().Size();
        // FIXME(wgtdkp): memory leakage
        tok._begin = new char[len];
        memcpy(tok._begin, ts.Front()._begin, len);
        tok._end = tok._begin + len;

        os.InsertBack(tok);
    } else {
        // There is no token (except for white spaces and comments)
        // Do nothing
    }

    os.InsertBack(is);
}

Token Preprocessor::Stringize(TokenSeq* ts)
{
    Token tok;
    tok._tag = Token::STRING_LITERAL;
    // TODO(wgtdkp):

    // FIXME(wgtdkp): memory leakage
    return tok;
}

void HSAdd(TokenSeq& ts, HideSet& hs)
{
    // TODO(wgtdkp): expand hideset
}




// TODO(wgtdkp): add predefined macros
void Preprocessor::Process(TokenSeq& os, TokenSeq& is)
{

    Expand(os, is);
}


void Preprocessor::ParseActualParam(TokenSeq& is, Macro* macro, ParamMap& paramMap)
{
    //TokenSeq ts(is);
    TokenSeq ap(is);
    auto fp = macro->Params().begin();

    ap._begin = is._begin;

    int cnt = 1;
    
    while (cnt > 0) {
        int tag = is.Front().Tag();
        
        if (tag == '(')
            ++cnt;
        else if (tag == ')')
            --cnt;
        
        if ((tag == ',' && cnt == 1) || cnt == 0) {
            ap._end = is._begin;

            if (fp == macro->Params().end()) {
                // TODO(wgtdkp): error
            }
            paramMap.insert(std::make_pair(*fp, ap));

            ap._begin = ++ap._end;  
        }

        is.Forward();
    }

    if (fp != macro->Params().end()) {
        // TODO(wgtdkp): error
    }
}

TokenSeq Preprocessor::GetLine(TokenSeq& is)
{
    auto begin = is._begin;
    while (!is.Empty() && is._begin->_line == begin->_line)
        is.Forward();
    auto end = is._begin;
    return  TokenSeq(is._tokList, begin, end);
}

int Preprocessor::GetDirective(TokenSeq& is)
{
    if (is.Front().Tag() != '#' || !is.IsBeginOfLine())
        return Token::INVALID;

    is.Forward();

    auto tag = is.Front().Tag();
    if (tag == Token::IDENTIFIER || Token::IsKeyWord(tag)) {
        if (is.IsBeginOfLine())
            return Token::PP_EMPTY;
        
        auto str = is.Front().Str();
        auto res = directiveMap.find(str);
        if (res == directiveMap.end()) {
            // TODO(wgtdkp): error
        }
        is.Forward();
        return res->second;
    } else {
        // TODO(wgtdkp): error
        
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
        break;
    case Token::PP_UNDEF:
        break;
    case Token::PP_LINE:
        break;
    case Token::PP_ERROR:
        break;
    case Token::PP_PRAGMA:
        break;
    case Token::PP_EMPTY:
        break;
    default:
        assert(false);
    }
}

void Preprocessor::ParseIf(TokenSeq& is)
{   
    auto begin = is.Front();
    
    Parser parser(GetLine(is), this);
    auto expr = parser.ParseExpr();
    auto cond = expr->EvalInteger(&begin);
    // TODO(wgtdkp): delete expr
    _ppCondStack.push(std::make_pair(Token::PP_IF, cond));
}

void Preprocessor::ParseIfdef(TokenSeq& is)
{
    Parser parser(GetLine(is), this);
    auto ident = parser.Expect(Token::IDENTIFIER);
    if (!parser.Peek()->IsEOF()) {
        Error(parser.Peek(), "expect new line");
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
    auto begin = is.Front();
    Parser parser(GetLine(is), this);
    auto expr = parser.ParseExpr();
    auto cond = expr->EvalInteger(&begin);

    if (_ppCondStack.empty()) {
        Error(parser.Peek(), "unexpected 'elif' directive");
    }
    auto top = _ppCondStack.top();
    cond = cond && !top.second;

    _ppCondStack.push(std::make_pair(Token::PP_ELIF, cond));
}

void Preprocessor::ParseElse(TokenSeq& is)
{
    Parser parser(GetLine(is), this);
    if (!parser.Peek()->IsEOF()) {
        Error(parser.Peek(), "expect new line");
    }

    if (_ppCondStack.empty()) {
        Error(parser.Peek(), "unexpected 'else' directive");
    }
    auto top = _ppCondStack.top();
    top.first = Token::PP_ELSE;
    top.second = !top.second;

    _ppCondStack.push(top);
}

void Preprocessor::ParseEndif(TokenSeq& is)
{
    Parser parser(GetLine(is));
    if (!parser.Peek()->IsEOF()) {
        Error(parser.Peek(), "expect new line");
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
        Error(parser.Peek(), "unexpected 'endif' directive");
    }
}

// Have Read the '#' and 'include' 
void Preprocessor::ParseInclude(TokenSeq& is)
{
    Parser parser(GetLine(is), this);
    auto tok = parser.Next();
    if (tok->Tag() == Token::STRING_LITERAL) {
        if (!parser.Peek()->IsEOF()) {
            Error(parser.Peek(), "expect new line");
        }
        // TODO(wgtdkp): include file
        auto fileName = SearchFile(tok->Str(), false);
        if (fileName.size() == 0) {
            Error(tok, "%s: No such file or directory", tok->Str().c_str());
        }
        IncludeFile(is, fileName);
    } else if (tok->Tag() == '<') {
        
    } else {
        Error(tok, "expect filename(string or in '<>')");
    }
}

void Preprocessor::IncludeFile(TokenSeq& is, const std::string& fileName)
{

}

std::string Preprocessor::SearchFile(const std::string& name, bool libHeader)
{
    std::string ret;

    return ret;
}

void Preprocessor::Init(void)
{
    // Preinclude search paths
    AddSearchPath("/usr/include");
    AddSearchPath("/usr/local/include");

    // Predefined macros
    /*
    AddMacro("__FILE__", );
    AddMacro("__LINE__", );
    AddMacro("__DATE__", );
    AddMacro("__STDC__", );
    AddMacro("__STDC__HOSTED__", );
    AddMacro("__STDC_VERSION__", );
    */

}
