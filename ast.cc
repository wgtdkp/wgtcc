#include "ast.h"

#include "error.h"
#include "parser.h"
#include "token.h"
#include "visitor.h"


/*
 * accept
 */

void EmptyStmt::Accept(Visitor* v) {
    v->VisitEmptyStmt(this);
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
/*
void Variable::Accept(Visitor* v) {
    v->VisitVariable(this);
}
*/
void Constant::Accept(Visitor* v) {
    v->VisitConstant(this);
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


long long BinaryOp::EvalInteger(const Token* errTok)
{
    // TypeChecking should make sure of this constrains
    assert(_ty->IsInteger());

#define L   _lhs->EvalInteger(errTok)
#define R   _rhs->EvalInteger(errTok)

    //bool res = true;
    switch (_op) {
    case '+':
        return L + R;
    case '-':
        return L - R;
    case '*':
        return L * R;
    case '/':
    case '%':
        {
            int l = L, r = R;
            if (r == 0)
                ;//Error(this, "division by zero");
            return _op == '%'? (l % r): (l / r);
        }
    case '<':
        return L < R;
    case '>':
        return L > R;
    case '|':
        return L | R;
    case '&':
        return L & R;
    case '^':
        return L ^ R;
    case Token::LEFT_OP:
        return L << R;
    case Token::RIGHT_OP:
        return L >> R;
    case Token::AND_OP:
        return L && R;
    case Token::OR_OP:
        return L || R;
    case '=':  
    case ',':
        return R;
    default:
        assert(0);
    }
}

void BinaryOp::TypeChecking(const Token* errTok)
{
    switch (_op) {
    case '[':
        return SubScriptingOpTypeChecking(errTok);

    case '*':
    case '/':
    case '%':
        return MultiOpTypeChecking(errTok);

    case '+':
    case '-':
        return AdditiveOpTypeChecking(errTok);

    case Token::LEFT_OP:
    case Token::RIGHT_OP:
        return ShiftOpTypeChecking(errTok);

    case '<':
    case '>':
    case Token::LE_OP:
    case Token::GE_OP:
        return RelationalOpTypeChecking(errTok);

    case Token::EQ_OP:
    case Token::NE_OP:
        return EqualityOpTypeChecking(errTok);

    case '&':
    case '^':
    case '|':
        return BitwiseOpTypeChecking(errTok);

    case Token::AND_OP:
    case Token::OR_OP:
        return LogicalOpTypeChecking(errTok);

    case '=':
        return AssignOpTypeChecking(errTok);

    default:
        assert(0);
    }
}

void BinaryOp::SubScriptingOpTypeChecking(const Token* errTok)
{
    auto lhsType = _lhs->Ty()->ToPointerType();
    if (nullptr == lhsType)
        Error(errTok->Coord(), "an pointer expected");
    if (!_rhs->Ty()->IsInteger())
        Error(errTok->Coord(), "the operand of [] should be intger");

    // The type of [] operator is the derived type
    _ty = lhsType->Derived();
}

void BinaryOp::MemberRefOpTypeChecking(
        const Token* errTok, const std::string& rhsName)
{
    StructUnionType* structUnionType;
    if (_op == Token::PTR_OP) {
        auto pointer = _lhs->Ty()->ToPointerType();
        if (pointer == nullptr) {
            Error(errTok->Coord(), "pointer expected for operator '->'");
        } else {
            structUnionType = pointer->Derived()->ToStructUnionType();
            if (structUnionType == nullptr)
                Error(errTok->Coord(), "pointer to struct/union expected");
        }
    } else {
        structUnionType = _lhs->Ty()->ToStructUnionType();
        if (structUnionType == nullptr)
            Error(errTok->Coord(), "an struct/union expected");
    }

    if (structUnionType == nullptr)
        return; // The _rhs is lefted nullptr

    _rhs = structUnionType->GetMember(rhsName);
    if (_rhs == nullptr) {
        Error(errTok->Coord(), "'%s' is not a member of '%s'", rhsName, "[obj]");
    }

    _ty = _rhs->Ty();
}

void BinaryOp::MultiOpTypeChecking(const Token* errTok)
{
    auto lhsType = _lhs->Ty()->ToArithmType();
    auto rhsType = _rhs->Ty()->ToArithmType();

    if (nullptr == lhsType || nullptr == rhsType)
        Error(errTok->Coord(), "operands should have arithmetic type");
    if ('%' == _op && !(_lhs->Ty()->IsInteger() && _rhs->Ty()->IsInteger()))
        Error(errTok->Coord(), "operands of '%%' should be integers");

    //TODO: type promotion
    _ty = _lhs->Ty();
}

/*
 * Additive operator is only allowed between:
 *  1. arithmetic types (bool, interger, floating)
 *  2. pointer can be used:
 *     1. lhs of MINUS operator, and rhs must be integer or pointer;
 *     2. lhs/rhs of ADD operator, and the other operand must be integer;
 */
void BinaryOp::AdditiveOpTypeChecking(const Token* errTok)
{
    auto lhsType = _lhs->Ty()->ToPointerType();
    auto rhsType = _rhs->Ty()->ToPointerType();
    if (lhsType) {
        if (_op == Token::MINUS) {
            if ((rhsType && *lhsType != *rhsType)
                    || !_rhs->Ty()->IsInteger()) {
                Error(errTok->Coord(), "invalid operands to binary -");
            }
        } else if (!_rhs->Ty()->IsInteger()) {
            Error(errTok->Coord(), "invalid operands to binary -");
        }
        _ty = _lhs->Ty();
    } else if (rhsType) {
        if (_op != Token::ADD || !_lhs->Ty()->IsInteger()) {
            Error(errTok->Coord(), "invalid operands to binary +");
        }
        _ty = _rhs->Ty();
    } else {
        auto lhsType = _lhs->Ty()->ToArithmType();
        auto rhsType = _rhs->Ty()->ToArithmType();
        if (lhsType == nullptr || rhsType == nullptr) {
            Error(errTok->Coord(), "invalid operands to binary %s",
                    errTok->Str());
        }

        if (lhsType->Width() < Type::_machineWord
                && rhsType->Width() < Type::_machineWord) {
            _ty = Type::NewArithmType(T_INT);
        } else if (lhsType->Width() > rhsType->Width()) {
            _ty = lhsType;
        } else if (lhsType->Width() < rhsType->Width()) {
            _ty = rhsType;
        } else if ((lhsType->Tag() & T_FLOAT) || (rhsType->Tag() & T_FLOAT)) {
            _ty = Type::NewArithmType(T_FLOAT);
        } else {
            _ty = lhsType;
        }
    }
}

void BinaryOp::ShiftOpTypeChecking(const Token* errTok)
{
    //TODO: type checking

    _ty = _lhs->Ty();
}

void BinaryOp::RelationalOpTypeChecking(const Token* errTok)
{
    //TODO: type checking

    _ty = Type::NewArithmType(T_BOOL);
}

void BinaryOp::EqualityOpTypeChecking(const Token* errTok)
{
    //TODO: type checking

    _ty = Type::NewArithmType(T_BOOL);
}

void BinaryOp::BitwiseOpTypeChecking(const Token* errTok)
{
    if (_lhs->Ty()->IsInteger() || _rhs->Ty()->IsInteger())
        Error(errTok->Coord(), "operands of '&' should be integer");
    
    //TODO: type promotion
    _ty = Type::NewArithmType(T_INT);
}

void BinaryOp::LogicalOpTypeChecking(const Token* errTok)
{
    //TODO: type checking
    if (!_lhs->Ty()->IsScalar() || !_rhs->Ty()->IsScalar())
        Error(errTok->Coord(), "the operand should be arithmetic type or pointer");
    
    _ty = Type::NewArithmType(T_BOOL);
}

void BinaryOp::AssignOpTypeChecking(const Token* errTok)
{
    //TODO: type checking
    if (!_lhs->IsLVal()) {
        //TODO: error
        Error(errTok->Coord(), "lvalue expression expected");
    } else if (_lhs->Ty()->IsConst()) {
        Error(errTok->Coord(), "can't modifiy 'const' qualified expression");
    }

    _ty = _lhs->Ty();
}


/*
 * Unary Operators
 */

bool UnaryOp::IsLVal(void) const {
    /*only deref('*') op is lvalue;
    so it's only deref will override this func*/
    return (Token::DEREF == _op);
}

long long UnaryOp::EvalInteger(const Token* errTok)
{
#define VAL _operand->EvalInteger(errTok)

    switch (_op) {
    case Token::PLUS:
        return VAL;
    case Token::MINUS:
        return -VAL;
    case '~':
        return ~VAL;
    case '!':
        return !VAL;
    case Token::CAST:
        return VAL;
    default:
        Error(errTok->Coord(), "expect constant integer");
    }

    return 0;   // Make compiler happy
}

void UnaryOp::TypeChecking(const Token* errTok)
{
    switch (_op) {
    case Token::POSTFIX_INC:
    case Token::POSTFIX_DEC:
    case Token::PREFIX_INC:
    case Token::PREFIX_DEC:
        return IncDecOpTypeChecking(errTok);

    case Token::ADDR:
        return AddrOpTypeChecking(errTok);

    case Token::DEREF:
        return DerefOpTypeChecking(errTok);

    case Token::PLUS:
    case Token::MINUS:
    case '~':
    case '!':
        return UnaryArithmOpTypeChecking(errTok);

    case Token::CAST:
        return CastOpTypeChecking(errTok);

    default:
        assert(false);
    }
}

void UnaryOp::IncDecOpTypeChecking(const Token* errTok)
{
    if (!_operand->IsLVal()) {
        //TODO: error
        Error(errTok->Coord(), "lvalue expression expected");
    } else if (_operand->Ty()->IsConst()) {
        Error(errTok->Coord(), "can't modifiy 'const' qualified expression");
    }

    _ty = _operand->Ty();
}

void UnaryOp::AddrOpTypeChecking(const Token* errTok)
{
    FuncType* funcType = _operand->Ty()->ToFuncType();
    if (funcType == nullptr && !_operand->IsLVal()) {
        Error(errTok->Coord(), "expression must be an lvalue or function designator");
    }
    
    _ty = Type::NewPointerType(_operand->Ty());
}

void UnaryOp::DerefOpTypeChecking(const Token* errTok)
{
    auto pointer = _operand->Ty()->ToPointerType();
    if (nullptr == pointer) {
        Error(errTok->Coord(), "pointer expected for deref operator '*'");
    }

    _ty = pointer->Derived();
}

void UnaryOp::UnaryArithmOpTypeChecking(const Token* errTok)
{
    if (Token::PLUS == _op || Token::MINUS == _op) {
        if (!_operand->Ty()->IsArithm())
            Error(errTok->Coord(), "Arithmetic type expected");
    } else if ('~' == _op) {
        if (!_operand->Ty()->IsInteger())
            Error(errTok->Coord(), "integer expected for operator '~'");
    } else {//'!'
        if (!_operand->Ty()->IsScalar())
            Error(errTok->Coord(), "arithmetic type or pointer expected for operator '!'");
    }

    _ty = _operand->Ty();
}

void UnaryOp::CastOpTypeChecking(const Token* errTok)
{
    //the _ty has been initiated to desType
    if (!_ty->IsScalar()) {
        Error(errTok->Coord(), "the cast type should be arithemetic type or pointer");
    }

    if (_ty->IsFloat() && nullptr != _operand->Ty()->ToPointerType()) {
        Error(errTok->Coord(), "can't cast a pointer to floating");
    } else if (nullptr != _ty->ToPointerType() && _operand->Ty()->IsFloat()) {
        Error(errTok->Coord(), "can't cast a floating to pointer");
    }
}

/*
 * Conditional Operator
 */

long long ConditionalOp::EvalInteger(const Token* errTok)
{
    int cond = _cond->EvalInteger(errTok);
    if (cond) {
        return _exprTrue->EvalInteger(errTok);
    } else {
        return _exprFalse->EvalInteger(errTok);
    }
}

void ConditionalOp::TypeChecking(const Token* errTok)
{

    //TODO: type checking

    //TODO: type evaluation
}


/*
 * Function Call
 */

void FuncCall::TypeChecking(const Token* errTok)
{
    auto funcType = _designator->Ty()->ToFuncType();
    if (nullptr == funcType)
        Error(errTok->Coord(), "not a function type");
    
    _ty = funcType->Derived();

    //TODO: check if args and params are compatible type

}

long long FuncCall::EvalInteger(const Token* errTok) {
    Error(errTok->Coord(),
            "function call is not allowed in constant expression");
    return 0;   // Make compiler happy
}

/*
 * Variable
 */
/*
Variable* Variable::GetStructMember(const char* name)
{
    auto type = _ty->ToStructUnionType();
    assert(type);

    auto member = type->Find(name);
    if (member == nullptr)
        return nullptr;

    member->SetOffset(member->Offset() + _offset);

    return member;
}

Variable* Variable::GetArrayElement(Parser* parser, size_t idx)
{
    auto type = _ty->ToArrayType();
    assert(type);

    auto eleType = type->Derived();
    auto offset = _offset + eleType->Width() * idx;

    return parser->NewVariable(eleType, offset);
}
*/

void Identifier::TypeChecking(const Token* errTok)
{
    // TODO(wgtdkp):
}

long long Identifier::EvalInteger(const Token* errTok) {
    Error(errTok->Coord(), "identifier in constant expression");
    return 0;   // Make compiler happy
}

long long Constant::EvalInteger(const Token* errTok) {
    if (_ty->IsFloat()) {
        Error(errTok->Coord(), "expect integer, but get floating");
    }
    return _ival;
}

long long TempVar::EvalInteger(const Token* errTok) {
    Error(errTok->Coord(),
            "function call is not allowed in constant expression");
    return 0;   // Make compiler happy
}







