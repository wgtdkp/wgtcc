#include "ast.h"

#include "code_gen.h"
#include "error.h"
#include "mem_pool.h"
#include "parser.h"
#include "token.h"
#include "evaluator.h"


static MemPoolImp<BinaryOp>         binaryOpPool;
static MemPoolImp<ConditionalOp>    conditionalOpPool;
static MemPoolImp<FuncCall>         funcCallPool;
static MemPoolImp<Declaration>   initializationPool;
static MemPoolImp<Object>           objectPool;
static MemPoolImp<Identifier>       identifierPool;
static MemPoolImp<Enumerator>       enumeratorPool;
static MemPoolImp<Constant>         constantPool;
static MemPoolImp<TempVar>          tempVarPool;
static MemPoolImp<UnaryOp>          unaryOpPool;
static MemPoolImp<EmptyStmt>        emptyStmtPool;
static MemPoolImp<IfStmt>           ifStmtPool;
static MemPoolImp<JumpStmt>         jumpStmtPool;
static MemPoolImp<ReturnStmt>       returnStmtPool;
static MemPoolImp<LabelStmt>        labelStmtPool;
static MemPoolImp<CompoundStmt>     compoundStmtPool;
static MemPoolImp<FuncDef>          funcDefPool;



/*
 * Accept
 */

void Declaration::Accept(Visitor* v)
{
    v->VisitDeclaration(this);
}

void EmptyStmt::Accept(Visitor* v) {
    //assert(false);
    //v->VisitEmptyStmt(this);
}

void LabelStmt::Accept(Visitor* v) {
    v->VisitLabelStmt(this);
}

void IfStmt::Accept(Visitor* v) {
    v->VisitIfStmt(this);
}

void JumpStmt::Accept(Visitor* v) {
    v->VisitJumpStmt(this);
}

void ReturnStmt::Accept(Visitor* v) {
    v->VisitReturnStmt(this);
}

void CompoundStmt::Accept(Visitor* v) {
    v->VisitCompoundStmt(this);
}

void BinaryOp::Accept(Visitor* v) {
    v->VisitBinaryOp(this);
}

void UnaryOp::Accept(Visitor* v) {
    v->VisitUnaryOp(this);
}

void ConditionalOp::Accept(Visitor* v) {
    v->VisitConditionalOp(this);
}

void FuncCall::Accept(Visitor* v) { 
    v->VisitFuncCall(this);
}

void Identifier::Accept(Visitor* v) {
    v->VisitIdentifier(this);
}

void Object::Accept(Visitor* v) {
    v->VisitObject(this);
}

void Constant::Accept(Visitor* v) {
    v->VisitConstant(this);
}

void Enumerator::Accept(Visitor* v)
{
    v->VisitEnumerator(this);
}

void TempVar::Accept(Visitor* v) {
    v->VisitTempVar(this);
}

void FuncDef::Accept(Visitor* v) {
    v->VisitFuncDef(this);
}

void TranslationUnit::Accept(Visitor* v) {
    v->VisitTranslationUnit(this);
}


// Casting array to pointer, function to pointer to function
Expr* Expr::MayCast(Expr* expr)
{
    auto type = Type::MayCast(expr->Type());
    // If the types are equal, no need cast
    if (type != expr->Type()) { // Pointer comparison is enough
        return UnaryOp::New(Token::CAST, expr, type);
    }
    return expr;
}


Expr* Expr::MayCast(Expr* expr, ::Type* desType)
{
    expr = MayCast(expr);
    if (*desType != *expr->Type())
        expr = UnaryOp::New(Token::CAST, expr, desType);
    return expr;
}


BinaryOp* BinaryOp::New(const Token* tok, Expr* lhs, Expr* rhs)
{
    return New(tok, tok->Tag(), lhs, rhs);
}

