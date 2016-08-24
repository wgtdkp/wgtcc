#include "parser.h"

#include "cpp.h"
#include "error.h"
#include "scope.h"
#include "type.h"
#include "evaluator.h"

#include <iostream>
#include <set>
#include <string>

using namespace std;


FuncDef* Parser::EnterFunc(Identifier* ident) {
    //TODO(wgtdkp): Add __func__ macro

    _curParamScope->SetParent(_curScope);
    _curScope = _curParamScope;

    _curFunc = FuncDef::New(ident, LabelStmt::New());
    return _curFunc;
}


void Parser::ExitFunc(void) {
    // Resolve 那些待定的jump；
	// 如果有jump无法resolve，也就是有未定义的label，报错；
    for (auto iter = _unresolvedJumps.begin();
            iter != _unresolvedJumps.end(); iter++) {
        auto label = iter->first;
        auto labelStmt = FindLabel(label->Str());
        if (labelStmt == nullptr) {
            Error(label, "label '%s' used but not defined",
                    label->Str().c_str());
        }
        
        iter->second->SetLabel(labelStmt);
    }
    
    _unresolvedJumps.clear();	//清空未定的 jump 动作
    _curLabels.clear();	//清空 label map

    _curScope = _curScope->Parent();
    _curFunc = nullptr;
}


void Parser::EnterBlock(FuncType* funcType)
{
    if (funcType) {
        auto paramScope = _curScope;
        _curScope = new Scope(_curScope->Parent(), S_BLOCK);
        
        // Merge elements in param scope into current block scope
        auto iter = paramScope->begin();
        for (; iter != paramScope->end(); iter++) {
            _curScope->Insert(iter->second);
        }
    } else {
        _curScope = new Scope(_curScope, S_BLOCK);
    }
}


void Parser::Parse(void)
{
    ParseTranslationUnit();
}


void Parser::ParseTranslationUnit(void)
{
    while (!_ts.Peek()->IsEOF()) {            
        _curParamScope = nullptr;

        int storageSpec, funcSpec;
        auto type = ParseDeclSpec(&storageSpec, &funcSpec);
        auto tokTypePair = ParseDeclarator(type);
        auto tok = tokTypePair.first;
        type = tokTypePair.second;

        if (tok == nullptr) {
            _ts.Expect(';');
            continue;
        }

        auto ident = ProcessDeclarator(tok, type, storageSpec, funcSpec);
        type = ident->Type();

        if (tok && type->ToFuncType() && _ts.Try('{')) { // Function definition
            _unit->Add(ParseFuncDef(ident));
        } else { // Declaration
            auto decl = ParseInitDeclarator(ident);
            if (decl) _unit->Add(decl);

            while (_ts.Try(',')) {
                auto ident = ParseDirectDeclarator(type, storageSpec, funcSpec);
                decl = ParseInitDeclarator(ident);
                if (decl) _unit->Add(decl);
            }
            _ts.Expect(';');
        }
    }
    
    //_externalSymbols->Print();
}


FuncDef* Parser::ParseFuncDef(Identifier* ident)
{
    auto funcDef = EnterFunc(ident);

    if (ident->Type()->Complete()) {
        Error(ident, "redefinition of '%s'", ident->Name().c_str());
    }

    auto funcType = ident->Type()->ToFuncType();
    FuncType::TypeList& paramTypes = funcType->ParamTypes();
    if (_curScope->size() != paramTypes.size()) {
        Error(ident, "parameter name omitted");
    }

    for (auto paramType: paramTypes) {
        auto iter = _curScope->begin();
        for (; iter != _curScope->end(); iter++) {
            if (iter->second->Type() == paramType) {
                funcDef->Params().push_back(iter->second->ToObject());
                break;
            }
        }
    }

    funcType->SetComplete(true);
    funcDef->SetBody(ParseCompoundStmt(funcType));
    ExitFunc();
    
    return funcDef;
}


Expr* Parser::ParseExpr(void)
{
    return ParseCommaExpr();
}


Expr* Parser::ParseCommaExpr(void)
{
    auto lhs = ParseAssignExpr();
    auto tok = _ts.Peek();
    while (_ts.Try(',')) {
        auto rhs = ParseAssignExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Peek();
    }
    return lhs;
}


Expr* Parser::ParsePrimaryExpr(void)
{
    if (_ts.Empty()) {
        Error(_ts.Peek(), "premature end of input");
    } else if (_ts.Peek()->IsKeyWord()) {
        Error(_ts.Peek(), "unexpected keyword");
    }

    auto tok = _ts.Next();
    if (tok->Tag() == '(') {
        auto expr = ParseExpr();
        _ts.Expect(')');
        return expr;
    }

    if (tok->IsIdentifier()) {
        auto ident = _curScope->Find(tok);
        /* if (ident == nullptr || ident->ToObject() == nullptr) { */
        if (ident == nullptr) {
            Error(tok, "undefined symbol '%s'", tok->Str().c_str());
        }
        return ident;
    } else if (tok->IsConstant()) {
        return ParseConstant(tok);
    } else if (tok->IsLiteral()) {
        return ParseLiteral(tok);
    } else if (tok->Tag() == Token::GENERIC) {
        return ParseGeneric();
    }

    Error(tok, "'%s' unexpected", tok->Str().c_str());
    return nullptr; // Make compiler happy
}


// TODO(wgtdkp):
Constant* Parser::ParseConstant(const Token* tok)
{
    assert(tok->IsConstant());

    if (tok->Tag() == Token::I_CONSTANT) {
        long ival = stol(tok->Str());
        return Constant::New(tok, T_LONG, ival);
    } else {
        double fval = stod(tok->Str());
        return Constant::New(tok, T_DOUBLE, fval);
    }
}


// TODO(wgtdkp):
Constant* Parser::ParseLiteral(const Token* tok)
{
    const char* p = tok->_begin;
    int tag;
    switch (p[0]) {
    case 'u':
        if (p[1] == '8') {
            tag = T_CHAR; ++p;
        } else {
            tag = T_UNSIGNED | T_SHORT;
        } ++p; break;
    case 'U': tag = T_UNSIGNED | T_INT; ++p; break;
    case 'L': tag = T_INT; ++p; break;
    case '"': tag = T_CHAR; break;
    default: assert(false);
    }
    ++p;

    // TODO(wgtdkp): May handle in lexer
    //while (p[0]) {
    //
    //}
    auto val = new std::string(p, tok->_end - 1 - p);
    return Constant::New(tok, tag, val);
}


// TODO(wgtdkp):
Expr* Parser::ParseGeneric(void)
{
    assert(0);
    return nullptr;
}


Expr* Parser::ParsePostfixExpr(void)
{
    auto tok = _ts.Next();
    if (tok->IsEOF()) {
        //return nullptr;
        Error(tok, "premature end of input");
    }

    if ('(' == tok->Tag() && IsTypeName(_ts.Peek())) {
        // TODO(wgtdkp):
        //compound literals
        Error(tok, "compound literals not supported yet");
        //return ParseCompLiteral();
    }

    _ts.PutBack();
    auto primExpr = ParsePrimaryExpr();
    
    return ParsePostfixExprTail(primExpr);
}


//return the constructed postfix expression
Expr* Parser::ParsePostfixExprTail(Expr* lhs)
{
    while (true) {
        auto tok = _ts.Next();
        
        switch (tok->Tag()) {
        case '[':
            lhs = ParseSubScripting(lhs);
            break;

        case '(':
            lhs = ParseFuncCall(lhs);
            break;
        
        case Token::PTR_OP:
            lhs = UnaryOp::New(Token::DEREF, lhs);    
        case '.':
            lhs = ParseMemberRef(tok, '.', lhs);
            break;

        case Token::INC_OP:
        case Token::DEC_OP:
            lhs = ParsePostfixIncDec(tok, lhs);
            break;

        default:
            _ts.PutBack();
            return lhs;
        }
    }
}


Expr* Parser::ParseSubScripting(Expr* lhs)
{
    auto rhs = ParseExpr();
    
    auto tok = _ts.Peek();
    _ts.Expect(']');
    //
    auto operand = BinaryOp::New(tok, '+', lhs, rhs);
    return UnaryOp::New(Token::DEREF, operand);
}


