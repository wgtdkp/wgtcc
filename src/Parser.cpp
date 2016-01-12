#include "parser.h"
#include "error.h"

using namespace std;

void Parser::Expect(int expect, int follow1, int follow2)
{
	auto tok = Next();
	if (tok->Tag() != expect) {
		PutBack();
		//TODO: error
		Error(tok, "'%s' expected, but got '%s'", Token::Lexeme(expect), tok->Val());
		Panic(follow1, follow2);
	}
}

TranslationUnit* Parser::ParseTranslationUnit(void)
{
	auto program = TranslationUnit::NewTranslationUnit();
    for (; ;) {
		
    }
	return program;
}

Expr* Parser::ParseExpr(void)
{
	return ParseCommaExpr();
}

BinaryOp* Parser::ParseCommaExpr(void)
{
	auto commaExpr = ParseAssignExpr();
	while (Try(',')) {
		auto rhs = ParseAssignExpr();
		//the type of comma expression is 
		//the type of the last assignment expression
		commaExpr = TranslationUnit::NewBinaryOp(rhs->Ty(), ',', commaExpr, rhs);
	}
	return commaExpr;
}

BinaryOp* Parser::ParseAssignExpr(void)
{
	return nullptr;
}

Expr* Parser::ParsePrimaryExpr(void)
{
    if (Peek()->IsKeyWord()) //can never be a expression
        return nullptr;

    auto tok = Next();
    if (tok->IsEOF()) return nullptr;
    if (tok->Tag() == '(') {
        auto expr = ParseExpr();
		Expect(')');
        return expr;
    }

    if (tok->IsIdentifier()) {
        //TODO: create a expression node with symbol

    } else if (tok->IsConstant()) {
        return ParseConstant(tok);
    } else if (tok->IsString()) {
        return ParseString(tok);
    } else if (tok->Tag() == Token::GENERIC) {
        return ParseGeneric();
    } 

    //TODO: error 
    return nullptr;
}

Expr* Parser::ParseConstant(const Token* tok)
{
	assert(tok->IsConstant());
	return nullptr;
}

Expr* Parser::ParseString(const Token* tok)
{
	assert(tok->IsString());
	return nullptr;
}

Expr* Parser::ParseGeneric(void)
{
	return nullptr;
}

Expr* Parser::ParsePostfixExpr(void)
{
	auto tok = Next();
	if (tok->IsEOF()) return nullptr;

	if ('(' == tok->Tag() && IsType(Peek())) {
		//compound literals
		Error("compound literals not supported yet");
		//return ParseCompLiteral();
	}
	PutBack();
	auto primExpr = ParsePrimaryExpr();
	return ParsePostfixExprTail(primExpr);
}

//return the constructed postfix expression
Expr* Parser::ParsePostfixExprTail(Expr* lhs)
{
	for (; ;) {
		auto tag = Next()->Tag();
		switch (tag) {
		case '[': 
			lhs = ParseSubScripting(lhs);
			break;
		case '(': 
			lhs = ParseFuncCall(lhs);
			break;
		case '.':
		case Token::PTR_OP: 
			lhs = ParseMemberRef(lhs);
			break;
		case Token::INC_OP: 
		case Token::DEC_OP:
			lhs = ParsePostfixIncDec(tag, lhs);
			break;
		default: return PutBack(), lhs;
		}
	}
}

BinaryOp* Parser::ParseSubScripting(Expr* pointer)
{
	auto pointerType = pointer->Ty()->ToPointerType();
	if (nullptr == pointerType) {
		//TODO: error
		Error("an pointer expected");
	}
	auto indexExpr = ParseExpr();
	auto intType = indexExpr->Ty()->ToArithmType();
	if (nullptr == intType && intType->IsInteger()) {
		//TODO:error ensure int type
		Error("the operand of [] should be intger");
	}

	Expect(']');
	//the type of operator '[' is the type pointer points to
	return TranslationUnit::NewBinaryOp(pointerType->Derived(), '[', pointer, indexExpr);
}