BinaryOp* BinaryOp::New(const Token* tok, int op, Expr* lhs, Expr* rhs)
{
    switch (op) {
    case ',':
    case '.':
    case '=': 
    case '*':
    case '/':
    case '%':
    case '+':
    case '-':
    case '&':
    case '^':
    case '|':
    case '<':
    case '>':
    case Token::LEFT_OP:
    case Token::RIGHT_OP:
    case Token::LE_OP:
    case Token::GE_OP:
    case Token::EQ_OP:
    case Token::NE_OP: 
    case Token::AND_OP:
    case Token::OR_OP:
        break;

    default:
        assert(0);
    }

    auto ret = new (binaryOpPool.Alloc()) BinaryOp(tok, op, lhs, rhs);
    ret->_pool = &binaryOpPool;
    
    ret->TypeChecking();
    return ret;    
}


ArithmType* BinaryOp::Promote(void)
{
    // Both lhs and rhs are ensured to be have arithmetic type
    auto lhsType = _lhs->Type()->ToArithmType();
    auto rhsType = _rhs->Type()->ToArithmType();
    
    auto type = MaxType(lhsType, rhsType);
    if (lhsType != type) {// Pointer comparation is enough!
        _lhs = UnaryOp::New(Token::CAST, _lhs, type);
    }
    if (rhsType != type) {
        _rhs = UnaryOp::New(Token::CAST, _rhs, type);
    }
    
    return type;
}


/*
 * Type checking
 */

void BinaryOp::TypeChecking(void)
{
    switch (_op) {
    case '.':
        return MemberRefOpTypeChecking();

    //case ']':
    //    return SubScriptingOpTypeChecking();

    case '*':
    case '/':
    case '%':
        return MultiOpTypeChecking();

    case '+':
    case '-':
        return AdditiveOpTypeChecking();

    case Token::LEFT_OP:
    case Token::RIGHT_OP:
        return ShiftOpTypeChecking();

    case '<':
    case '>':
    case Token::LE_OP:
    case Token::GE_OP:
        return RelationalOpTypeChecking();

    case Token::EQ_OP:
    case Token::NE_OP:
        return EqualityOpTypeChecking();

    case '&':
    case '^':
    case '|':
        return BitwiseOpTypeChecking();

    case Token::AND_OP:
    case Token::OR_OP:
        return LogicalOpTypeChecking();

    case '=':
        return AssignOpTypeChecking();

    default:
        assert(0);
    }
}

void BinaryOp::SubScriptingOpTypeChecking(void)
{
    auto lhsType = _lhs->Type()->ToPointerType();
    if (nullptr == lhsType) {
        Error(_tok, "an pointer expected");
    }
    if (!_rhs->Type()->IsInteger()) {
        Error(_tok, "the operand of [] should be intger");
    }

    // The type of [] operator is the derived type
    _type = lhsType->Derived();    
}

void BinaryOp::MemberRefOpTypeChecking(void)
{
    _type = _rhs->Type();
}

void BinaryOp::MultiOpTypeChecking(void)
{
    auto lhsType = _lhs->Type()->ToArithmType();
    auto rhsType = _rhs->Type()->ToArithmType();
    if (lhsType == nullptr || rhsType == nullptr) {
        Error(_tok, "operands should have arithmetic type");
    }

    if ('%' == _op && 
            !(_lhs->Type()->IsInteger()
            && _rhs->Type()->IsInteger())) {
        Error(_tok, "operands of '%%' should be integers");
    }

    //TODO: type promotion
    _type = Promote();
}

/*
 * Additive operator is only allowed between:
 *  1. arithmetic types (bool, interger, floating)
 *  2. pointer can be used:
 *     1. lhs of MINUS operator, and rhs must be integer or pointer;
 *     2. lhs/rhs of ADD operator, and the other operand must be integer;
 */