BinaryOp* Parser::ParseMemberRef(const Token* tok, int op, Expr* lhs)
{
    auto memberName = _ts.Peek()->Str();
    _ts.Expect(Token::IDENTIFIER);

    auto structUnionType = lhs->Type()->ToStructUnionType();
    if (structUnionType == nullptr) {
        Error(tok, "an struct/union expected");
    }

    auto rhs = structUnionType->GetMember(memberName);
    if (rhs == nullptr) {
        Error(tok, "'%s' is not a member of '%s'",
                memberName.c_str(), "[obj]");
    }

    return  BinaryOp::New(tok, op, lhs, rhs);
}


UnaryOp* Parser::ParsePostfixIncDec(const Token* tok, Expr* operand)
{
    auto op = tok->Tag() == Token::INC_OP ?
            Token::POSTFIX_INC: Token::POSTFIX_DEC;

    return UnaryOp::New(op, operand);
}


FuncCall* Parser::ParseFuncCall(Expr* designator)
{
    FuncCall::ArgList args;
    while (!_ts.Try(')')) {
        args.push_back(Expr::MayCast(ParseAssignExpr()));
        if (!_ts.Test(')'))
            _ts.Expect(',');
    }

    return FuncCall::New(designator, args);
}


Expr* Parser::ParseUnaryExpr(void)
{
    auto tok = _ts.Next();
    switch (tok->Tag()) {
    case Token::ALIGNOF:
        return ParseAlignof();
    case Token::SIZEOF:
        return ParseSizeof();
    case Token::INC_OP:
        return ParsePrefixIncDec(tok);
    case Token::DEC_OP:
        return ParsePrefixIncDec(tok);
    case '&':
        return ParseUnaryOp(tok, Token::ADDR);
    case '*':
        return ParseUnaryOp(tok, Token::DEREF); 
    case '+':
        return ParseUnaryOp(tok, Token::PLUS);
    case '-':
        return ParseUnaryOp(tok, Token::MINUS); 
    case '~':
        return ParseUnaryOp(tok, '~');
    case '!':
        return ParseUnaryOp(tok, '!');
    default:
        _ts.PutBack();
        return ParsePostfixExpr();
    }
}


Constant* Parser::ParseSizeof(void)
{
    Type* type;
    auto tok = _ts.Next();
    Expr* unaryExpr = nullptr;

    if (tok->Tag() == '(' && IsTypeName(_ts.Peek())) {
        type = ParseTypeName();
        _ts.Expect(')');
    } else {
        _ts.PutBack();
        unaryExpr = ParseUnaryExpr();
        type = unaryExpr->Type();
    }

    if (type->ToFuncType() || type->ToVoidType()) {
    } else if (!type->Complete()) {
        Error(tok, "sizeof(incomplete type)");
    }

    return Constant::New(tok, T_UNSIGNED | T_LONG, (long)type->Width());
}


Constant* Parser::ParseAlignof(void)
{
    auto tok = _ts.Expect('(');
    auto type = ParseTypeName();
    _ts.Expect(')');

    return Constant::New(tok, T_UNSIGNED | T_LONG, (long)type->Align());
}


UnaryOp* Parser::ParsePrefixIncDec(const Token* tok)
{
    assert(tok->Tag() == Token::INC_OP || tok->Tag() == Token::DEC_OP);
    
    auto op = tok->Tag() == Token::INC_OP ?
            Token::PREFIX_INC: Token::PREFIX_DEC;
    auto operand = ParseUnaryExpr();
    
    return UnaryOp::New(op, operand);
}


UnaryOp* Parser::ParseUnaryOp(const Token* tok, int op)
{
    auto operand = ParseCastExpr();

    return UnaryOp::New(op, operand);
}


Type* Parser::ParseTypeName(void)
{
    auto type = ParseSpecQual();
    if (_ts.Test('*') || _ts.Test('(')) //abstract-declarator FIRST set
        return ParseAbstractDeclarator(type);
    
    return type;
}


Expr* Parser::ParseCastExpr(void)
{
    auto tok = _ts.Next();
    if (tok->Tag() == '(' && IsTypeName(_ts.Peek())) {
        auto desType = ParseTypeName();
        _ts.Expect(')');
        auto operand = ParseCastExpr();
        
        return UnaryOp::New(Token::CAST, operand, desType);
    }
    
    _ts.PutBack();
    return ParseUnaryExpr();
}


Expr* Parser::ParseMultiplicativeExpr(void)
{
    auto lhs = ParseCastExpr();
    auto tok = _ts.Next();
    while (tok->Tag() == '*' || tok->Tag() == '/' || tok->Tag() == '%') {
        auto rhs = ParseCastExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Next();
    }
    
    _ts.PutBack();
    return lhs;
}


Expr* Parser::ParseAdditiveExpr(void)
{
    auto lhs = ParseMultiplicativeExpr();
    auto tok = _ts.Next();
    while (tok->Tag() == '+' || tok->Tag() == '-') {
        auto rhs = ParseMultiplicativeExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Next();
    }
    
    _ts.PutBack();
    return lhs;
}


Expr* Parser::ParseShiftExpr(void)
{
    auto lhs = ParseAdditiveExpr();
    auto tok = _ts.Next();
    while (tok->Tag() == Token::LEFT_OP || tok->Tag() == Token::RIGHT_OP) {
        auto rhs = ParseAdditiveExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Next();
    }
    
    _ts.PutBack();
    return lhs;
}


Expr* Parser::ParseRelationalExpr(void)
{
    auto lhs = ParseShiftExpr();
    auto tok = _ts.Next();
    while (tok->Tag() == Token::LE_OP || tok->Tag() == Token::GE_OP 
            || tok->Tag() == '<' || tok->Tag() == '>') {
        auto rhs = ParseShiftExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Next();
    }
    
    _ts.PutBack();
    return lhs;
}


Expr* Parser::ParseEqualityExpr(void)
{
    auto lhs = ParseRelationalExpr();
    auto tok = _ts.Next();
    while (tok->Tag() == Token::EQ_OP || tok->Tag() == Token::NE_OP) {
        auto rhs = ParseRelationalExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Next();
    }
    
    _ts.PutBack();
    return lhs;
}


Expr* Parser::ParseBitiwiseAndExpr(void)
{
    auto lhs = ParseEqualityExpr();
    auto tok = _ts.Peek();
    while (_ts.Try('&')) {
        auto rhs = ParseEqualityExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Peek();
    }
    
    return lhs;
}


Expr* Parser::ParseBitwiseXorExpr(void)
{
    auto lhs = ParseBitiwiseAndExpr();
    auto tok = _ts.Peek();
    while (_ts.Try('^')) {
        auto rhs = ParseBitiwiseAndExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Peek();
    }
    
    return lhs;
}


Expr* Parser::ParseBitwiseOrExpr(void)
{
    auto lhs = ParseBitwiseXorExpr();
    auto tok = _ts.Peek();
    while (_ts.Try('|')) {
        auto rhs = ParseBitwiseXorExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Peek();
    }
    
    return lhs;
}


Expr* Parser::ParseLogicalAndExpr(void)
{
    auto lhs = ParseBitwiseOrExpr();
    auto tok = _ts.Peek();
    while (_ts.Try(Token::AND_OP)) {
        auto rhs = ParseBitwiseOrExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Peek();
    }
    
    return lhs;
}


Expr* Parser::ParseLogicalOrExpr(void)
{
    auto lhs = ParseLogicalAndExpr();
    auto tok = _ts.Peek();
    while (_ts.Try(Token::OR_OP)) {
        auto rhs = ParseLogicalAndExpr();
        lhs = BinaryOp::New(tok, lhs, rhs);

        tok = _ts.Peek();
    }
    
    return lhs;
}


Expr* Parser::ParseConditionalExpr(void)
{
    auto cond = ParseLogicalOrExpr();
    auto tok = _ts.Peek();
    if (_ts.Try('?')) {
        auto exprTrue = ParseExpr();
        _ts.Expect(':');
        auto exprFalse = ParseConditionalExpr();

        return ConditionalOp::New(tok, cond, exprTrue, exprFalse);
    }
    
    return cond;
}