BinaryOp* Parser::ParseMemberRef(Expr* objExpr)
{
	auto objType = objExpr->Ty()->ToStructUnionType();
	if (nullptr == objType) {
		Error("an struct/union expected");
	}
	Expect(Token::IDENTIFIER);
	auto memberName = Peek()->Val();
	auto member = objType->Find(memberName);
	if (nullptr == member) {
		//TODO: how to print type info ?
		Error("'%s' is not a member of '%s'", memberName, "[obj]");
	}
	return TranslationUnit::NewBinaryOp(member->Ty(), '.', objExpr, member);
}

UnaryOp* Parser::ParsePostfixIncDec(int tag, Expr* expr)
{
	EnsureModifiable(expr);
	int op = tag == Token::INC_OP ? Token::POSTFIX_INC : Token::POSTFIX_DEC;
	return TranslationUnit::NewUnaryOp(expr->Ty(), op, expr);
}

FuncCall* Parser::ParseFuncCall(Expr* func)
{
	auto funcType = func->Ty()->ToFuncType();
	if (nullptr == funcType) {
		Error("not a function type");
	}

	list<Expr*> args;
	args.push_back(ParseAssignExpr());
	while (Try(',')) {
		args.push_back(ParseAssignExpr());
		//TODO: args type checking

	}

	Expect(')');
	return TranslationUnit::NewFuncCall(funcType->Derived(), func, args);
}

Expr* Parser::ParseUnaryExpr(void)
{
	auto tok = Next();
	switch (tok->Tag()) {
	case Token::ALIGNOF: return ParseAlignofOperand();
	case Token::SIZEOF: return ParseSizeofOperand();
	case Token::INC_OP: case Token::DEC_OP: return ParsePrefixIncDec(tok->Tag());
		
	case '&': case '*': case '+': 
	case '-': case '~': case '!': 
		//return ParseUnaryOp(tok->Tag());
	default:
		return PutBack(), ParsePostfixExpr();
	}
}

Constant* Parser::ParseSizeofOperand(void)
{
	Type* type;
	auto tok = Next();
	if (tok->Tag() == '(' && IsType(Peek())) {
		type = ParseTypeName();
		Expect(')');
		
	} else {
		PutBack();
		auto unaryExpr = ParseUnaryExpr();
		type = unaryExpr->Ty();
	}

	if (nullptr != type->ToFuncType()) {
		Error("sizeof operator can't act on function");
	}
	auto intType = Type::NewArithmType(ArithmType::TULONG);
	return TranslationUnit::NewConstant(intType, type->Width());
}

Constant* Parser::ParseAlignofOperand(void)
{
	Expect('(');
	auto type = nullptr;// ParseTypeName();
	Expect(')');
	auto intType = Type::NewArithmType(ArithmType::TULONG);
	return TranslationUnit::NewConstant(intType, intType->Align());
}

UnaryOp* Parser::ParsePrefixIncDec(int tag)
{
	assert(Token::INC_OP == tag || Token::DEC_OP == tag);
	auto unaryExpr = ParseUnaryExpr();
	EnsureModifiable(unaryExpr);
	auto op = tag == Token::INC_OP ? Token::PREFIX_INC : Token::PREFIX_DEC;
	return TranslationUnit::NewUnaryOp(unaryExpr->Ty(), op, unaryExpr);
}

UnaryOp* Parser::ParseAddrOperand(void)
{
	auto castExpr = ParseCastExpr();
	PointerType* pointerType;
	if (nullptr != castExpr->Ty()->ToFuncType()) {
		pointerType = Type::NewPointerType(castExpr->Ty());
		return TranslationUnit::NewUnaryOp(pointerType, '&', castExpr);
	}

	EnsureLVal(castExpr);
	pointerType = Type::NewPointerType(castExpr->Ty());
	return TranslationUnit::NewUnaryOp(pointerType, '&', castExpr);
}


Type* Parser::ParseTypeName(void)
{
	return nullptr;
}

Expr* Parser::ParseCastExpr(void)
{
	return nullptr;
}