void BinaryOp::AdditiveOpTypeChecking(void)
{
    auto lhsType = _lhs->Type()->ToPointerType();
    auto rhsType = _rhs->Type()->ToPointerType();
    if (lhsType) {
        if (_op == Token::MINUS) {
            if ((rhsType && *lhsType != *rhsType)
                    || !_rhs->Type()->IsInteger()) {
                Error(_tok, "invalid operands to binary -");
            }
            _type = ArithmType::New(T_LONG); // ptrdiff_t
        } else if (!_rhs->Type()->IsInteger()) {
            Error(_tok, "invalid operands to binary -");
        } else {
            _type = _lhs->Type();
        }
    } else if (rhsType) {
        if (_op != Token::ADD || !_lhs->Type()->IsInteger()) {
            Error(_tok, "invalid operands to binary '%s'",
                    _tok->Str().c_str());
        }
        _type = _rhs->Type();
        std::swap(_lhs, _rhs); // To simplify code gen
    } else {
        auto lhsType = _lhs->Type()->ToArithmType();
        auto rhsType = _rhs->Type()->ToArithmType();
        if (lhsType == nullptr || rhsType == nullptr) {
            Error(_tok, "invalid operands to binary %s",
                    _tok->Str().c_str());
        }

        _type = Promote();
    }
}

void BinaryOp::ShiftOpTypeChecking(void)
{
    if (!_lhs->Type()->IsInteger() || !_rhs->Type()->IsInteger()) {
        Error(_tok, "expect integers for shift operator '%s'",
                _tok->Str());
    }

    Promote();
    _type = _lhs->Type();
}

void BinaryOp::RelationalOpTypeChecking(void)
{
    if (!_lhs->Type()->IsReal() || !_rhs->Type()->IsReal()) {
        Error(_tok, "expect integer/float"
                " for relational operator '%s'", _tok->Str());
    }

    Promote();
    _type = ArithmType::New(T_INT);    
}

void BinaryOp::EqualityOpTypeChecking(void)
{
    auto lhsType = _lhs->Type()->ToPointerType();
    auto rhsType = _rhs->Type()->ToPointerType();
    if (lhsType || rhsType) {
        if (!lhsType->Compatible(*rhsType)) {
            Error(_tok, "incompatible pointers of operands");
        }
    } else {
        auto lhsType = _lhs->Type()->ToArithmType();
        auto rhsType = _rhs->Type()->ToArithmType();
        if (lhsType == nullptr || rhsType == nullptr) {
            Error(_tok, "invalid operands to binary %s",
                    _tok->Str());
        }

        Promote();
    }

    _type = ArithmType::New(T_INT);    
}

void BinaryOp::BitwiseOpTypeChecking(void)
{
    if (!_lhs->Type()->IsInteger() || !_rhs->Type()->IsInteger()) {
        Error(_tok, "operands of '&' should be integer");
    }
    
    _type = Promote();
}

void BinaryOp::LogicalOpTypeChecking(void)
{
    if (!_lhs->Type()->IsScalar() || !_rhs->Type()->IsScalar()) {
        Error(_tok, "the operand should be arithmetic type or pointer");
    }
    
    Promote();
    _type = ArithmType::New(T_INT);
}

void BinaryOp::AssignOpTypeChecking(void)
{
    if (!_lhs->IsLVal()) {
        Error(_lhs->Tok(), "lvalue expression expected");
    } else if (_lhs->Type()->IsConst()) {
        Error(_lhs->Tok(), "can't modifiy 'const' qualified expression");
    } else if (!_lhs->Type()->Compatible(*_rhs->Type())) {
        Error(_lhs->Tok(), "uncompatible types");
    }
    // The other constraints are lefted to cast operator
    _rhs = Expr::MayCast(_rhs, _lhs->Type());
    _type = _lhs->Type();
}


/*
 * Unary Operators
 */

UnaryOp* UnaryOp::New(int op, Expr* operand, ::Type* type)
{
    auto ret = new (unaryOpPool.Alloc()) UnaryOp(op, operand, type);
    ret->_pool = &unaryOpPool;
    
    ret->TypeChecking();
    return ret;
}