Expr* Parser::ParseAssignExpr(void)
{
    // Yes, I know the lhs should be unary expression, 
    // let it handled by type checking
    Expr* lhs = ParseConditionalExpr();
    Expr* rhs;

    auto tok = _ts.Next();
    switch (tok->Tag()) {
    case Token::MUL_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, '*', lhs, rhs);
        break;

    case Token::DIV_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, '/', lhs, rhs);
        break;

    case Token::MOD_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, '%', lhs, rhs);
        break;

    case Token::ADD_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, '+', lhs, rhs);
        break;

    case Token::SUB_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, '-', lhs, rhs);
        break;

    case Token::LEFT_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, Token::LEFT_OP, lhs, rhs);
        break;

    case Token::RIGHT_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, Token::RIGHT_OP, lhs, rhs);
        break;

    case Token::AND_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, '&', lhs, rhs);
        break;

    case Token::XOR_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, '^', lhs, rhs);
        break;

    case Token::OR_ASSIGN:
        rhs = ParseAssignExpr();
        rhs = BinaryOp::New(tok, '|', lhs, rhs);
        break;

    case '=':
        rhs = ParseAssignExpr();
        break;

    default:
        _ts.PutBack();
        return lhs; // Could be constant
    }

    return BinaryOp::New(tok, '=', lhs, rhs);
}


/**************** Declarations ********************/

// Return: list of declarations
CompoundStmt* Parser::ParseDecl(void)
{
    StmtList stmts;

    if (_ts.Try(Token::STATIC_ASSERT)) {
        //TODO: static_assert();
        assert(false);
    } else {
        int storageSpec, funcSpec;
        auto type = ParseDeclSpec(&storageSpec, &funcSpec);
        
        //init-declarator 的 FIRST 集合：'*', identifier, '('
        if (_ts.Test('*') || _ts.Test(Token::IDENTIFIER) || _ts.Test('(')) {
            do {
                auto ident = ParseDirectDeclarator(type, storageSpec, funcSpec);
                auto init = ParseInitDeclarator(ident);
                if (init) {
                    stmts.push_back(init);
                }
            } while (_ts.Try(','));
        }            
        _ts.Expect(';');
    }

    return CompoundStmt::New(stmts);
}


//for state machine
enum {
    //compatibility for these key words
    COMP_SIGNED = T_SHORT | T_INT | T_LONG | T_LONG_LONG,
    COMP_UNSIGNED = T_SHORT | T_INT | T_LONG | T_LONG_LONG,
    COMP_CHAR = T_SIGNED | T_UNSIGNED,
    COMP_SHORT = T_SIGNED | T_UNSIGNED | T_INT,
    COMP_INT = T_SIGNED | T_UNSIGNED | T_LONG | T_SHORT | T_LONG_LONG,
    COMP_LONG = T_SIGNED | T_UNSIGNED | T_LONG | T_INT,
    COMP_DOUBLE = T_LONG | T_COMPLEX,
    COMP_COMPLEX = T_FLOAT | T_DOUBLE | T_LONG,

    COMP_THREAD = S_EXTERN | S_STATIC,
};


static inline void TypeLL(int& typeSpec)
{
    if (typeSpec & T_LONG) {
        typeSpec &= ~T_LONG;
        typeSpec |= T_LONG_LONG;
    } else {
        typeSpec |= T_LONG;
    }
}


Type* Parser::ParseSpecQual(void)
{
    return ParseDeclSpec(nullptr, nullptr);
}


/*
param: storage: null, only type specifier and qualifier accepted;
*/
Type* Parser::ParseDeclSpec(int* storage, int* func)
{
    Type* type = nullptr;
    int align = -1;
    int storageSpec = 0;
    int funcSpec = 0;
    int qualSpec = 0;
    int typeSpec = 0;
    
    Token* tok;
    for (; ;) {
        tok = _ts.Next();
        switch (tok->Tag()) {
        //function specifier
        case Token::INLINE:
            funcSpec |= F_INLINE;
            break;

        case Token::NORETURN:
            funcSpec |= F_NORETURN;
            break;

        //alignment specifier
        case Token::ALIGNAS:
            align = ParseAlignas();
            break;

        //storage specifier
        //TODO: typedef needs more constraints
        case Token::TYPEDEF:
            if (storageSpec != 0)
                goto error;
            storageSpec |= S_TYPEDEF;
            break;

        case Token::EXTERN:
            if (storageSpec & ~S_THREAD)
                goto error;
            storageSpec |= S_EXTERN;
            break;

        case Token::STATIC:
            if (storageSpec & ~S_THREAD)
                goto error;
            storageSpec |= S_STATIC;
            break;

        case Token::THREAD:
            if (storageSpec & ~COMP_THREAD)
                goto error;
            storageSpec |= S_THREAD;
            break;

        case Token::AUTO:
            if (storageSpec != 0)
                goto error;
            storageSpec |= S_AUTO;
            break;

        case Token::REGISTER:
            if (storageSpec != 0)
                goto error; 
            storageSpec |= S_REGISTER;
            break;
        
        //type qualifier
        case Token::CONST:
            qualSpec |= Q_CONST;
            break;

        case Token::RESTRICT:
            qualSpec |= Q_RESTRICT;
            break;

        case Token::VOLATILE:
            qualSpec |= Q_VOLATILE;
            break;
        /*
        atomic_qual:
            qualSpec |= Q_ATOMIC;
            break;
        */

        //type specifier
        case Token::SIGNED:
            if (typeSpec & ~COMP_SIGNED)
                goto error; 
            typeSpec |= T_SIGNED;
            break;

        case Token::UNSIGNED:
            if (typeSpec & ~COMP_UNSIGNED)
                goto error;
            typeSpec |= T_UNSIGNED;
            break;

        case Token::VOID:
            if (0 != typeSpec)
                goto error;
            typeSpec |= T_VOID;
            break;

        case Token::CHAR:
            if (typeSpec & ~COMP_CHAR)
                goto error;
            typeSpec |= T_CHAR;
            break;

        case Token::SHORT:
            if (typeSpec & ~COMP_SHORT)
                goto error;
            typeSpec |= T_SHORT;
            break;

        case Token::INT:
            if (typeSpec & ~COMP_INT)
                goto error;
            typeSpec |= T_INT;
            break;

        case Token::LONG:
            if (typeSpec & ~COMP_LONG)
                goto error; 
            TypeLL(typeSpec); 
            break;
            
        case Token::FLOAT:
            if (typeSpec & ~T_COMPLEX)
                goto error;
            typeSpec |= T_FLOAT;
            break;

        case Token::DOUBLE:
            if (typeSpec & ~COMP_DOUBLE)
                goto error;
            typeSpec |= T_DOUBLE;
            break;

        case Token::BOOL:
            if (typeSpec != 0)
                goto error;
            typeSpec |= T_BOOL;
            break;

        case Token::COMPLEX:
            if (typeSpec & ~COMP_COMPLEX)
                goto error;
            typeSpec |= T_COMPLEX;
            break;

        case Token::STRUCT: 
        case Token::UNION:
            if (typeSpec)
                goto error; 
            type = ParseStructUnionSpec(Token::STRUCT == tok->Tag()); 
            typeSpec |= T_STRUCT_UNION;
            break;

        case Token::ENUM:
            if (typeSpec != 0)
                goto error;
            type = ParseEnumSpec();
            typeSpec |= T_ENUM;
            break;

        case Token::ATOMIC:\
            assert(false);
            /*
            if (_ts.Peek()->Tag() != '(')
                goto atomic_qual;
            if (typeSpec != 0)
                goto error;
            type = ParseAtomicSpec();
            typeSpec |= T_ATOMIC;
            break;
            */
        default:
            if (typeSpec == 0 && IsTypeName(tok)) {
                auto ident = _curScope->Find(tok);
                if (ident) {
                    ident = ident->ToTypeName();
                    type = ident ? ident->Type(): nullptr;
                }
                typeSpec |= T_TYPEDEF_NAME;
            } else  {
                goto end_of_loop;
            }
        }
    }

end_of_loop:
    _ts.PutBack();
    switch (typeSpec) {
    case 0:
        Error(tok, "expect type specifier");
        break;

    case T_VOID:
        type = VoidType::New();
        break;

    case T_ATOMIC:
    case T_STRUCT_UNION:
    case T_ENUM:
    case T_TYPEDEF_NAME:
        break;

    default:
        type = ArithmType::New(typeSpec);
        break;
    }

    if ((storage == nullptr || func == nullptr)) {
        if (funcSpec && storageSpec && align != -1) {
            Error(tok, "type specifier/qualifier only");
        }
    } else {
        *storage = storageSpec;
        *func = funcSpec;
    }

    type->SetQual(qualSpec);
    return type;

error:
    Error(tok, "type speficier/qualifier/storage error");
    return nullptr;	// Make compiler happy
}


