#include "expr.h"
#include "parser.h"
#include "error.h"
#include <string>

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

Expr* Parser::ParseCommaExpr(void)
{
	auto lhs = ParseAssignExpr();
	while (Try(',')) {
		auto rhs = ParseAssignExpr();
		lhs = TranslationUnit::NewBinaryOp(',', lhs, rhs);
	}
	return lhs;
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

	if ('(' == tok->Tag() && IsTypeName(Peek())) {
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
		case '[': lhs = ParseSubScripting(lhs); break;
		case '(': lhs = ParseFuncCall(lhs); break;
		case '.': case Token::PTR_OP: lhs = ParseMemberRef(tag, lhs); break;
		case Token::INC_OP: case Token::DEC_OP: lhs = ParsePostfixIncDec(tag, lhs); break;
		default: return PutBack(), lhs;
		}
	}
}

Expr* Parser::ParseSubScripting(Expr* pointer)
{
	auto indexExpr = ParseExpr();
	Expect(']');
	return TranslationUnit::NewBinaryOp('[', pointer, indexExpr);
}


Expr* Parser::ParseMemberRef(int tag, Expr* lhs)
{
	auto memberName = Peek()->Val();
	Expect(Token::IDENTIFIER);
	return TranslationUnit::NewMemberRefOp(tag, lhs, memberName);
}

UnaryOp* Parser::ParsePostfixIncDec(int tag, Expr* operand)
{
	return TranslationUnit::NewUnaryOp(tag, operand);
}

FuncCall* Parser::ParseFuncCall(Expr* designator)
{
	//TODO: ParseArgs
	list<Expr*> args;// = ParseFuncArgs(designator);
	Expect(')');
	/*
		args.push_back(ParseAssignExpr());
	while (Try(',')) {
		args.push_back(ParseAssignExpr());
		//TODO: args type checking

	}
	*/
	
	return TranslationUnit::NewFuncCall(designator, args);
}

Expr* Parser::ParseUnaryExpr(void)
{
	auto tag = Next()->Tag();
	switch (tag) {
	case Token::ALIGNOF: return ParseAlignof();
	case Token::SIZEOF: return ParseSizeof();
	case Token::INC_OP: return ParsePrefixIncDec(Token::INC_OP);
	case Token::DEC_OP: return ParsePrefixIncDec(Token::DEC_OP);
	case '&': return ParseUnaryOp(Token::ADDR);
	case '*': return ParseUnaryOp(Token::DEREF); 
	case '+': return ParseUnaryOp(Token::PLUS);
	case '-': return ParseUnaryOp(Token::MINUS); 
	case '~': return ParseUnaryOp('~');
	case '!': return ParseUnaryOp('!');
	default: return PutBack(), ParsePostfixExpr();
	}
}