bool UnaryOp::IsLVal(void) {
    // only deref('*') could be lvalue;
    // so it's only deref will override this func
    switch (_op) {
    case Token::DEREF: return !Type()->ToArrayType();
    //case Token::CAST: return _operand->IsLVal();
    default: return false;
    }
}


ArithmType* UnaryOp::Promote(void)
{
    //
    auto arithmType = _operand->Type()->ToArithmType(); 
    auto type = MaxType(arithmType, arithmType);
    if (type != arithmType) {
        _operand = UnaryOp::New(Token::CAST, _operand, type);
    }
    return type;
}


void UnaryOp::TypeChecking(void)
{
    switch (_op) {
    case Token::POSTFIX_INC:
    case Token::POSTFIX_DEC:
    case Token::PREFIX_INC:
    case Token::PREFIX_DEC:
        return IncDecOpTypeChecking();

    case Token::ADDR:
        return AddrOpTypeChecking();

    case Token::DEREF:
        return DerefOpTypeChecking();

    case Token::PLUS:
    case Token::MINUS:
    case '~':
    case '!':
        return UnaryArithmOpTypeChecking();

    case Token::CAST:
        return CastOpTypeChecking();

    default:
        assert(false);
    }
}

void UnaryOp::IncDecOpTypeChecking(void)
{
    if (!_operand->IsLVal()) {
        Error(_tok, "lvalue expression expected");
    } else if (_operand->Type()->IsConst()) {
        Error(_tok, "can't modifiy 'const' qualified expression");
    }

    _type = _operand->Type();
}

void UnaryOp::AddrOpTypeChecking(void)
{
    FuncType* funcType = _operand->Type()->ToFuncType();
    if (funcType == nullptr && !_operand->IsLVal()) {
        Error(_tok, "expression must be an lvalue or function designator");
    }
    
    _type = PointerType::New(_operand->Type());
}

void UnaryOp::DerefOpTypeChecking(void)
{
    auto pointerType = _operand->Type()->ToPointerType();
    if (pointerType == nullptr) {
        Error(_tok, "pointer expected for deref operator '*'");
    }

    _type = pointerType->Derived();    
}

void UnaryOp::UnaryArithmOpTypeChecking(void)
{
    if (Token::PLUS == _op || Token::MINUS == _op) {
        if (!_operand->Type()->IsArithm()) {
            Error(_tok, "Arithmetic type expected");
        }
        Promote();
        _type = _operand->Type();
    } else if ('~' == _op) {
        if (!_operand->Type()->IsInteger()) {
            Error(_tok, "integer expected for operator '~'");
        }
        Promote();
        _type = _operand->Type();
    } else if (!_operand->Type()->IsScalar()) {
        Error(_tok, "arithmetic type or pointer expected for operator '!'");
    } else {
        _type = ArithmType::New(T_INT);
    }
}


void UnaryOp::CastOpTypeChecking(void)
{
    // The _type has been initiated to dest type
    if (_type->ToVoidType()) {
        // The expression becomes a void expression
    } else if (_type->ToPointerType() && _operand->Type()->ToArrayType()) {
    } else if (_type->ToPointerType() && _operand->Type()->ToFuncType()) {
    } else if (!_type->IsScalar() || !_operand->Type()->IsScalar()) {
        Error(_tok, "the cast type should be arithemetic type or pointer");
    } else if (_type->IsFloat() && _operand->Type()->ToPointerType()) {
        Error(_tok, "can't cast a pointer to floating");
    } else if (_type->ToPointerType() && _operand->Type()->IsFloat()) {
        Error(_tok, "can't cast a floating to pointer");
    }
}


/*
 * Conditional Operator
 */