int Parser::ParseAlignas(void)
{
    int align;
    _ts.Expect('(');
    if (IsTypeName(_ts.Peek())) {
        auto type = ParseTypeName();
        _ts.Expect(')');
        align = type->Align();
    } else {
        auto expr = ParseExpr();
        align = Evaluator<long>().Eval(expr);
        _ts.Expect(')');
        delete expr;
    }

    return align;
}


Type* Parser::ParseEnumSpec(void)
{
    std::string tagName;
    auto tok = _ts.Peek();
    if (_ts.Try(Token::IDENTIFIER)) {
        tagName = tok->Str();
        if (_ts.Try('{')) {
            //定义enum类型
            auto tagIdent = _curScope->FindTagInCurScope(tok);
            if (tagIdent == nullptr)
                goto enum_decl;

            if (!tagIdent->Type()->Complete()) {
                return ParseEnumerator(tagIdent->Type()->ToArithmType());
            } else {
                Error(tok, "redefinition of enumeration tag '%s'",
                        tagName.c_str());
            }
        } else {
            //Type* type = _curScope->FindTag(tagName);
            auto tagIdent = _curScope->FindTag(tok);
            if (tagIdent) {
                return tagIdent->Type();
            }
            auto type = ArithmType::New(T_INT);
            type->SetComplete(false);   //尽管我们把 enum 当成 int 看待，但是还是认为他是不完整的
            auto ident = Identifier::New(tok, type, _curScope, L_NONE);
            _curScope->InsertTag(ident);
            return type;
        }
    }
    
    _ts.Expect('{');

enum_decl:
    auto type = ArithmType::New(T_INT);
    type->SetComplete(false);
    if (tagName.size() != 0) {
        auto ident = Identifier::New(tok, type, _curScope, L_NONE);
        _curScope->InsertTag(ident);
    }
    
    return ParseEnumerator(type);   //处理反大括号: '}'
}


Type* Parser::ParseEnumerator(ArithmType* type)
{
    assert(type && !type->Complete() && type->IsInteger());
    
    std::set<int> valSet;
    int val = 0;
    do {
        auto tok = _ts.Expect(Token::IDENTIFIER);
        
        auto enumName = tok->Str();
        auto ident = _curScope->FindInCurScope(tok);
        if (ident) {
            Error(tok, "redefinition of enumerator '%s'", enumName.c_str());
        }
        if (_ts.Try('=')) {
            auto expr = ParseAssignExpr();
            val = Evaluator<long>().Eval(expr);

            if (valSet.find(val) != valSet.end()) {
                Error(expr, "conflict enumerator constant '%d'", val);
            }
        }
        valSet.insert(val);
        auto enumer = Enumerator::New(tok, _curScope, val);
        ++val;

        _curScope->Insert(enumer);

        _ts.Try(',');
    } while (!_ts.Try('}'));
    
    type->SetComplete(true);
    
    return type;
}


/*
 * 四种 name space：
 * 1.label, 如 goto end; 它有函数作用域
 * 2.struct/union/enum 的 tag
 * 3.struct/union 的成员
 * 4.其它的普通的变量
 */
Type* Parser::ParseStructUnionSpec(bool isStruct)
{
    std::string tagName;
    auto tok = _ts.Peek();
    if (_ts.Try(Token::IDENTIFIER)) {
        tagName = tok->Str();
        if (_ts.Try('{')) {
            //看见大括号，表明现在将定义该struct/union类型
            auto tagIdent = _curScope->FindTagInCurScope(tok);
            if (tagIdent == nullptr) //我们不用关心上层scope是否定义了此tag，如果定义了，那么就直接覆盖定义
                goto struct_decl; //现在是在当前scope第一次看到name，所以现在是第一次定义，连前向声明都没有；
            
            /*
             * 在当前scope找到了类型，但可能只是声明；注意声明与定义只能出现在同一个scope；
             * 1.如果声明在定义的外层scope,那么即使在内层scope定义了完整的类型，此声明仍然是无效的；
             *   因为如论如何，编译器都不会在内部scope里面去找定义，所以声明的类型仍然是不完整的；
             * 2.如果声明在定义的内层scope,(也就是先定义，再在内部scope声明)，这时，不完整的声明会覆盖掉完整的定义；
             *   因为编译器总是向上查找符号，不管找到的是完整的还是不完整的，都要；
             */
            if (!tagIdent->Type()->Complete()) {
                //找到了此tag的前向声明，并更新其符号表，最后设置为complete type
                return ParseStructUnionDecl(
                        tagIdent->Type()->ToStructUnionType());
            } else {
                //在当前作用域找到了完整的定义，并且现在正在定义同名的类型，所以报错；
                Error(tok, "redefinition of struct tag '%s'",
                        tagName.c_str());
            }
        } else {
            /*
			 * 没有大括号，表明不是定义一个struct/union;那么现在只可能是在：
			 * 1.声明；
			 * 2.声明的同时，定义指针(指针允许指向不完整类型) (struct Foo* p; 是合法的) 或者其他合法的类型；
			 *   如果现在索引符号表，那么：
			 *   1.可能找到name的完整定义，也可能只找得到不完整的声明；不管name指示的是不是完整类型，我们都只能选择name指示的类型；
			 *   2.如果我们在符号表里面压根找不到name,那么现在是name的第一次声明，创建不完整的类型并插入符号表；
			 */
            auto tagIdent = _curScope->FindTag(tok);
            
            //如果tag已经定义或声明，那么直接返回此定义或者声明
            if (tagIdent) {
                return tagIdent->Type();
            }
            
            //如果tag尚没有定义或者声明，那么创建此tag的声明(因为没有见到‘{’，所以不会是定义)
            auto type = StructUnionType::New(isStruct, true, _curScope);
            
            //因为有tag，所以不是匿名的struct/union， 向当前的scope插入此tag
            auto ident = Identifier::New(tok, type, _curScope, L_NONE);
            _curScope->InsertTag(ident);
            return type;
        }
    }
    //没见到identifier，那就必须有struct/union的定义，这叫做匿名struct/union;
    _ts.Expect('{');

struct_decl:
    //现在，如果是有tag，那它没有前向声明；如果是没有tag，那更加没有前向声明；
	//所以现在是第一次开始定义一个完整的struct/union类型
    auto type = StructUnionType::New(isStruct, tagName.size(), _curScope);
    if (tagName.size() != 0) {
        auto ident = Identifier::New(tok, type, _curScope, L_NONE);
        _curScope->InsertTag(ident);
    }
    
    return ParseStructUnionDecl(type); //处理反大括号: '}'
}


StructUnionType* Parser::ParseStructUnionDecl(StructUnionType* type)
{
    //既然是定义，那输入肯定是不完整类型，不然就是重定义了
    assert(type && !type->Complete());
    
    auto scopeBackup = _curScope;
    _curScope = type->MemberMap(); // Internal symbol lookup rely on _curScope

    bool packed = true;
    while (!_ts.Try('}')) {
        if (_ts.Empty()) {
            Error(_ts.Peek(), "premature end of input");
        }
        
        // 解析type specifier/qualifier, 不接受storage等

        auto memberType = ParseSpecQual();
        do {
            auto tokTypePair = ParseDeclarator(memberType);
            auto tok = tokTypePair.first;
            memberType = tokTypePair.second;
            std::string name;

            if (_ts.Try(':')) {
                packed = ParseBitField(type, tok, memberType, packed);
                goto end_decl;
            }

            if (tok == nullptr) {
                auto suType = memberType->ToStructUnionType();
                if (suType && !suType->HasTag()) {
                    type->MergeAnony(suType);
                    continue;
                } else {
                    Error(_ts.Peek(), "declaration does not declare anything");
                }
            }

            name = tok->Str();                
            if (type->GetMember(name)) {
                Error(tok, "duplicate member '%s'", name.c_str());
            }

            if (!memberType->Complete()) {
                Error(tok, "field '%s' has incomplete type", name.c_str());
            }

            if (memberType->ToFuncType()) {
                Error(tok, "field '%s' declared as a function", name.c_str());
            }

            type->AddMember(Object::New(tok, memberType, _curScope));

        end_decl:;

        } while (_ts.Try(','));
        _ts.Expect(';');
    }
    
    //struct/union定义结束，设置其为完整类型
    type->SetComplete(true);
    
    _curScope = scopeBackup;

    return type;
}


