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

    //if (!_type->IsInteger()) {
    //    Error(errTok, "expect constant integer expression");
    //}

    //auto pointerType = binary->Type()->ToPointerType();
    //unsigned long width = pointerType ? pointerType->Derived()->Width(): 0;
    //bool res = true;
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
#define VAL Evaluator<T>().Eval(unary->_operand)

    switch (unary->_op) {
    case Token::PLUS: _val = VAL; break;
    case Token::MINUS: _val = -VAL; break;
    case '~': _val = ~VAL; break;
    case '!': _val = !VAL; break;
    case Token::CAST: _val = VAL; break;
    default: Error(unary, "expect constant expression");
    }

#undef VAL
}


template<typename T>
void Evaluator<T>::VisitConditionalOp(ConditionalOp* condOp)
{
    auto cond = Evaluator<T>().Eval(condOp->_cond);
    if (cond) {
        _val = Evaluator<T>().Eval(condOp->_exprTrue);
    } else {
        _val = Evaluator<T>().Eval(condOp->_exprFalse);
    }
}


void AddrEvaluator::VisitBinaryOp(BinaryOp* binary)
{
    auto l = AddrEvaluator().Eval(binary->_lhs);
    auto r = AddrEvaluator().Eval(binary->_rhs);
    
    int width;
    auto pointerType = binary->_lhs->Type()->ToPointerType();
    if (pointerType) {
        width = pointerType->Derived()->Width();
    }

    switch (binary->_op) {
    case '+':
        _addr._label = l._label;
        _addr._offset += r._offset * width;
        break;
    case '-':
        _addr._label = l._label;
        _addr._offset -= r._offset * width;
        break;
    case ']':
        _addr._label = l._label;
        _addr._offset += r._offset * width;
        break;
    case '.': {
        _addr._label = l._label;
        auto type = binary->_lhs->Type()->ToStructUnionType();
        auto offset = type->GetMember(r._label)->Offset();
        _addr._offset += offset;
        break;
    }
    default: assert(false);
    }
}


void AddrEvaluator::VisitUnaryOp(UnaryOp* unary)
{
    auto addr = AddrEvaluator().Eval(unary->_operand);

    switch (unary->_op) {
    case Token::CAST:
    case Token::ADDR:
    case Token::DEREF:
        _addr = addr; break;
    default: assert(false);
    }
}


void AddrEvaluator::VisitConditionalOp(ConditionalOp* condOp)
{
    auto cond = Evaluator<long>().Eval(condOp->_cond);
    if (cond) {
        _addr = AddrEvaluator().Eval(condOp->_exprTrue);
    } else {
        _addr = AddrEvaluator().Eval(condOp->_exprFalse);
    }
}