ConditionalOp* ConditionalOp::New(const Token* tok,
        Expr* cond, Expr* exprTrue, Expr* exprFalse)
{
    auto ret = new (conditionalOpPool.Alloc())
            ConditionalOp(cond, exprTrue, exprFalse);
    ret->_pool = &conditionalOpPool;

    ret->TypeChecking();
    return ret;
}


ArithmType* ConditionalOp::Promote(void)
{
    auto lhsType = _exprTrue->Type()->ToArithmType();
    auto rhsType = _exprFalse->Type()->ToArithmType();
    
    auto type = MaxType(lhsType, rhsType);
    if (lhsType != type) {// Pointer comparation is enough!
        _exprTrue = UnaryOp::New(Token::CAST, _exprTrue, type);
    }
    if (rhsType != type) {
        _exprFalse = UnaryOp::New(Token::CAST, _exprFalse, type);
    }
    
    return type;
}

void ConditionalOp::TypeChecking(void)
{
    if (!_cond->Type()->IsScalar()) {
        Error(_cond->Tok(), "scalar is required");
    }

    auto lhsType = _exprTrue->Type();
    auto rhsType = _exprFalse->Type();
    if (!lhsType->Compatible(*rhsType)) {
        Error(_exprTrue->Tok(), "incompatible types of true/false expression");
    } else {
        _type = lhsType;
    }

    lhsType = lhsType->ToArithmType();
    rhsType = rhsType->ToArithmType();
    if (lhsType && rhsType) {
        _type = Promote();
    }
}


/*
 * Function Call
 */

FuncCall* FuncCall::New(Expr* designator, const ArgList& args)
{
    auto ret = new (funcCallPool.Alloc()) FuncCall(designator, args);
    ret->_pool = &funcCallPool;

    ret->TypeChecking();
    return ret;
}


void FuncCall::TypeChecking(void)
{
    auto pointerType = _designator->Type()->ToPointerType();
    if (pointerType) {
        if (!pointerType->Derived()->ToFuncType()) {
            Error(_designator, "called object is not a function or function pointer");
        }
        // Convert function pointer to function type
        _designator = UnaryOp::New(Token::DEREF, _designator);
    }
    auto funcType = _designator->Type()->ToFuncType();
    if (!funcType) {
        Error(_designator, "called object is not a function or function pointer");
    }

    auto funcName = _designator->Tok()->Str();
    auto arg = _args.begin();
    for (auto paramType: funcType->ParamTypes()) {
        if (arg == _args.end()) {
            Error(_tok, "too few arguments for function '%s'", funcName.c_str());
        }
        if (!paramType->Compatible(*(*arg)->Type())) {
            // TODO(wgtdkp): function name
            Error(_tok, "incompatible type for argument 1 of '%s'", funcName.c_str());
        }
        *arg = Expr::MayCast(*arg, paramType);
        ++arg;
    }
    
    if (arg != _args.end() && !funcType->Variadic()) {
        Error(_tok, "too many arguments for function '%s'", funcName.c_str());
    }

    // c11 6.5.2.2 [6]: promote float to double if it has no prototype
    while (arg != _args.end()) {
        if ((*arg)->Type()->IsFloat() && (*arg)->Type()->Width() == 4) {
            auto type = ArithmType::New(T_DOUBLE);
            *arg = UnaryOp::New(Token::CAST, *arg, type);
        }
        ++arg;
    }

    _type = funcType->Derived();
}


/*
 * Identifier
 */

Identifier* Identifier::New(const Token* tok,
        ::Type* type, enum Linkage linkage)
{
    auto ret = new (identifierPool.Alloc()) Identifier(tok, type, linkage);
    ret->_pool = &identifierPool;
    return ret;
}


Enumerator* Enumerator::New(const Token* tok, int val)
{
    auto ret = new (enumeratorPool.Alloc()) Enumerator(tok, val);
    ret->_pool = &enumeratorPool;
    return ret;
}


Declaration* Declaration::New(Object* obj)
{
    auto ret = new (initializationPool.Alloc()) Declaration(obj);
    ret->_pool = &initializationPool;
    return ret;
}