bool Parser::ParseBitField(StructUnionType* structType,
        Token* tok, Type* type, bool packed)
{
    if (!type->IsInteger()) {
        Error(tok ? tok: _ts.Peek(), "expect integer type for bitfield");
    }

    auto expr = ParseAssignExpr();
    auto width = Evaluator<long>().Eval(expr);
    if (width < 0) {
        Error(expr, "expect non negative value");
    } else if (width == 0 && tok) {
        Error(tok, "no declarator expected for a bitfield with width 0");
    } else if (width == 0) {
        return true;
    } else if (width > type->Width() * 8) {
        Error(expr, "width exceeds its type");
    }

    auto offset = structType->Offset() - type->Width();
    offset = Type::MakeAlign(std::max(offset, 0), type->Align());


    int bitFieldOffset;
    unsigned char begin;

    if (structType->Members().size() == 0) {
        begin = 0;
        bitFieldOffset = 0;
    } else {
        auto last = structType->Members().back();
        auto totalBits = (last->Offset() - offset) * 8;
        if (Object::IsBitField(last)) {
            totalBits += last->BitFieldEnd();
        } else {
            totalBits += last->Type()->Width() * 8;
        }

        auto bitsOffset = Type::MakeAlign(totalBits, 8);
        if (packed && (width + totalBits <= bitsOffset)) {
            begin = totalBits % 8;
            bitFieldOffset = totalBits / 8;
        } else if (Type::MakeAlign(width, 8) + bitsOffset <= type->Width() * 8) {
            begin = 0;
            bitFieldOffset = bitsOffset / 8;
        } else {
            begin = 0;
            bitFieldOffset = Type::MakeAlign(structType->Offset(), type->Width());
        }
    }

    auto bitField = Object::New(tok, type, _curScope, 0, L_NONE, begin, width);
    structType->AddBitField(bitField, bitFieldOffset);
    return false;
}


int Parser::ParseQual(void)
{
    int qualSpec = 0;
    for (; ;) {
        switch (_ts.Next()->Tag()) {
        case Token::CONST:
            qualSpec |= Q_CONST;
            break;

        case Token::RESTRICT:
            qualSpec |= Q_RESTRICT;
            break;

        case Token::VOLATILE:
            qualSpec |= Q_VOLATILE;
            break;

        case Token::ATOMIC:
            qualSpec |= Q_ATOMIC;
            break;

        default:
            _ts.PutBack();
            return qualSpec;
        }
    }
}


Type* Parser::ParsePointer(Type* typePointedTo)
{
    Type* retType = typePointedTo;
    while (_ts.Try('*')) {
        retType = PointerType::New(typePointedTo);
        retType->SetQual(ParseQual());
        typePointedTo = retType;
    }

    return retType;
}


static Type* ModifyBase(Type* type, Type* base, Type* newBase)
{
    if (type == base)
        return newBase;
    
    auto ty = type->ToDerivedType();
    ty->SetDerived(ModifyBase(ty->Derived(), base, newBase));
    
    return ty;
}


/*
 * Return: pair of token(must be identifier) and it's type
 *     if token is nullptr, then we are parsing abstract declarator
 *     else, parsing direct declarator.
 */
TokenTypePair Parser::ParseDeclarator(Type* base)
{
    // May be pointer
    auto pointerType = ParsePointer(base);
    
    if (_ts.Try('(')) {
        //现在的 pointerType 并不是正确的 base type
        auto tokenTypePair = ParseDeclarator(pointerType);
        auto tok = tokenTypePair.first;
        auto type = tokenTypePair.second;

        _ts.Expect(')');

        auto newBase = ParseArrayFuncDeclarator(tok, pointerType);
        
        //修正 base type
        auto retType = ModifyBase(type, pointerType, newBase);
        return TokenTypePair(tokenTypePair.first, retType);
    } else if (_ts.Peek()->IsIdentifier()) {
        auto tok = _ts.Next();
        auto retType = ParseArrayFuncDeclarator(tok, pointerType);
        return TokenTypePair(tok, retType);
    }

    _errTok = _ts.Peek();

    return TokenTypePair(nullptr, pointerType);
}


Identifier* Parser::ProcessDeclarator(Token* tok, Type* type,
        int storageSpec, int funcSpec)
{
    assert(tok);
    /*
     * 检查在同一 scope 是否已经定义此变量
	 * 如果 storage 是 typedef，那么应该往符号表里面插入 type
	 * 定义 void 类型变量是非法的，只能是指向void类型的指针
	 * 如果 funcSpec != 0, 那么现在必须是在定义函数，否则出错
     */
    auto name = tok->Str();
    Identifier* ident;

    if (storageSpec & S_TYPEDEF) {
        ident = _curScope->FindInCurScope(tok);
        if (ident) { // There is prio declaration in the same scope
            // The same declaration, simply return the prio declaration
            if (*type == *ident->Type())
                return ident;
            // TODO(wgtdkp): add previous declaration information
            Error(tok, "conflicting types for '%s'", name.c_str());
        }
        ident = Identifier::New(tok, type, _curScope, L_NONE);
        _curScope->Insert(ident);
        return ident;
    }

    if (type->ToVoidType()) {
        Error(tok, "variable or field '%s' declared void",
                name.c_str());
    }

    if (type->ToFuncType() && _curScope->Type() != S_FILE
            && (storageSpec & S_STATIC)) {
        Error(tok, "invalid storage class for function '%s'",
                name.c_str());
    }

    Linkage linkage;
    // Identifiers in function prototype have no linkage
    if (_curScope->Type() == S_PROTO) {
        linkage = L_NONE;
    } else if (_curScope->Type() == S_FILE) {
        linkage = L_EXTERNAL; // Default linkage for file scope identifiers
        if (storageSpec & S_STATIC)
            linkage = L_INTERNAL;
    } else if (!(storageSpec & S_EXTERN)) {
        linkage = L_NONE; // Default linkage for block scope identifiers
        if (type->ToFuncType())
            linkage = L_EXTERNAL;
    } else {
        linkage = L_EXTERNAL;
    }

    //_curScope->Print();
    ident = _curScope->FindInCurScope(tok);
    if (ident) { // There is prio declaration in the same scope
        if (*type != *ident->Type()) {
            Error(tok, "conflicting types for '%s'", name.c_str());
        }

        // The same scope prio declaration has no linkage,
        // there is a redeclaration error
        if (linkage == L_NONE) {
            Error(tok, "redeclaration of '%s' with no linkage",
                    name.c_str());
        } else if (linkage == L_EXTERNAL) {
            if (ident->Linkage() == L_NONE) {
                Error(tok, "conflicting linkage for '%s'", name.c_str());
            }
        } else {
            if (ident->Linkage() != L_INTERNAL) {
                Error(tok, "conflicting linkage for '%s'", name.c_str());
            }
        }
        // The same declaration, simply return the prio declaration
        if (!ident->Type()->Complete())
            ident->Type()->SetComplete(type->Complete());
        return ident;
    } else if (linkage == L_EXTERNAL) {
        ident = _curScope->Find(tok);
        if (ident) {
            if (*type != *ident->Type()) {
            	Error(tok, "conflicting types for '%s'",
                        name.c_str());
            }
            if (ident->Linkage() != L_NONE) {
                linkage = ident->Linkage();
            }
            // Don't return, override it
        } else {
            ident = _externalSymbols->FindInCurScope(tok);
            if (ident) {
                if (*type != *ident->Type()) {
                    Error(tok, "conflicting types for '%s'",
                            name.c_str());
                }
                // TODO(wgtdkp): ???????
                // Don't return
                // To stop later declaration with the same name in the same scope overriding this declaration
                
                // Useless here, just keep it
                if (!ident->Type()->Complete())
                    ident->Type()->SetComplete(type->Complete()); 
                //return ident;
            }
        }
    }

    Identifier* ret;
    // TODO(wgtdkp): Treat function as object ?
    if (type->ToFuncType()) {
        ret = Identifier::New(tok, type, _curScope, linkage);
    } else {
        ret = Object::New(tok, type, _curScope, storageSpec, linkage);
    }

    _curScope->Insert(ret);
    
    if (linkage == L_EXTERNAL && ident == nullptr) {
        _externalSymbols->Insert(ret);
    }

    return ret;
}


