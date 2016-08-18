#include "evaluator.h"

#include "ast.h"
#include "token.h"


template<typename T>
void Evaluator<T>::VisitBinaryOp(BinaryOp* binary)
{
#define L   Evaluator<T>().Eval(binary->_lhs)
#define R   Evaluator<T>().Eval(binary->_rhs)
#define LL  Evaluator<long>().Eval(binary->_lhs)
#define LR  Evaluator<long>().Eval(binary->_rhs)

    if (binary->Type()->ToPointerType()) {
        auto val = Evaluator<Addr>().Eval(binary);
        if (val._label.size()) {
            Error(binary, "expect constant integer expression");
        }
        _val = static_cast<T>(val._offset);
        return;
    }

    switch (binary->_op) {
    case '+': _val = L + R; break; 
    case '-': _val = L - R; break;
    case '*': _val = L * R; break;
    case '/': {
        auto l = L, r = R;
        if (r == 0)
            Error(binary, "division by zero");
        _val = l / r;
    } break;
    case '%': {
        auto l = LL, r = LR;
        if (r == 0)
            Error(binary, "division by zero");
        _val = l % r;
    } break;
    // Bitwise operators that do not accept float
    case '|': _val = LL | LR; break;
    case '&': _val = LL & LR; break;
    case '^': _val = LL ^ LR; break;
    case Token::LEFT_OP: _val = LL << LR; break;
    case Token::RIGHT_OP: _val = LL >> LR; break;

    case '<': _val = L < R; break;
    case '>': _val = L > R; break;
    case Token::AND_OP: _val = L && R; break;
    case Token::OR_OP: _val = L || R; break;
    case Token::EQ_OP: _val = L == R; break;
    case Token::NE_OP: _val = L != R; break;
    case Token::LE_OP: _val = L <= R; break;
    case Token::GE_OP: _val = L >= R; break;
    case '=': case ',': _val = R; break;
    case ']': case '.':
        Error(binary, "expect constant expression"); 
    default: assert(false);
    }

#undef L
#undef R
#undef LL
#undef LR
}


template<typename T>
void Evaluator<T>::VisitUnaryOp(UnaryOp* unary)
{
#define VAL     Evaluator<T>().Eval(unary->_operand)
#define LVAL    Evaluator<long>().Eval(unary->_operand)

    switch (unary->_op) {
    case Token::PLUS: _val = VAL; break;
    case Token::MINUS: _val = -VAL; break;
    case '~': _val = ~LVAL; break;
    case '!': _val = !VAL; break;
    case Token::CAST:
        if (unary->Type()->IsInteger())
            _val = static_cast<long>(VAL);
        else
            _val = VAL;
        break;
    default: Error(unary, "expect constant expression");
    }

#undef VAL
}


template<typename T>
void Evaluator<T>::VisitConditionalOp(ConditionalOp* condOp)
{
    bool cond;
    auto condType = condOp->_cond->Type();
    if (condType->IsInteger()) {
        auto val = Evaluator<long>().Eval(condOp->_cond);
        cond = val != 0;
    } else if (condType->IsFloat()) {
        auto val = Evaluator<double>().Eval(condOp->_cond);
        cond  = val != 0.0;
    } else if (condType->ToPointerType()) {
        auto val = Evaluator<Addr>().Eval(condOp->_cond);
        cond = val._label.size() || val._offset;
    }

    if (cond) {
        _val = Evaluator<T>().Eval(condOp->_exprTrue);
    } else {
        _val = Evaluator<T>().Eval(condOp->_exprFalse);
    }
}


void Evaluator<Addr>::VisitBinaryOp(BinaryOp* binary)
{
    auto l = Evaluator<Addr>().Eval(binary->_lhs);
    auto r = Evaluator<Addr>().Eval(binary->_rhs);
    
    int width;
    auto pointerType = binary->_lhs->Type()->ToPointerType();
    if (pointerType) {
        width = pointerType->Derived()->Width();
    }

    switch (binary->_op) {
    case '+':
        _addr._label = l._label;
        _addr._offset = l._offset + r._offset * width;
        break;
    case '-':
        _addr._label = l._label;
        _addr._offset = l._offset + r._offset * width;
        break;
    case ']':
        _addr._label = l._label;
        _addr._offset = l._offset + r._offset * width;
        break;
    case '.': {
        _addr._label = l._label;
        auto type = binary->_lhs->Type()->ToStructUnionType();
        auto offset = type->GetMember(r._label)->Offset();
        _addr._offset = l._offset + offset;
        break;
    }
    default: assert(false);
    }
}


void Evaluator<Addr>::VisitUnaryOp(UnaryOp* unary)
{
    auto addr = Evaluator<Addr>().Eval(unary->_operand);

    switch (unary->_op) {
    case Token::CAST:
    case Token::ADDR:
    case Token::DEREF:
        _addr = addr; break;
    default: assert(false);
    }
}


void Evaluator<Addr>::VisitConditionalOp(ConditionalOp* condOp)
{
    bool cond;
    auto condType = condOp->_cond->Type();
    if (condType->IsInteger()) {
        auto val = Evaluator<long>().Eval(condOp->_cond);
        cond = val != 0;
    } else if (condType->IsFloat()) {
        auto val = Evaluator<double>().Eval(condOp->_cond);
        cond  = val != 0.0;
    } else if (condType->ToPointerType()) {
        auto val = Evaluator<Addr>().Eval(condOp->_cond);
        cond = val._label.size() || val._offset;
    }

    if (cond) {
        _addr = Evaluator<Addr>().Eval(condOp->_exprTrue);
    } else {
        _addr = Evaluator<Addr>().Eval(condOp->_exprFalse);
    }
}
