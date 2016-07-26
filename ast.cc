#include "ast.h"

#include "error.h"
#include "parser.h"
#include "token.h"
#include "visitor.h"


/*
 * Accept
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
#define L   _lhs->EvalInteger(errTok)
#define R   _rhs->EvalInteger(errTok)

    if (!_ty->IsInteger()) {
        Error(errTok, "expect constant integer expression");
    }

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
                Error(errTok, "division by zero");
            return _op == '%' ? (l % r): (l / r);
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
    case Token::EQ_OP:
        return L == R;
    case Token::NE_OP:
        return L != R;
    case Token::LE_OP:
        return L <= R;
    case Token::GE_OP:
        return L >= R; 
    case '=':  
    case ',':
        return R;
    case '[':
    case '.':
    case Token::PTR_OP:
        Error(errTok, "expect constant expression"); 
    default:
        assert(0);
    }
}

ArithmType* BinaryOp::Promote(Parser* parser, const Token* errTok)
{
    // Both lhs and rhs are ensured to be have arithmetic type
    auto lhsType = _lhs->Ty()->ToArithmType();
    auto rhsType = _rhs->Ty()->ToArithmType();
    
    auto type = MaxType(lhsType, rhsType);
    if (lhsType != type) {// Pointer comparation is enough!
        _lhs = parser->NewUnaryOp(errTok, Token::CAST, _lhs, type);
    }
    if (rhsType != type) {
        _rhs = parser->NewUnaryOp(errTok, Token::CAST, _rhs, type);
    }
    
    return type;
}

/*
 * Unary Operators
 */
bool UnaryOp::IsLVal(void) const {
    // only deref('*') op is lvalue;
    // so it's only deref will override this func
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
        Error(errTok, "expect constant expression");
    }

    return 0;   // Make compiler happy
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

ArithmType* ConditionalOp::Promote(Parser* parser, const Token* errTok)
{
    auto lhsType = _exprTrue->Ty()->ToArithmType();
    auto rhsType = _exprFalse->Ty()->ToArithmType();
    
    auto type = MaxType(lhsType, rhsType);
    if (lhsType != type) {// Pointer comparation is enough!
        _exprTrue = parser->NewUnaryOp(errTok, Token::CAST, _exprTrue, type);
    }
    if (rhsType != type) {
        _exprFalse = parser->NewUnaryOp(errTok, Token::CAST, _exprFalse, type);
    }
    
    return type;
}

long long FuncCall::EvalInteger(const Token* errTok) {
    Error(errTok, "function call in constant expression");
    return 0;   // Make compiler happy
}

long long Identifier::EvalInteger(const Token* errTok) {
    Error(errTok, "identifier in constant expression");
    return 0;   // Make compiler happy
}

const std::string Identifier::Name(void) {
    return _tok->Str();
}

long long Constant::EvalInteger(const Token* errTok) {
    if (_ty->IsFloat()) {
        Error(errTok, "expect integer, but get floating");
    }
    return _ival;
}

long long TempVar::EvalInteger(const Token* errTok) {
    assert(false);
    Error(errTok, "function call in constant expression");
    return 0;   // Make compiler happy
}