void Declaration::AddInit(Initializer init)
{
    init._expr = Expr::MayCast(init._expr, init._type);

    auto res = _inits.insert(init);
    if (!res.second) {
        _inits.erase(res.first);
        _inits.insert(init);
    }
}


/*
 * Object
 */

Object* Object::New(const Token* tok, ::Type* type, 
        int storage, enum Linkage linkage,
        unsigned char bitFieldBegin, unsigned char bitFieldWidth)
{
    auto ret = new (objectPool.Alloc())
            Object(tok, type, storage, linkage, bitFieldBegin, bitFieldWidth);
    ret->_pool = &objectPool;
    return ret;
}


/*
 * Constant
 */

Constant* Constant::New(const Token* tok, int tag, long val)
{
    auto type = ArithmType::New(tag);
    auto ret = new (constantPool.Alloc()) Constant(tok, type, val);
    ret->_pool = &constantPool;
    return ret;
}

Constant* Constant::New(const Token* tok, int tag, double val)
{
    auto type = ArithmType::New(tag);
    auto ret = new (constantPool.Alloc()) Constant(tok, type, val);
    ret->_pool = &constantPool;
    return ret;
}

Constant* Constant::New(const Token* tok, int tag, const std::string* val)
{
    static auto derived = ArithmType::New(tag);
    derived->SetQual(Q_CONST);
    static auto type = PointerType::New(derived);

    auto ret = new (constantPool.Alloc()) Constant(tok, type, val);
    ret->_pool = &constantPool;

    static long id = 0;
    ret->_id = id++;

    return ret;
}


std::string Constant::SValRepr(void) const
{
    std::vector<char> buf(4 * _sval->size() + 1);
    for (size_t i = 0; i < _sval->size(); ++i) {
        sprintf(&buf[i * 4], "\\%03o", (*_sval)[i]);
    }
    return std::string(buf.begin(), buf.end() - 1);
}


/*
 * TempVar
 */

TempVar* TempVar::New(::Type* type)
{
    auto ret = new (tempVarPool.Alloc()) TempVar(type);
    ret->_pool = &tempVarPool;
    return ret;
}


/*
 * Statement
 */

EmptyStmt* EmptyStmt::New(void)
{
    auto ret = new (emptyStmtPool.Alloc()) EmptyStmt();
    ret->_pool = &emptyStmtPool;
    return ret;
}

//else stmt Ĭ���� null
IfStmt* IfStmt::New(Expr* cond, Stmt* then, Stmt* els)
{
    auto ret = new (ifStmtPool.Alloc()) IfStmt(cond, then, els);
    ret->_pool = &ifStmtPool;
    return ret;
}

CompoundStmt* CompoundStmt::New(std::list<Stmt*>& stmts, ::Scope* scope)
{
    auto ret = new (compoundStmtPool.Alloc()) CompoundStmt(stmts, scope);
    ret->_pool = &compoundStmtPool;
    return ret;
}

JumpStmt* JumpStmt::New(LabelStmt* label)
{
    auto ret = new (jumpStmtPool.Alloc()) JumpStmt(label);
    ret->_pool = &jumpStmtPool;
    return ret;
}

ReturnStmt* ReturnStmt::New(Expr* expr)
{
    auto ret = new (returnStmtPool.Alloc()) ReturnStmt(expr);
    ret->_pool = &returnStmtPool;
    return ret;
}

LabelStmt* LabelStmt::New(void)
{
    auto ret = new (labelStmtPool.Alloc()) LabelStmt();
    ret->_pool = &labelStmtPool;
    return ret;
}

FuncDef* FuncDef::New(Identifier* ident, LabelStmt* retLabel)
{
    auto ret = new (funcDefPool.Alloc()) FuncDef(ident, retLabel);
    ret->_pool = &funcDefPool;

    return ret;
}
