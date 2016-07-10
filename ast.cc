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


long long BinaryOp::EvalInteger(const Coordinate& coord)
{
    // TypeChecking should make sure of this constrains
    assert(_ty->IsInteger());

#define L   _lhs->EvalInteger(coord)
#define R   _rhs->EvalInteger(coord)

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

void BinaryOp::TypeChecking(const Coordinate& coord)
{
    switch (_op) {
    case '[':
        return SubScriptingOpTypeChecking(coord);

    case '*':
    case '/':
    case '%':
        return MultiOpTypeChecking(coord);

    case '+':
    case '-':
        return AdditiveOpTypeChecking(coord);

    case Token::LEFT_OP:
    case Token::RIGHT_OP:
        return ShiftOpTypeChecking(coord);

    case '<':
    case '>':
    case Token::LE_OP:
    case Token::GE_OP:
        return RelationalOpTypeChecking(coord);

    case Token::EQ_OP:
    case Token::NE_OP:
        return EqualityOpTypeChecking(coord);

    case '&':
    case '^':
    case '|':
        return BitwiseOpTypeChecking(coord);

    case Token::AND_OP:
    case Token::OR_OP:
        return LogicalOpTypeChecking(coord);

    case '=':
        return AssignOpTypeChecking(coord);

    default:
        assert(0);
    }
}

void BinaryOp::SubScriptingOpTypeChecking(const Coordinate& coord)
{
    auto lhsType = _lhs->Ty()->ToPointerType();
    if (nullptr == lhsType)
        Error(coord, "an pointer expected");
    if (!_rhs->Ty()->IsInteger())
        Error(coord, "the operand of [] should be intger");

    // The type of [] operator is the derived type
    _ty = lhsType->Derived();
}

void BinaryOp::MemberRefOpTypeChecking(
        const Coordinate& coord, const std::string& rhsName)
{
    StructUnionType* structUnionType;
    if (_op == Token::PTR_OP) {
        auto pointer = _lhs->Ty()->ToPointerType();
        if (pointer == nullptr) {
            Error(coord, "pointer expected for operator '->'");
        } else {
            structUnionType = pointer->Derived()->ToStructUnionType();
            if (structUnionType == nullptr)
                Error(coord, "pointer to struct/union expected");
        }
    } else {
        structUnionType = _lhs->Ty()->ToStructUnionType();
        if (structUnionType == nullptr)
            Error(coord, "an struct/union expected");
    }

    if (structUnionType == nullptr)
        return; // The _rhs is lefted nullptr

    _rhs = structUnionType->GetMember(rhsName);
    if (_rhs == nullptr) {
        Error(coord, "'%s' is not a member of '%s'", rhsName, "[obj]");
    }

    _ty = _rhs->Ty();
}

void BinaryOp::MultiOpTypeChecking(const Coordinate& coord)
{
    auto lhsType = _lhs->Ty()->ToArithmType();
    auto rhsType = _rhs->Ty()->ToArithmType();

    if (nullptr == lhsType || nullptr == rhsType)
        Error(coord, "operands should have arithmetic type");
    if ('%' == _op && !(_lhs->Ty()->IsInteger() && _rhs->Ty()->IsInteger()))
        Error(coord, "operands of '%%' should be integers");

    //TODO: type promotion
    _ty = _lhs->Ty();
}

void BinaryOp::AdditiveOpTypeChecking(const Coordinate& coord)
{
    //auto lhsType = _lhs->Ty()->ToArithmType();
    //auto rhsType = _rhs->Ty()->ToArithmType();


    //TODO: type promotion

    _ty = _lhs->Ty();
}

void BinaryOp::ShiftOpTypeChecking(const Coordinate& coord)
{
    //TODO: type checking

    _ty = _lhs->Ty();
}

void BinaryOp::RelationalOpTypeChecking(const Coordinate& coord)
{
    //TODO: type checking

    _ty = Type::NewArithmType(T_BOOL);
}

void BinaryOp::EqualityOpTypeChecking(const Coordinate& coord)
{
    //TODO: type checking

    _ty = Type::NewArithmType(T_BOOL);
}

void BinaryOp::BitwiseOpTypeChecking(const Coordinate& coord)
{
    if (_lhs->Ty()->IsInteger() || _rhs->Ty()->IsInteger())
        Error(coord, "operands of '&' should be integer");
    
    //TODO: type promotion
    _ty = Type::NewArithmType(T_INT);
}

void BinaryOp::LogicalOpTypeChecking(const Coordinate& coord)
{
    //TODO: type checking
    if (!_lhs->Ty()->IsScalar() || !_rhs->Ty()->IsScalar())
        Error(coord, "the operand should be arithmetic type or pointer");
    
    _ty = Type::NewArithmType(T_BOOL);
}

void BinaryOp::AssignOpTypeChecking(const Coordinate& coord)
{
    //TODO: type checking
    if (!_lhs->IsLVal()) {
        //TODO: error
        Error(coord, "lvalue expression expected");
    } else if (_lhs->Ty()->IsConst()) {
        Error(coord, "can't modifiy 'const' qualified expression");
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

long long UnaryOp::EvalInteger(const Coordinate& coord)
{
#define VAL _operand->EvalInteger(coord)

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
        Error(coord, "expect constant integer");
    }

    return 0;   // Make compiler happy
}

void UnaryOp::TypeChecking(const Coordinate& coord)
{
    switch (_op) {
    case Token::POSTFIX_INC:
    case Token::POSTFIX_DEC:
    case Token::PREFIX_INC:
    case Token::PREFIX_DEC:
        return IncDecOpTypeChecking(coord);

    case Token::ADDR:
        return AddrOpTypeChecking(coord);

    case Token::DEREF:
        return DerefOpTypeChecking(coord);

    case Token::PLUS:
    case Token::MINUS:
    case '~':
    case '!':
        return UnaryArithmOpTypeChecking(coord);

    case Token::CAST:
        return CastOpTypeChecking(coord);

    default:
        assert(false);
    }
}

void UnaryOp::IncDecOpTypeChecking(const Coordinate& coord)
{
    if (!_operand->IsLVal()) {
        //TODO: error
        Error(coord, "lvalue expression expected");
    } else if (_operand->Ty()->IsConst()) {
        Error(coord, "can't modifiy 'const' qualified expression");
    }

    _ty = _operand->Ty();
}

void UnaryOp::AddrOpTypeChecking(const Coordinate& coord)
{
    FuncType* funcType = _operand->Ty()->ToFuncType();
    if (funcType == nullptr && !_operand->IsLVal()) {
        Error(coord, "expression must be an lvalue or function designator");
    }
    
    _ty = Type::NewPointerType(_operand->Ty());
}

void UnaryOp::DerefOpTypeChecking(const Coordinate& coord)
{
    auto pointer = _operand->Ty()->ToPointerType();
    if (nullptr == pointer) {
        Error(coord, "pointer expected for deref operator '*'");
    }

    _ty = pointer->Derived();
}

void UnaryOp::UnaryArithmOpTypeChecking(const Coordinate& coord)
{
    if (Token::PLUS == _op || Token::MINUS == _op) {
        if (!_operand->Ty()->IsArithm())
            Error(coord, "Arithmetic type expected");
    } else if ('~' == _op) {
        if (!_operand->Ty()->IsInteger())
            Error(coord, "integer expected for operator '~'");
    } else {//'!'
        if (!_operand->Ty()->IsScalar())
            Error(coord, "arithmetic type or pointer expected for operator '!'");
    }

    _ty = _operand->Ty();
}

void UnaryOp::CastOpTypeChecking(const Coordinate& coord)
{
    //the _ty has been initiated to desType
    if (!_ty->IsScalar()) {
        Error(coord, "the cast type should be arithemetic type or pointer");
    }

    if (_ty->IsFloat() && nullptr != _operand->Ty()->ToPointerType()) {
        Error(coord, "can't cast a pointer to floating");
    } else if (nullptr != _ty->ToPointerType() && _operand->Ty()->IsFloat()) {
        Error(coord, "can't cast a floating to pointer");
    }
}

/*
 * Conditional Operator
 */

long long ConditionalOp::EvalInteger(const Coordinate& coord)
{
    int cond = _cond->EvalInteger(coord);
    if (cond) {
        return _exprTrue->EvalInteger(coord);
    } else {
        return _exprFalse->EvalInteger(coord);
    }
}

void ConditionalOp::TypeChecking(const Coordinate& coord)
{

    //TODO: type checking

    //TODO: type evaluation
}


/*
 * Function Call
 */

void FuncCall::TypeChecking(const Coordinate& coord)
{
    auto funcType = _designator->Ty()->ToFuncType();
    if (nullptr == funcType)
        Error(coord, "not a function type");
    
    _ty = funcType->Derived();

    //TODO: check if args and params are compatible type

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

void Identifier::TypeChecking(const Coordinate& coord)
{
    // TODO(wgtdkp):
}
