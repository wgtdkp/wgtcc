#include "cpp.h"


/*
 * @params:
 *     is: input token sequence
 *     os: output token sequence
 */
void Preprocessor::Expand(TokenSeq& os, TokenSeq& is)
{
    Macro* macro = nullptr;
    if (is.Empty()) {
        return;
    } else if (Hidden(is.Front().Str())) {
        os.InsertBack(is.Front());
        is.Forward();
        Expand(os, is);
    } else if ((macro = FindMacro(is.Front().Str()))) {
        is.Forward();
        if (macro->ObjLike()) {
            TokenSeq& repSeq = macro->RepSeq();

            TokenList tokList;
            TokenSeq repSeqSubsted(&tokList);
            
            _hs.insert(macro->Name());
            ParamMap paramMap;
            Subst(repSeqSubsted, repSeq, _hs, paramMap);

            is.InsertFront(repSeqSubsted);
            Expand(os, is);
        } else if (is.Front().Tag() == '(') {
            // TODO(wgtdkp): Check params and ')'
            // Then, substitute and expand
            is.Forward();

            ParamMap paramMap;

            TokenList tokList;
            TokenSeq repSeqSubsted(&tokList);

            ParseActualParam(is, macro, paramMap);
            // TODO(wgtdkp): hideset is not right
            _hs.insert(macro->Name());
            Subst(repSeqSubsted, macro->RepSeq(), _hs, paramMap);
            is.InsertFront(repSeqSubsted);
            Expand(os, is);
        } else {
            // TODO(wgtdkp): error
        }
    } else {
        os.InsertBack(is.Front());
        is.Forward();
        Expand(os, is);
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
    TokenList tokList;
    lexer.ReadStr(str);
    lexer.Tokenize(tokList);
    
    --os._end;
    is.Forward();

    if (tokList.size() > 2) {// Including the END token
        // TODO(wgtdkp): error

    } else if (tokList.size() == 2) {
        Token tok = lhs;
        tok._tag = tokList.front()._tag;
        auto len = tokList.front().Size();
        // FIXME(wgtdkp): memory leakage
        tok._begin = new char[len];
        memcpy(tok._begin, tokList.front()._begin, len);
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
void Preprocessor::Process(TokenList& ol, TokenList& il)
{
    _tokList = &il;
    auto is = TokenSeq(&il);
    auto os = TokenSeq(&ol);
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