Constant* Parser::ParseSizeof(void)
{
	Type* type;
	auto tok = Next();
	if (tok->Tag() == '(' && IsTypeName(Peek())) {
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

Constant* Parser::ParseAlignof(void)
{
	Expect('(');
	auto type = ParseTypeName();
	Expect(')');
	auto intType = Type::NewArithmType(ArithmType::TULONG);
	return TranslationUnit::NewConstant(intType, intType->Align());
}

UnaryOp* Parser::ParsePrefixIncDec(int op)
{
	assert(Token::INC_OP == op || Token::DEC_OP == op);
	auto operand = ParseUnaryExpr();
	return TranslationUnit::NewUnaryOp(op, operand);
}

UnaryOp* Parser::ParseUnaryOp(int op)
{
	auto operand = ParseCastExpr();
	return TranslationUnit::NewUnaryOp(op, operand);
}

Type* Parser::ParseTypeName(void)
{
	return nullptr;
}

Expr* Parser::ParseCastExpr(void)
{
	auto tok = Next();
	if (tok->Tag() == '(' && IsTypeName(Peek())) {
		auto desType = ParseTypeName();
		Expect(')');
		auto operand = ParseCastExpr();
		return TranslationUnit::NewUnaryOp(Token::CAST, operand, desType);
	} 
	return PutBack(), ParseUnaryExpr();
}

Expr* Parser::ParseMultiplicativeExpr(void)
{
	auto lhs = ParseCastExpr();
	auto tag = Next()->Tag();
	while ('*' == tag || '/' == tag || '%' == tag) {
		auto rhs = ParseCastExpr();
		lhs = TranslationUnit::NewBinaryOp(tag, lhs, rhs);
		tag = Next()->Tag();
	}
	return PutBack(), lhs;
}

Expr* Parser::ParseAdditiveExpr(void)
{
	auto lhs = ParseMultiplicativeExpr();
	auto tag = Next()->Tag();
	while ('+' == tag || '-' == tag) {
		auto rhs = ParseMultiplicativeExpr();
		lhs = TranslationUnit::NewBinaryOp(tag, lhs, rhs);
		tag = Next()->Tag();
	}
	return PutBack(), lhs;
}

Expr* Parser::ParseShiftExpr(void)
{
	auto lhs = ParseAdditiveExpr();
	auto tag = Next()->Tag();
	while (Token::LEFT_OP == tag || Token::RIGHT_OP == tag) {
		auto rhs = ParseAdditiveExpr();
		tag = Next()->Tag();
	}
	return PutBack(), lhs;
}

Expr* Parser::ParseRelationalExpr(void)
{
	auto lhs = ParseShiftExpr();
	auto tag = Next()->Tag();
	while (Token::LE_OP == tag || Token::GE_OP == tag 
		|| '<' == tag || '>' == tag) {
		auto rhs = ParseShiftExpr();
		lhs = TranslationUnit::NewBinaryOp(tag, lhs, rhs);
		tag = Next()->Tag();
	}
	return PutBack(), lhs;
}

Expr* Parser::ParseEqualityExpr(void)
{
	auto lhs = ParseRelationalExpr();
	auto tag = Next()->Tag();
	while (Token::EQ_OP == tag || Token::NE_OP == tag) {
		auto rhs = ParseRelationalExpr();
		lhs = TranslationUnit::NewBinaryOp(tag, lhs, rhs);
		tag = Next()->Tag();
	}
	return PutBack(), lhs;
}

Expr* Parser::ParseBitiwiseAndExpr(void)
{
	auto lhs = ParseEqualityExpr();
	while (Try('&')) {
		auto rhs = ParseEqualityExpr();
		lhs = TranslationUnit::NewBinaryOp('&', lhs, rhs);
	}
	return lhs;
}

Expr* Parser::ParseBitwiseXorExpr(void)
{
	auto lhs = ParseBitiwiseAndExpr();
	while (Try('^')) {
		auto rhs = ParseBitiwiseAndExpr();
		lhs = TranslationUnit::NewBinaryOp('^', lhs, rhs);
	}
	return lhs;
}

Expr* Parser::ParseBitwiseOrExpr(void)
{
	auto lhs = ParseBitwiseXorExpr();
	while (Try('|')) {
		auto rhs = ParseBitwiseXorExpr();
		lhs = TranslationUnit::NewBinaryOp('|', lhs, rhs);
	}
	return lhs;
}

Expr* Parser::ParseLogicalAndExpr(void)
{
	auto lhs = ParseBitwiseOrExpr();
	while (Try(Token::AND_OP)) {
		auto rhs = ParseBitwiseOrExpr();
		lhs = TranslationUnit::NewBinaryOp(Token::AND_OP, lhs, rhs);
	}
	return lhs;
}

Expr* Parser::ParseLogicalOrExpr(void)
{
	auto lhs = ParseLogicalAndExpr();
	while (Try(Token::OR_OP)) {
		auto rhs = ParseLogicalAndExpr();
		lhs = TranslationUnit::NewBinaryOp(Token::OR_OP, lhs, rhs);
	}
	return lhs;
}

Expr* Parser::ParseConditionalExpr(void)
{
	auto cond = ParseLogicalOrExpr();
	if (Try('?')) {
		auto exprTrue = ParseExpr();
		Expect(':');
		auto exprFalse = ParseConditionalExpr();
		return TranslationUnit::NewConditionalOp(cond, exprTrue, exprFalse);
	}
	return cond;
}

Expr* Parser::ParseAssignExpr(void)
{
	//yes i know the lhs should be unary expression, let it handled by type checking
	Expr* lhs = ParseConditionalExpr();
	Expr* rhs;
	switch (Next()->Tag()) {
	case Token::MUL_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp('*', lhs, rhs); goto RETURN;
	case Token::DIV_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp('/', lhs, rhs); goto RETURN;
	case Token::MOD_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp('%', lhs, rhs); goto RETURN;
	case Token::ADD_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp('+', lhs, rhs); goto RETURN;
	case Token::SUB_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp('-', lhs, rhs); goto RETURN;
	case Token::LEFT_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp(Token::LEFT_OP, lhs, rhs); goto RETURN;
	case Token::RIGHT_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp(Token::RIGHT_OP, lhs, rhs); goto RETURN;
	case Token::AND_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp('&', lhs, rhs); goto RETURN;
	case Token::XOR_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp('^', lhs, rhs); goto RETURN;
	case Token::OR_ASSIGN: rhs = ParseAssignExpr(); rhs = TranslationUnit::NewBinaryOp('|', lhs, rhs); goto RETURN;
	case '=': rhs = ParseAssignExpr(); goto RETURN;
	default: return lhs;
	}
RETURN:
	return TranslationUnit::NewBinaryOp('=', lhs, rhs);
}

Constant* Parser::ParseConstantExpr(void)
{
	return nullptr;
}


/**************** Declarations ********************/

/* if there is an initializer, then return the initializer expression,
   else, return null.*/
Expr* Parser::ParseDecl(void)
{

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
	} else
		typeSpec |= T_LONG;
}

/*
param: storage: null, only type specifier and qualifier accepted;
*/
Type* Parser::ParseDeclSpec(int* storage)
{
	Type* type = nullptr;
	int align = 0;
	int storageSpec = 0;
	int funcSpec = 0;
	int qualSpec = 0;
	int typeSpec = 0;
	for (; ;) {
		auto tok = Next();
		switch (tok->Tag()) {
		//function specifier
		case Token::INLINE:		funcSpec |= F_INLINE; break;
		case Token::NORETURN:	funcSpec |= F_NORETURN; break;

		//alignment specifier
		case Token::ALIGNAS:    align = ParseAlignas(); break;

		//storage specifier
			//TODO: typedef needs more constraints
		case Token::TYPEDEF:	if (storageSpec != 0)			goto error; storageSpec |= S_TYPEDEF; break;
		case Token::EXTERN:		if (storageSpec & ~S_THREAD)	goto error; storageSpec |= S_EXTERN; break;
		case Token::STATIC:		if (storageSpec & ~S_THREAD)	goto error; storageSpec |= S_STATIC; break;
		case Token::THREAD:		if (storageSpec & ~COMP_THREAD)	goto error; storageSpec |= S_THREAD; break;
		case Token::AUTO:		if (storageSpec != 0)			goto error; storageSpec |= S_AUTO; break;
		case Token::REGISTER:	if (storageSpec != 0)			goto error; storageSpec |= S_REGISTER; break;
		
		//type qualifier
		case Token::CONST:		qualSpec |= Q_CONST; break;
		case Token::RESTRICT:	qualSpec |= Q_RESTRICT; break;
		case Token::VOLATILE:	qualSpec |= Q_VOLATILE; break;
		atomic_qual:			qualSpec |= Q_ATOMIC; break;

		//type specifier
		case Token::SIGNED:		if (typeSpec & ~COMP_SIGNED)	goto error; typeSpec |= T_SIGNED; break;
		case Token::UNSIGNED:	if (typeSpec & ~COMP_UNSIGNED)	goto error; typeSpec |= T_UNSIGNED; break;
		case Token::VOID:		if (0 != typeSpec)				goto error; typeSpec |= T_VOID; break;
		case Token::CHAR:		if (typeSpec & ~COMP_CHAR)		goto error; typeSpec |= T_CHAR; break;
		case Token::SHORT:		if (typeSpec & ~COMP_SHORT)		goto error; typeSpec |= T_SHORT; break;
		case Token::INT:		if (typeSpec & ~COMP_INT)		goto error; typeSpec |= T_INT; break;
		case Token::LONG:		if (typeSpec & ~COMP_LONG)		goto error; TypeLL(typeSpec); break;
		case Token::FLOAT:		if (typeSpec & ~T_COMPLEX)		goto error; typeSpec |= T_FLOAT; break;
		case Token::DOUBLE:		if (typeSpec & ~COMP_DOUBLE)	goto error; typeSpec |= T_DOUBLE; break;
		case Token::BOOL:		if (typeSpec != 0)				goto error; typeSpec |= T_BOOL; break;
		case Token::COMPLEX:	if (typeSpec & ~COMP_COMPLEX)	goto error; typeSpec |= T_COMPLEX; break;
		case Token::STRUCT: 
		case Token::UNION:		if (typeSpec != 0)				goto error; type = ParseStructUnionSpec(); typeSpec |= T_STRUCT_UNION; break;
		case Token::ENUM:		if (typeSpec != 0)				goto error; type = ParseEnumSpec(); typeSpec |= T_ENUM; break;
		case Token::ATOMIC:		assert(false);// if (Peek()->Tag() != '(')		goto atomic_qual; if (typeSpec != 0) goto error;
									//type = ParseAtomicSpec();  typeSpec |= T_ATOMIC; break;
		default:
			if (IsTypeName(tok)) {
				type = _topEnv->FindType(tok->Val());
				typeSpec |= T_TYPEDEF_NAME;
			} else goto end_of_loop;
		}
	}
end_of_loop:
	PutBack();
	switch (typeSpec) {
	case T_VOID: type = Type::NewVoidType(); break;
	case T_ATOMIC: case T_STRUCT_UNION: case T_ENUM: case T_TYPEDEF_NAME: break;
	default: type = ArithmType::NewArithmType(typeSpec); break;
	}

	if (nullptr == storage && 0 != funcSpec && 0 != storageSpec && 0 != align)
		Error("type specifier/qualifier only");
	*storage = storageSpec;
	if (0 != funcSpec)
		return Type::NewFuncType(type, funcSpec);	//the params is lefted unspecified
	return type;
error:
	Error("type speficier/qualifier/storage error");
}

int Parser::ParseAlignas(void)
{
	int align;
	Expect('(');
	if (IsTypeName(Peek())) {
		auto type = ParseTypeName();
		Expect(')');
		align = type->Align();
	} else {
		auto constantExpr = ParseConstantExpr();
		Expect(')');
		align = EvalIntegerExpr(constantExpr);
	}
	return align;
}

static inline string MakeStructUnionName(const char* name)
{
	/*why i use this prefix ?
	  considering this code:
	  struct A A;
	  struct A B;
	  this is allowed, we can see that the local var 'A' does not override the struct A;
	  thus, them have different name in the symbol table;
	  to make the struct type's name unique, i use characters that are not allowed for identifier;
	  that is why we see '/' and '@' in the prefix;
	  and, the typedef type would not has this prefix;
	*/
	string ret = "struct/union@";
	return ret + name;
}


/*
following declaration is allowed:
struct Foo;
union Bar;
*/
Type* Parser::ParseStructUnionSpec(void)
{
	
	string name("");
	auto tok = Next();
	if (tok->IsIdentifier()) {
		name = MakeStructUnionName(tok->Val());
		auto curScopeType = _topEnv->FindTypeInCurScope(name.c_str());
		if (Try('{')) {
			if (nullptr != curScopeType)
				Error("'%s': struct type redefinition", tok->Val());
			else goto struct_block;
		} else {
			//TODO: should add this name into current env
			auto type = _topEnv->FindType(name.c_str());
			if (nullptr != type) return type;
			type = Type::NewStructUnionType(nullptr); //incomplete struct type
			_topEnv->InsertType(name.c_str(), type);
			return type;
		}
	}
	Expect('{');
struct_block:
	auto structType = Type::NewStructUnionType(nullptr);
	if (name.size()) _topEnv->InsertType(name.c_str(), structType);
	EnterBlock();
	//auto structType = Type::NewStructUnionType(_topEnv);
	//if (name.size()) oldEnv->InsertType(name.c_str(), structType);
	while (!Try('}')) {
		auto fieldType = ParseDeclSpec(nullptr);
		//TODO: parse declarator
	}
	ExitBlock();
	return structType;
}