Type* Parser::ParseArrayFuncDeclarator(Token* ident, Type* base)
{
    if (_ts.Try('[')) {

        if (nullptr != base->ToFuncType()) {
            Error(_ts.Peek(), "the element of array can't be a function");
        }

        auto len = ParseArrayLength();
        if (0 == len) {
            Error(_ts.Peek(), "can't declare an array of length 0");
        }
        _ts.Expect(']');
        base = ParseArrayFuncDeclarator(ident, base);
        if (!base->Complete()) {
            Error(ident, "'%s' has incomplete element type",
                    ident->Str().c_str());
        }
        return ArrayType::New(len, base);
    } else if (_ts.Try('(')) {	//function declaration
        if (base->ToFuncType()) {
            Error(_ts.Peek(),
                    "the return value of function can't be function");
        } else if (nullptr != base->ToArrayType()) {
            Error(_ts.Peek(),
                    "the return value of function can't be array");
        }

        FuncType::TypeList paramTypes;
        EnterProto();
        bool hasEllipsis = ParseParamList(paramTypes);
        ExitProto();
        
        _ts.Expect(')');
        base = ParseArrayFuncDeclarator(ident, base);
        
        return FuncType::New(base, 0, hasEllipsis, paramTypes);
    }

    return base;
}


/*
 * return: -1, 没有指定长度；其它，长度；
 */
int Parser::ParseArrayLength(void)
{
    auto hasStatic = _ts.Try(Token::STATIC);
    auto qual = ParseQual();
    if (0 != qual)
        hasStatic = _ts.Try(Token::STATIC);
    /*
    if (!hasStatic) {
        if (_ts.Try('*'))
            return _ts.Expect(']'), -1;
        if (_ts.Try(']'))
            return -1;
        else {
            auto expr = ParseAssignExpr();
            auto len = Evaluate(expr);
            _ts.Expect(']');
            return len;
        }
    }*/

    //不支持变长数组
    if (!hasStatic && _ts.Test(']'))
        return -1;
    
    auto expr = ParseAssignExpr();
    EnsureInteger(expr);
    return Evaluator<long>().Eval(expr);
}


/*
 * Return: true, has ellipsis;
 */
bool Parser::ParseParamList(FuncType::TypeList& paramTypes)
{
    auto paramType = ParseParamDecl();
    paramTypes.push_back(paramType);
    /*
     * The parameter list is 'void'
     */
    if (paramType->ToVoidType()) {
        paramTypes.pop_back();
        return false;
    }

    while (_ts.Try(',')) {
        if (_ts.Try(Token::ELLIPSIS)) {
            return true;
        }

        auto tok = _ts.Peek();
        paramType = ParseParamDecl();
        if (paramType->ToVoidType()) {
            Error(tok, "'void' must be the only parameter");
        }

        paramTypes.push_back(paramType);
    }

    return false;
}


Type* Parser::ParseParamDecl(void)
{
    int storageSpec, funcSpec;
    auto type = ParseDeclSpec(&storageSpec, &funcSpec);
    
    // No declarator
    if (_ts.Test(',') || _ts.Test(')'))
        return type;
    
    auto tokTypePair = ParseDeclarator(type);
    auto tok = tokTypePair.first;
    type = tokTypePair.second;
    if (tok == nullptr) { // Abstract declarator
        return type;
    }

    auto obj = ProcessDeclarator(tok, type, storageSpec, funcSpec);
    assert(obj->ToObject());

    return type;
}


Type* Parser::ParseAbstractDeclarator(Type* type)
{
    auto tokenTypePair = ParseDeclarator(type);
    auto tok = tokenTypePair.first;
    type = tokenTypePair.second;
    if (tok) { // Not a abstract declarator!
        Error(tok, "unexpected identifier '%s'",
                tok->Str().c_str());
    }
    return type;
    /*
    auto pointerType = ParsePointer(type);
    if (nullptr != pointerType->ToPointerType() && !_ts.Try('('))
        return pointerType;
    
    auto ret = ParseAbstractDeclarator(pointerType);
    _ts.Expect(')');
    auto newBase = ParseArrayFuncDeclarator(pointerType);
    
    return ModifyBase(ret, pointerType, newBase);
    */
}


Identifier* Parser::ParseDirectDeclarator(Type* type,
        int storageSpec, int funcSpec)
{
    auto tokenTypePair = ParseDeclarator(type);
    auto tok = tokenTypePair.first;
    type = tokenTypePair.second;
    if (tok == nullptr) {
        Error(_errTok, "expect identifier or '('");
    }

    return ProcessDeclarator(tok, type, storageSpec, funcSpec);
}


Declaration* Parser::ParseInitDeclarator(Identifier* ident)
{
    auto obj = ident->ToObject();
    if (!obj) { // Do not record function Declaration
        return nullptr;
    }

    auto name = obj->Name();
    if (_ts.Try('=')) {
        if ((_curScope->Type() != S_FILE) && obj->Linkage() != L_NONE) {
            Error(obj, "'%s' has both 'extern' and initializer", name.c_str());
        }

        if (!obj->Type()->Complete() && !obj->Type()->ToArrayType()) {
            Error(obj, "variable '%s' has initializer but incomplete type",
                obj->Name().c_str());
        }

        if (obj->HasInit()) {
            Error(obj, "redefinition of variable '%s'", name.c_str());
        }

        // There could be more than one declaration for 
        //     an object in the same scope.
        // But it must has external or internal linkage.
        // So, for external/internal objects,
        //     the initialization will always go to
        //     the first declaration. As the initialization 
        //     is evaluated at compile time, 
        //     the order doesn't matter.
        // For objects with no linkage, there is
        //     always only one declaration.
        // Once again, we need not to worry about 
        //     the order of the initialization.
        if (obj->Decl()) {
            ParseInitializer(obj->Decl(), obj->Type(), 0);
            return nullptr;
        } else {
            auto decl = Declaration::New(obj);
            ParseInitializer(decl, obj->Type(), 0);
            obj->SetDecl(decl);
            return decl;
        }
    }
    
    if (!obj->Type()->Complete()) {
        if (obj->Linkage() == L_NONE) {
            Error(obj, "storage size of '%s' isn’t known", name.c_str());
        }
        return nullptr; // Discards the incomplete object declarations
    }

    if (!obj->Decl()) {
        auto decl = Declaration::New(obj);
        obj->SetDecl(decl);
        return decl;
    }

    return nullptr;
}


void Parser::ParseInitializer(Declaration* decl, Type* type, int offset)
{
    auto arrType = type->ToArrayType();
    auto structType = type->ToStructUnionType();
    if (arrType) {
        if (!_ts.Test('{') && !_ts.Test(Token::STRING_LITERAL)) {
            _ts.Expect('{');
        }
        return ParseArrayInitializer(decl, arrType, offset);
    } else if (structType) {
        if (!_ts.Test('{')) {
            _ts.Expect('{');
        }
        return ParseStructInitializer(decl, structType, offset);
    }

    // Scalar type
    auto hasBrace = _ts.Try('{');
    Expr* expr = ParseAssignExpr();
    if (hasBrace) {
        _ts.Try(',');
        _ts.Expect('}');
    }

    decl->AddInit(offset, type, expr);
}


void Parser::ParseLiteralInitializer(Declaration* decl,
        ArrayType* type, int offset)
{
    auto literal = ParseLiteral(_ts.Next());
    auto tok = literal->Tok();

    if (!type->Complete())
        type->SetLen(literal->SVal()->size() + 1);

    auto width = std::min(static_cast<size_t>(type->Len()),
            literal->SVal()->size() + 1);
    
    auto str = literal->SVal()->c_str();    
    for (; width >= 8; width -= 8) {
        auto p = reinterpret_cast<const long*>(str);
        auto type = ArithmType::New(T_LONG);
        auto val = Constant::New(tok, T_LONG, static_cast<long>(*p));
        decl->Inits().push_back({offset, type, val});
        offset += 8;
        str += 8;
    }

    for (; width >= 4; width -= 4) {
        auto p = reinterpret_cast<const int*>(str);
        auto type = ArithmType::New(T_INT);
        auto val = Constant::New(tok, T_INT, static_cast<long>(*p));
        decl->Inits().push_back({offset, type, val});
        offset += 4;
        str += 4;
    }

    for (; width >= 2; width -= 2) {
        auto p = reinterpret_cast<const short*>(str);
        auto type = ArithmType::New(T_SHORT);
        auto val = Constant::New(tok, T_SHORT, static_cast<long>(*p));
        decl->Inits().push_back({offset, type, val});
        offset += 2;
        str += 2;
    }

    for (; width >= 1; width--) {
        auto p = str;
        auto type = ArithmType::New(T_CHAR);
        auto val = Constant::New(tok, T_CHAR, static_cast<long>(*p));
        decl->Inits().push_back({offset, type, val});
        offset++;
        str++;
    }
}


void Parser::ParseArrayInitializer(Declaration* decl,
        ArrayType* type, int offset)
{
    assert(type);

    auto width = type->Derived()->Width();
    long idx = 0;

    auto hasBrace = _ts.Try('{');
    if (_ts.Test(Token::STRING_LITERAL) && width == 1) {
        // TODO(wgtdkp): handle wide character
        ParseLiteralInitializer(decl, type, offset);
        if (hasBrace) {
            _ts.Try(',');
            if (!_ts.Try('}')) {
                Error(_ts.Peek(), "excess elements in array initializer");
            }
        }
        return;
    }

    while (true) {
        if (_ts.Test('}')) {
            if (hasBrace)
                _ts.Next();
            goto end;
        }

        if (hasBrace && _ts.Try('[')) {
            auto expr = ParseAssignExpr();
            EnsureInteger(expr);
            idx = Evaluator<long>().Eval(expr);
            _ts.Expect(']');
            _ts.Expect('=');

            if (idx < 0 || (type->HasLen() && idx >= type->Len())) {
                Error(_ts.Peek(), "excess elements in array initializer");
            }
        } else if (!hasBrace && (_ts.Test('.') || _ts.Test('['))) {
            _ts.PutBack(); // Put the read comma(',') back
            goto end;
        }

        ParseInitializer(decl, type->Derived(), idx * width);
        ++idx;

        if (type->HasLen() && idx >= type->Len())
            break;

        // Needless comma at the end is allowed
        if (!_ts.Try(',')) {
            if (hasBrace)
                _ts.Expect('}');
            goto end;
        }
    }

    if (hasBrace) {
        _ts.Try(',');
        if (!_ts.Try('}')) {
            Error(_ts.Peek(), "excess elements in array initializer");
        }
    }
end:
    if (!type->HasLen())
        type->SetLen(idx);
}


void Parser::ParseStructInitializer(Declaration* decl,
        StructUnionType* type, int offset)
{
    assert(type);
    auto hasBrace = _ts.Try('{');
    auto member = type->Members().begin();
    while (true) {
        if (_ts.Test('}')) {
            if (hasBrace)
                _ts.Next();
            return;
        }

        if (hasBrace && _ts.Try('.')) {
            auto memberTok = _ts.Expect(Token::IDENTIFIER);
            _ts.Expect('=');
            
            auto name = memberTok->Str();
            auto iter = type->Members().begin();
            for (; iter != type->Members().end(); iter++)
                if ((*iter)->Name() == name)
                    break;

            member = iter;            
            if (member == type->Members().end()) {
                Error(_ts.Peek(), "excess elements in array initializer");
            }
        } else if (!hasBrace && (_ts.Test('.') || _ts.Test('['))) {
            _ts.PutBack(); // Put the read comma(',') back
            return;
        }
        
        

        ParseInitializer(decl, (*member)->Type(),
                offset + (*member)->Offset());
        member++;

        // Union, just init the first member
        if (!type->IsStruct()) {
            break;
        }

        if (member == type->Members().end())
            break;

        // Needless comma at the end is allowed
        if (!_ts.Try(',')) {
            if (hasBrace)
                _ts.Expect('}');
            return;
        }
    }

    if (hasBrace) {
        _ts.Try(',');
        if (!_ts.Try('}')) {
            Error(_ts.Peek(), "excess elements in array initializer");
        }
    }
}


/*
 * Statements
 */

Stmt* Parser::ParseStmt(void)
{
    auto tok = _ts.Next();
    if (tok->IsEOF())
        Error(tok, "premature end of input");

    switch (tok->Tag()) {
    case ';':
        return EmptyStmt::New();
    case '{':
        return ParseCompoundStmt();
    case Token::IF:
        return ParseIfStmt();
    case Token::SWITCH:
        return ParseSwitchStmt();
    case Token::WHILE:
        return ParseWhileStmt();
    case Token::DO:
        return ParseDoStmt();
    case Token::FOR:
        return ParseForStmt();
    case Token::GOTO:
        return ParseGotoStmt();
    case Token::CONTINUE:
        return ParseContinueStmt();
    case Token::BREAK:
        return ParseBreakStmt();
    case Token::RETURN:
        return ParseReturnStmt();
    case Token::CASE:
        return ParseCaseStmt();
    case Token::DEFAULT:
        return ParseDefaultStmt();
    }

    if (tok->IsIdentifier() && _ts.Try(':'))
        return ParseLabelStmt(tok);
    
    _ts.PutBack();
    auto expr = ParseExpr();
    _ts.Expect(';');

    return expr;
}


CompoundStmt* Parser::ParseCompoundStmt(FuncType* funcType)
{
    if (!funcType)
        EnterBlock();

    std::list<Stmt*> stmts;

    while (!_ts.Try('}')) {
        if (_ts.Peek()->IsEOF()) {
            Error(_ts.Peek(), "premature end of input");
        }

        if (IsType(_ts.Peek())) {
            stmts.push_back(ParseDecl());
        } else {
            stmts.push_back(ParseStmt());
        }
    }

    auto scope = funcType ? _curScope: ExitBlock();

    return CompoundStmt::New(stmts, scope);
}


IfStmt* Parser::ParseIfStmt(void)
{
    _ts.Expect('(');
    auto tok = _ts.Peek();
    auto cond = ParseExpr();
    if (!cond->Type()->IsScalar()) {
        Error(tok, "expect scalar");
    }
    _ts.Expect(')');

    auto then = ParseStmt();
    Stmt* els = nullptr;
    if (_ts.Try(Token::ELSE))
        els = ParseStmt();
    
    return IfStmt::New(cond, then, els);
}


/*
 * for 循环结构：
 *      for (declaration; expression1; expression2) statement
 * 展开后的结构：
 *		declaration
 * cond: if (expression1) then empty
 *		else goto end
 *		statement
 * step: expression2
 *		goto cond
 * next:
 */

#define ENTER_LOOP_BODY(breakDest, continueDest)    \
{											        \
    LabelStmt* breakDestBackup = _breakDest;	    \
    LabelStmt* continueDestBackup = _continueDest;  \
    _breakDest = breakDest;			                \
    _continueDest = continueDest; 

#define EXIT_LOOP_BODY()		        \
    _breakDest = breakDestBackup;       \
    _continueDest = continueDestBackup;	\
}

CompoundStmt* Parser::ParseForStmt(void)
{
    EnterBlock();
    _ts.Expect('(');
    
    std::list<Stmt*> stmts;

    if (IsType(_ts.Peek())) {
        stmts.push_back(ParseDecl());
    } else if (!_ts.Try(';')) {
        stmts.push_back(ParseExpr());
        _ts.Expect(';');
    }

    Expr* condExpr = nullptr;
    if (!_ts.Try(';')) {
        condExpr = ParseExpr();
        _ts.Expect(';');
    }

    Expr* stepExpr = nullptr;
    if (!_ts.Try(')')) {
        stepExpr = ParseExpr();
        _ts.Expect(')');
    }

    auto condLabel = LabelStmt::New();
    auto stepLabel = LabelStmt::New();
    auto endLabel = LabelStmt::New();
    stmts.push_back(condLabel);
    if (nullptr != condExpr) {
        auto gotoEndStmt = JumpStmt::New(endLabel);
        auto ifStmt = IfStmt::New(condExpr, EmptyStmt::New(), gotoEndStmt);
        stmts.push_back(ifStmt);
    }

    //我们需要给break和continue语句提供相应的标号，不然不知往哪里跳
    Stmt* bodyStmt;
    ENTER_LOOP_BODY(endLabel, condLabel);
    bodyStmt = ParseStmt();
    //因为for的嵌套结构，在这里需要回复break和continue的目标标号
    EXIT_LOOP_BODY()
    
    stmts.push_back(bodyStmt);
    stmts.push_back(stepLabel);
    stmts.push_back(stepExpr);
    stmts.push_back(JumpStmt::New(condLabel));
    stmts.push_back(endLabel);

    auto scope = _curScope;
    ExitBlock();
    
    return CompoundStmt::New(stmts, scope);
}


/*
 * while 循环结构：
 * while (expression) statement
 * 展开后的结构：
 * cond: if (expression1) then empty
 *		else goto end
 *		statement
 *		goto cond
 * end:
 */
CompoundStmt* Parser::ParseWhileStmt(void)
{
    std::list<Stmt*> stmts;
    _ts.Expect('(');
    auto tok = _ts.Peek();
    auto condExpr = ParseExpr();
    _ts.Expect(')');

    if (!condExpr->Type()->IsScalar()) {
        Error(tok, "scalar expression expected");
    }

    auto condLabel = LabelStmt::New();
    auto endLabel = LabelStmt::New();
    auto gotoEndStmt = JumpStmt::New(endLabel);
    auto ifStmt = IfStmt::New(condExpr, EmptyStmt::New(), gotoEndStmt);
    stmts.push_back(condLabel);
    stmts.push_back(ifStmt);
    
    Stmt* bodyStmt;
    ENTER_LOOP_BODY(endLabel, condLabel)
    bodyStmt = ParseStmt();
    EXIT_LOOP_BODY()
    
    stmts.push_back(bodyStmt);
    stmts.push_back(JumpStmt::New(condLabel));
    stmts.push_back(endLabel);
    
    return CompoundStmt::New(stmts);
}


/*
 * do-while 循环结构：
 *      do statement while (expression)
 * 展开后的结构：
 * begin: statement
 * cond: if (expression) then goto begin
 *		 else goto end
 * end:
 */
CompoundStmt* Parser::ParseDoStmt(void)
{
    auto beginLabel = LabelStmt::New();
    auto condLabel = LabelStmt::New();
    auto endLabel = LabelStmt::New();
    
    Stmt* bodyStmt;
    ENTER_LOOP_BODY(endLabel, beginLabel)
    bodyStmt = ParseStmt();
    EXIT_LOOP_BODY()

    _ts.Expect(Token::WHILE);
    _ts.Expect('(');
    auto condExpr = ParseExpr();
    _ts.Expect(')');
    _ts.Expect(';');

    auto gotoBeginStmt = JumpStmt::New(beginLabel);
    auto gotoEndStmt = JumpStmt::New(endLabel);
    auto ifStmt = IfStmt::New(condExpr, gotoBeginStmt, gotoEndStmt);

    std::list<Stmt*> stmts;
    stmts.push_back(beginLabel);
    stmts.push_back(bodyStmt);
    stmts.push_back(condLabel);
    stmts.push_back(ifStmt);
    stmts.push_back(endLabel);
    
    return CompoundStmt::New(stmts);
}


#define ENTER_SWITCH_BODY(breakDest, caseLabels)    \
{ 												    \
    CaseLabelList* caseLabelsBackup = _caseLabels;  \
    LabelStmt* defaultLabelBackup = _defaultLabel;  \
    LabelStmt* breakDestBackup = _breakDest;        \
    _breakDest = breakDest;                         \
    _caseLabels = &caseLabels; 

#define EXIT_SWITCH_BODY()			    \
    _caseLabels = caseLabelsBackup;     \
    _breakDest = breakDestBackup;       \
    _defaultLabel = defaultLabelBackup;	\
}


/*
 * switch
 *  jump stmt (skip case labels)
 *  case labels
 *  jump stmts
 *  default jump stmt
 */
CompoundStmt* Parser::ParseSwitchStmt(void)
{
    std::list<Stmt*> stmts;
    _ts.Expect('(');
    auto tok = _ts.Peek();
    auto expr = ParseExpr();
    _ts.Expect(')');

    if (!expr->Type()->IsInteger()) {
        Error(tok, "switch quantity not an integer");
    }

    auto testLabel = LabelStmt::New();
    auto endLabel = LabelStmt::New();
    auto t = TempVar::New(expr->Type());
    auto assign = BinaryOp::New(tok, '=', t, expr);
    stmts.push_back(assign);
    stmts.push_back(JumpStmt::New(testLabel));

    CaseLabelList caseLabels;
    ENTER_SWITCH_BODY(endLabel, caseLabels);

    auto bodyStmt = ParseStmt(); // Fill caseLabels and defaultLabel
    stmts.push_back(bodyStmt);
    stmts.push_back(JumpStmt::New(endLabel));
    stmts.push_back(testLabel);

    for (auto iter = caseLabels.begin();
            iter != caseLabels.end(); iter++) {
        auto cond = BinaryOp::New(tok, Token::EQ_OP, t, iter->first);
        auto then = JumpStmt::New(iter->second);
        auto ifStmt = IfStmt::New(cond, then, nullptr);
        stmts.push_back(ifStmt);
    }
    
    stmts.push_back(JumpStmt::New(_defaultLabel));
    EXIT_SWITCH_BODY();

    stmts.push_back(endLabel);

    return CompoundStmt::New(stmts);
}


CompoundStmt* Parser::ParseCaseStmt(void)
{
    auto expr = ParseExpr();
    _ts.Expect(':');
    
    auto val = Evaluator<long>().Eval(expr);
    auto cons = Constant::New(expr->Tok(), T_INT, val);

    auto labelStmt = LabelStmt::New();
    _caseLabels->push_back(std::make_pair(cons, labelStmt));
    
    std::list<Stmt*> stmts;
    stmts.push_back(labelStmt);
    stmts.push_back(ParseStmt());
    
    return CompoundStmt::New(stmts);
}


CompoundStmt* Parser::ParseDefaultStmt(void)
{
    auto tok = _ts.Peek();
    _ts.Expect(':');
    if (_defaultLabel != nullptr) { // There is a 'default' stmt
        Error(tok, "multiple default labels in one switch");
    }
    auto labelStmt = LabelStmt::New();
    _defaultLabel = labelStmt;
    
    std::list<Stmt*> stmts;
    stmts.push_back(labelStmt);
    stmts.push_back(ParseStmt());
    
    return CompoundStmt::New(stmts);
}


JumpStmt* Parser::ParseContinueStmt(void)
{
    auto tok = _ts.Peek();
    _ts.Expect(';');
    if (_continueDest == nullptr) {
        Error(tok, "'continue' is allowed only in loop");
    }
    
    return JumpStmt::New(_continueDest);
}


JumpStmt* Parser::ParseBreakStmt(void)
{
    auto tok = _ts.Peek();
    _ts.Expect(';');
    // ERROR(wgtdkp):
    if (_breakDest == nullptr) {
        Error(tok, "'break' is allowed only in switch/loop");
    }
    
    return JumpStmt::New(_breakDest);
}


ReturnStmt* Parser::ParseReturnStmt(void)
{
    Expr* expr;

    if (_ts.Try(';')) {
        expr = nullptr;
    } else {
        expr = ParseExpr();
        _ts.Expect(';');
        
        auto retType = _curFunc->Type()->ToFuncType()->Derived();
        expr = Expr::MayCast(expr, retType);
    }

    return ReturnStmt::New(expr);
}


JumpStmt* Parser::ParseGotoStmt(void)
{
    auto label = _ts.Peek();
    _ts.Expect(Token::IDENTIFIER);
    _ts.Expect(';');

    auto labelStmt = FindLabel(label->Str());
    if (labelStmt) {
        return JumpStmt::New(labelStmt);
    }
    
    auto unresolvedJump = JumpStmt::New(nullptr);;
    _unresolvedJumps.push_back(std::make_pair(label, unresolvedJump));
    
    return unresolvedJump;
}


CompoundStmt* Parser::ParseLabelStmt(const Token* label)
{
    auto labelStr = label->Str();
    auto stmt = ParseStmt();
    if (nullptr != FindLabel(labelStr)) {
        Error(label, "redefinition of label '%s'", labelStr.c_str());
    }

    auto labelStmt = LabelStmt::New();
    AddLabel(labelStr, labelStmt);
    std::list<Stmt*> stmts;
    stmts.push_back(labelStmt);
    stmts.push_back(stmt);

    return CompoundStmt::New(stmts);
}
