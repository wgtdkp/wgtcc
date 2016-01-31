#include <string>
#include "parser.h"
#include "type.h"
#include "env.h"
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

void Parser::EnterFunc(const char* funcName) {
	//TODO: 添加编译器自带的 __func__ 宏
}

void Parser::ExitFunc(void) {
	//TODO: resolve 那些待定的jump
	//TODO: 如果有jump无法resolve，也就是有未定义的label，报错；
	for (auto iter = _unresolvedJumps.begin(); iter != _unresolvedJumps.end(); iter++) {
		auto labelStmt = FindLabel(iter->first);
		if (nullptr == labelStmt)
			Error("unresolved label '%s'", iter->first);
		iter->second->SetLabel(labelStmt);
	}
	_unresolvedJumps.clear();	//清空未定的 jump 动作
	_topLabels.clear();	//清空 label map
}

TranslationUnit* Parser::ParseTranslationUnit(void)
{
	auto transUnit = TranslationUnit::NewTranslationUnit();
    for (; ;) {
		if (Peek()->IsEOF())	 break;
		if (IsFuncDef())
			transUnit->Add(ParseFuncDef());
		else
			transUnit->Add(ParseDecl());
    }
	return transUnit;
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
	auto intType = Type::NewArithmType(T_UNSIGNED | T_LONG);
	return TranslationUnit::NewConstantInteger(intType, type->Width());
}

Constant* Parser::ParseAlignof(void)
{
	Expect('(');
	auto type = ParseTypeName();
	Expect(')');
	auto intType = Type::NewArithmType(T_UNSIGNED | T_LONG);
	return TranslationUnit::NewConstantInteger(intType, intType->Align());
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
	auto type = ParseSpecQual();
	if (Try('*') || Try('(')) //abstract-declarator 的FIRST集合
		return ParseAbstractDeclarator(type);
	return type;
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
	Constant* constant;
	auto expr = ParseConditionalExpr();
	if (!expr->IsConstant())
		Error("constant expression expected");
	if (expr->Ty()->IsInteger()) {
		//TODO:
		//auto val = expr->EvaluateConstant();
		//constant = TranslationUnit::NewConstantInteger(Type::NewArithmType(T_INT), val);
	} else if (expr->Ty()->IsFloat()) {
		//TODO:
		//auto val = expr->EvaluateConstant();
		//constant = TranslationUnit::NewConstantFloat(Type::NewArithmType(T_FLOAT), val);
	} else assert(0);
	return constant;
}

/**************** Declarations ********************/

/* if there is an initializer, then return the initializer expression,
   else, return null.*/
CompoundStmt* Parser::ParseDecl(void)
{
	std::list<Stmt*> stmts;
	if (Try(Token::STATIC_ASSERT)) {
		//TODO: static_assert();
	} else {
		int storageSpec, funcSpec;
		auto type = ParseDeclSpec(&storageSpec, &funcSpec);
		//init-declarator 的 FIRST 集合：'*' identifier '('
		if (Test('*') || Test(Token::IDENTIFIER) || Test('(')) {
			do {
				auto initExpr = ParseInitDeclarator(type, storageSpec, funcSpec);
				if (nullptr != initExpr) stmts.push_back(initExpr);
			} while (Try(','));
		}
	}
	return TranslationUnit::NewCompoundStmt(stmts);
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
		case Token::UNION:		if (typeSpec != 0)				goto error; type = ParseStructUnionSpec(Token::STRUCT == tok->Tag()); 
																			typeSpec |= T_STRUCT_UNION; break;
		case Token::ENUM:		if (typeSpec != 0)				goto error; type = ParseEnumSpec(); typeSpec |= T_ENUM; break;
		case Token::ATOMIC:		assert(false);// if (Peek()->Tag() != '(')		goto atomic_qual; if (typeSpec != 0) goto error;
									//type = ParseAtomicSpec();  typeSpec |= T_ATOMIC; break;
		default:
			if (0 == typeSpec && IsTypeName(tok)) {
				type = _topEnv->FindType(tok->Val());
				typeSpec |= T_TYPEDEF_NAME;
			} else goto end_of_loop;
		}
	}
	//TODO: 语义部分
end_of_loop:
	PutBack();
	switch (typeSpec) {
	case 0: Error("no type specifier");
	case T_VOID: type = Type::NewVoidType(); break;
	case T_ATOMIC: case T_STRUCT_UNION: case T_ENUM: case T_TYPEDEF_NAME: break;
	default: type = ArithmType::NewArithmType(typeSpec); break;
	}

	if ((nullptr == storage || nullptr == func)
		&& 0 != funcSpec && 0 != storageSpec && -1 != align) {
		Error("type specifier/qualifier only");
	}
	*storage = storageSpec;
	*func = funcSpec;
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
		EnsureIntegerExpr(constantExpr);
		Expect(')');
		align = constantExpr->IVal();
		//TODO: delete constantExpr
	}
	return align;
}

static inline string MakeStructUnionName(const char* name)
{
	static string ret = "struct/union@";
	return ret + name;
}

Type* Parser::ParseEnumSpec(void)
{
	const char* enumTag = nullptr;
	auto tok = Next();
	if (tok->IsIdentifier()) {
		enumTag = tok->Val();
		if (Try('{')) {
			//定义enum类型
			auto curScopeType = _topEnv->FindTagInCurScope(enumTag);
			if (nullptr != curScopeType) {
				if (!curScopeType->IsComplete()) {
					return ParseEnumerator(curScopeType->ToArithmType());
				} else Error("'%s': enumeration redifinition");
			} else
				goto enum_decl;
		} else {
			Type* type = _topEnv->FindTag(enumTag);
			if (nullptr != type) return type;
			type = Type::NewArithmType(T_INT); 
			type->SetComplete(false); //尽管我们把 enum 当成 int 看待，但是还是认为他是不完整的
			_topEnv->InsertTag(enumTag, type);
		}
	}
	Expect('{');
enum_decl:
	auto type = Type::NewArithmType(T_INT);
	if (nullptr != enumTag)
		_topEnv->InsertTag(enumTag, type);
	return ParseEnumerator(type); //处理反大括号: '}'
}

Type* Parser::ParseEnumerator(ArithmType* type)
{
	assert(type && !type->IsComplete() && type->IsInteger());
	int val = 0;
	do {
		auto tok = Peek();
		if (!tok->IsIdentifier())
			Error("enumration constant expected");
		
		auto enumName = tok->Val();
		if (nullptr != _topEnv->FindVarInCurScope(enumName))
			Error("'%s': symbol redifinition");
		if (Try('=')) {
			auto constExpr = ParseConstantExpr();
			EnsureIntegerExpr(constExpr);
			//TODO:
			//int enumVal = EvaluateConstant(constExpr);
			//val = enumVal;
		}
		auto Constant = TranslationUnit::NewConstantInteger(Type::NewArithmType(T_INT), val++);
		_topEnv->InsertConstant(enumName, Constant);

		Try(',');
	} while (!Try('}'));
	type->SetComplete(true);
	return type;
}

/***
四种 name space：
1.label, 如 goto end; 它有函数作用域
2.struct/union/enum 的 tag
3.struct/union 的成员
4.其它的普通的变量
***/
Type* Parser::ParseStructUnionSpec(bool isStruct)
{
	const char* structUnionTag = nullptr; //
	auto tok = Next();
	if (tok->IsIdentifier()) {
		structUnionTag = tok->Val();
		if (Try('{')) {
			//看见大括号，表明现在将定义该struct/union类型
			auto curScopeType = _topEnv->FindTagInCurScope(structUnionTag);
			if (nullptr != curScopeType) {	
				/*
				  在当前scope找到了类型，但可能只是声明；注意声明与定义只能出现在同一个scope；
				  1.如果声明在定义的外层scope,那么即使在内层scope定义了完整的类型，此声明仍然是无效的；
				    因为如论如何，编译器都不会在内部scope里面去找定义，所以声明的类型仍然是不完整的；
				  2.如果声明在定义的内层scope,(也就是先定义，再在内部scope声明)，这时，不完整的声明会覆盖掉完整的定义；
				    因为编译器总是向上查找符号，不管找到的是完整的还是不完整的，都要；
				*/
				if (!curScopeType->IsComplete()) {
					//找到了此tag的前向声明，并更新其符号表，最后设置为complete type
					return ParseStructDecl(curScopeType->ToStructUnionType());
				}
				else Error("'%s': struct type redefinition", tok->Val()); //在当前作用域找到了完整的定义，并且现在正在定义同名的类型，所以报错；
			} else //我们不用关心上层scope是否定义了此tag，如果定义了，那么就直接覆盖定义
				goto struct_decl; //现在是在当前scope第一次看到name，所以现在是第一次定义，连前向声明都没有；
		} else {	
			/*
				没有大括号，表明不是定义一个struct/union;那么现在只可能是在：
				1.声明；
				2.声明的同时，定义指针(指针允许指向不完整类型) (struct Foo* p; 是合法的) 或者其他合法的类型；
				如果现在索引符号表，那么：
				1.可能找到name的完整定义，也可能只找得到不完整的声明；不管name指示的是不是完整类型，我们都只能选择name指示的类型；
				2.如果我们在符号表里面压根找不到name,那么现在是name的第一次声明，创建不完整的类型并插入符号表；
			*/
			auto type = _topEnv->FindTag(structUnionTag);
			//如果tag已经定义或声明，那么直接返回此定义或者声明
			if (nullptr != type) return type;
			//如果tag尚没有定义或者声明，那么创建此tag的声明(因为没有见到‘{’，所以不会是定义)
			type = Type::NewStructUnionType(isStruct); //创建不完整的类型
			//因为有tag，所以不是匿名的struct/union， 向当前的scope插入此tag
			_topEnv->InsertTag(structUnionTag, type);
			return type;
		}
	}
	//没见到identifier，那就必须有struct/union的定义，这叫做匿名struct/union;
	Expect('{');
struct_decl:
	//现在，如果是有tag，那它没有前向声明；如果是没有tag，那更加没有前向声明；
	//所以现在是第一次开始定义一个完整的struct/union类型
	auto type = Type::NewStructUnionType(isStruct);
	if (nullptr != structUnionTag) 
		_topEnv->InsertTag(structUnionTag, type);
	return ParseStructDecl(type); //处理反大括号: '}'
}

StructUnionType* Parser::ParseStructDecl(StructUnionType* type)
{
	//既然是定义，那输入肯定是不完整类型，不然就是重定义了
	assert(type && !type->IsComplete());
	while (!Try('}')) {
		if (Peek()->IsEOF())
			Error("premature end of input");

		//解析type specifier/qualifier, 不接受storage等
		auto fieldType = ParseSpecQual();
		//TODO: 解析declarator

	}

	//struct/union定义结束，设置其为完整类型
	type->SetComplete(true);
	return type;
}

int Parser::ParseQual(void)
{
	int qualSpec = 0;
	for (; ;) {
		switch (Next()->Tag()) {
		case Token::CONST:		qualSpec |= Q_CONST; break;
		case Token::RESTRICT:	qualSpec |= Q_RESTRICT; break;
		case Token::VOLATILE:	qualSpec |= Q_VOLATILE; break;
		case Token::ATOMIC:		qualSpec |= Q_ATOMIC; break;
		default: PutBack(); return qualSpec;
		}
	}
}

Type* Parser::ParsePointer(Type* typePointedTo)
{
	Type* retType = typePointedTo;
	while (Try('*')) {
		retType = Type::NewPointerType(typePointedTo);
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

Variable* Parser::ParseDeclaratorAndDo(Type* base, int storageSpec, int funcSpec)
{
	NameTypePair nameType = Parser::ParseDeclarator(base);
	//TODO: 检查在同一 scope 是否已经定义此变量
	//      如果 storage 是 typedef，那么应该往符号表里面插入 type
	//      定义 void 类型变量是非法的，只能是指向void类型的指针
	//      如果 funcSpec != 0, 那么现在必须是在定义函数，否则出错
	auto var = _topEnv->InsertVar(nameType.first, nameType.second);
	var->SetStorage(storageSpec);
	return var;
}

NameTypePair Parser::ParseDeclarator(Type* base)
{
	auto pointerType = ParsePointer(base);
	if (Try('(')) {
		//现在的 pointerType 并不是正确的 base type
		auto nameTypePair = ParseDeclarator(pointerType);
		Expect(')');
		auto newBase = ParseArrayFuncDeclarator(pointerType);
		//修正 base type
		auto retType = ModifyBase(nameTypePair.second, pointerType, newBase);
		return std::pair<const char*, Type*>(nameTypePair.first, retType);
	} else if (Peek()->IsIdentifier()) {
		auto retType = ParseArrayFuncDeclarator(pointerType);
		auto ident = Peek()->Val();
		return NameTypePair(ident, retType);
	}
	Error("expect identifier or '(' but get '%s'", Peek()->Val());
	return std::pair<const char*, Type*>(nullptr, nullptr); //make compiler happy
}

Type* Parser::ParseArrayFuncDeclarator(Type* base)
{
	if (Try('[')) {
		if (nullptr != base->ToFuncType())
			Error("the element of array can't be a function");
		//TODO: parse array length expression
		auto len = ParseArrayLength();
		if (0 == len)
			Error("can't declare an array of length 0");
		Expect(']');
		base = ParseArrayFuncDeclarator(base);
		return Type::NewArrayType(len, base);
	}
	else if (Try('(')) {	//function declaration
		if (nullptr != base->ToFuncType())
			Error("the return value of a function can't be a function");
		else if (nullptr != base->ToArrayType())
			Error("the return value of afunction can't be a array");

		//TODO: parse arguments
		std::list<Type*> params;
		EnterBlock();
		bool hasEllipsis = ParseParamList(params);
		ExitBlock();
		
		Expect(')');
		base = ParseArrayFuncDeclarator(base);
		return Type::NewFuncType(base, 0, hasEllipsis, params);
	}
	return base;
}

/*
return: -1, 没有指定长度；其它，长度；
*/
int Parser::ParseArrayLength(void)
{
	auto hasStatic = Try(Token::STATIC);
	auto qual = ParseQual();
	if (0 != qual)
		hasStatic = Try(Token::STATIC);
	/*
	if (!hasStatic) {
		if (Try('*'))
			return Expect(']'), -1;
		if (Try(']'))
			return -1;
		else {
			auto expr = ParseAssignExpr();
			auto len = Evaluate(expr);
			Expect(']');
			return len;
		}
	}*/
	//不支持变长数组
	if (!hasStatic && Try(']'))
		return -1;
	else {
		auto expr = ParseAssignExpr();
		//TODO:
		//auto len = EvaluateConstant(expr);
		//Expect(']');
		//return len;
	}
}


bool Parser::ParseParamList(std::list<Type*>& params)
{
	auto paramType = ParseParamDecl();
	if (nullptr != paramType->ToVoidType())
		return false;
	while (Try(',')) {
		if (Try(Token::ELLIPSIS))
			return Expect(')'), true;
		paramType = ParseParamDecl();
		params.push_back(paramType);
	}
	Expect(')');
	return false;
}

Type* Parser::ParseParamDecl(void)
{
	int storageSpec, funcSpec;
	auto type = ParseDeclSpec(&storageSpec, &funcSpec);
	if (Peek()->Tag() == ',')
		return type;
	//TODO: declarator 和 abstract declarator 都要支持
	//TODO: 区分 declarator 和 abstract declarator
	return ParseDeclaratorAndDo(type, storageSpec, funcSpec)->Ty();		//ParseDeclarator(type);
}

Type* Parser::ParseAbstractDeclarator(Type* type)
{
	auto pointerType = ParsePointer(type);
	if (nullptr != pointerType->ToPointerType() && !Try('('))
		return pointerType;
	auto ret = ParseAbstractDeclarator(pointerType);
	Expect(')');
	auto newBase = ParseArrayFuncDeclarator(pointerType);
	return ModifyBase(ret, pointerType, newBase);
}

//TODO:: 缓一缓
Expr* Parser::ParseInitDeclarator(Type* type, int storageSpec, int funcSpec)
{
	auto var = ParseDeclaratorAndDo(type, storageSpec, funcSpec);
	if (Try('=')) {
		//TODO:
		//auto rhs = ParseInitializer();
		//return TranslationUnit::NewBinaryOp('=', var, rhs);
	}
	return nullptr;
}


/************** Statements ****************/

Stmt* Parser::ParseStmt(void)
{
	auto tok = Next();
	if (tok->IsEOF())
		Error("premature end of input");
	switch (tok->Tag()) {
	case '{': return ParseCompoundStmt();
	case Token::IF: return ParseIfStmt();
	case Token::SWITCH: return ParseSwitchStmt();
	case Token::WHILE: return ParseWhileStmt();
	case Token::DO: return ParseDoStmt();
	case Token::FOR: return ParseForStmt();
	case Token::GOTO: return ParseGotoStmt();
	case Token::CONTINUE: return ParseContinueStmt();
	case Token::BREAK: return ParseBreakStmt();
	case Token::RETURN: return ParseReturnStmt();
	case Token::CASE: return ParseCaseStmt();
	case Token::DEFAULT: return ParseDefaultStmt();
	}
	if (tok->IsIdentifier() && Try(':'))
		return ParseLabelStmt(tok->Val());
	if (Try(';')) return TranslationUnit::NewEmptyStmt();
	auto expr = ParseExpr();
	return Expect(';'), expr;
}

CompoundStmt* Parser::ParseCompoundStmt(void)
{
	EnterBlock();
	std::list<Stmt*> stmts;
	while (!Try('}')) {
		if (Peek()->IsEOF())
			Error("premature end of input");
		if (IsType(Peek()))
			stmts.push_back(ParseDecl());
		else
			stmts.push_back(ParseStmt());
	}
	ExitBlock();
	return TranslationUnit::NewCompoundStmt(stmts);
}

IfStmt* Parser::ParseIfStmt(void)
{
	Expect('(');
	auto cond = ParseExpr();
	EnsureScalarExpr(cond);
	Expect(')');

	auto then = ParseStmt();
	Stmt* els = nullptr;
	if (Try(Token::ELSE))
		els = ParseStmt();
	return TranslationUnit::NewIfStmt(cond, then, els);
}

/*
for 循环结构：
	for (declaration; expression1; expression2) statement

展开后的结构：
		declaration
cond:	if (expression1) then empty
		else goto end
		statement
step:	expression2
		goto cond
next:
*/

#define ENTER_LOOP_BODY(breakl, continuel) \
	LabelStmt* breako = _breakDest; \
	LabelStmt* continueo = _continueDest; \
	LabelStmt* _breakDest = breakl; \
	LabelStmt* _continueDest = continuel; 

#define EXIT_LOOP_BODY() \
	_breakDest = breako; \
	_continueDest = continueo;

CompoundStmt* Parser::ParseForStmt(void)
{
	EnterBlock();
	Expect('(');
	std::list<Stmt*> stmts;
	if (IsType(Peek()))
		stmts.push_back(ParseDecl());
	else if (!Try(';')) {
		stmts.push_back(ParseExpr());
		Expect(';');
	}

	Expr* condExpr = nullptr;
	if (!Try(';')) {
		condExpr = ParseExpr();
	}

	Expr* stepExpr = nullptr;
	if (!Try(')')) {
		stepExpr = ParseExpr();
		Expect(')');
	}

	auto condLabel = TranslationUnit::NewLabelStmt();
	auto stepLabel = TranslationUnit::NewLabelStmt();
	auto endLabel = TranslationUnit::NewLabelStmt();
	stmts.push_back(condLabel);
	if (nullptr != condExpr) {
		auto gotoEndStmt = TranslationUnit::NewJumpStmt(endLabel);
		auto ifStmt = TranslationUnit::NewIfStmt(condExpr, nullptr, gotoEndStmt);
		stmts.push_back(ifStmt);
	}

	//我们需要给break和continue语句提供相应的标号，不然不知往哪里跳
	ENTER_LOOP_BODY(endLabel, condLabel);
	auto bodyStmt = ParseStmt();
	//因为for的嵌套结构，在这里需要回复break和continue的目标标号
	EXIT_LOOP_BODY()
	
	stmts.push_back(bodyStmt);
	stmts.push_back(stepLabel);
	stmts.push_back(stepExpr);
	stmts.push_back(TranslationUnit::NewJumpStmt(condLabel));
	stmts.push_back(endLabel);

	ExitBlock();
	return TranslationUnit::NewCompoundStmt(stmts);
}

/*
while 循环结构：
while (expression) statement

展开后的结构：
cond:	if (expression1) then empty
		else goto end
		statement
		goto cond
end:
*/
CompoundStmt* Parser::ParseWhileStmt(void)
{
	std::list<Stmt*> stmts;
	Expect('(');
	auto condExpr = ParseExpr();
	//TODO: ensure scalar type
	Expect(')');

	auto condLabel = TranslationUnit::NewLabelStmt();
	auto endLabel = TranslationUnit::NewLabelStmt();
	auto gotoEndStmt = TranslationUnit::NewJumpStmt(endLabel);
	auto ifStmt = TranslationUnit::NewIfStmt(condExpr, nullptr, gotoEndStmt);
	stmts.push_back(condLabel);
	stmts.push_back(ifStmt);
	
	ENTER_LOOP_BODY(endLabel, condLabel)
	auto bodyStmt = ParseStmt();
	EXIT_LOOP_BODY()
	
	stmts.push_back(bodyStmt);
	stmts.push_back(TranslationUnit::NewJumpStmt(condLabel));
	stmts.push_back(endLabel);
	return TranslationUnit::NewCompoundStmt(stmts);
}

/*
do-while 循环结构：
do statement while (expression)

展开后的结构：
begin:	statement
cond:	if (expression) then goto begin
		else goto end
end:
*/

CompoundStmt* Parser::ParseDoStmt(void)
{
	auto beginLabel = TranslationUnit::NewLabelStmt();
	auto condLabel = TranslationUnit::NewLabelStmt();
	auto endLabel = TranslationUnit::NewLabelStmt();
	ENTER_LOOP_BODY(endLabel, beginLabel)
	auto bodyStmt = ParseStmt();
	EXIT_LOOP_BODY()

	Expect(Token::WHILE);
	Expect('(');
	auto condExpr = ParseExpr();
	Expect(')');

	auto gotoBeginStmt = TranslationUnit::NewJumpStmt(beginLabel);
	auto gotoEndStmt = TranslationUnit::NewJumpStmt(endLabel);
	auto ifStmt = TranslationUnit::NewIfStmt(condExpr, gotoBeginStmt, gotoEndStmt);

	std::list<Stmt*> stmts;
	stmts.push_back(beginLabel);
	stmts.push_back(bodyStmt);
	stmts.push_back(condLabel);
	stmts.push_back(ifStmt);
	stmts.push_back(endLabel);
	return TranslationUnit::NewCompoundStmt(stmts);
}

#define ENTER_SWITCH_BODY(caseLabels) \
	{ \
		CaseLabelList* caseLabelso = _caseLabels; \
		LabelStmt* defaultLabelo = _defaultLabel; \
		_caseLabels = &caseLabels; 

#define EXIT_SWITCH_BODY() \
		_caseLabels = caseLabelso; \
		_defaultLabel = defaultLabelo; \
	} 
	

CompoundStmt* Parser::ParseSwitchStmt(void)
{
	std::list<Stmt*> stmts;
	Expect('(');
	auto expr = ParseExpr();
	//TODO: ensure integer type
	Expect(')');

	auto labelTest = TranslationUnit::NewLabelStmt();
	auto labelEnd = TranslationUnit::NewLabelStmt();
	auto t = TranslationUnit::NewTempVar(expr->Ty());
	auto assign = TranslationUnit::NewBinaryOp('=', t, expr);
	stmts.push_back(assign);
	stmts.push_back(TranslationUnit::NewJumpStmt(labelTest));

	CaseLabelList caseLabels;
	ENTER_SWITCH_BODY(caseLabels)
	auto bodyStmt = ParseStmt();
	stmts.push_back(labelTest);
	for (auto iter = _caseLabels->begin(); iter != _caseLabels->end(); iter++) {
		auto rhs = TranslationUnit::NewConstantInteger(Type::NewArithmType(T_INT), iter->first);
		auto cond = TranslationUnit::NewBinaryOp(Token::EQ_OP, t, rhs);
		auto then = TranslationUnit::NewJumpStmt(iter->second);
		auto ifStmt = TranslationUnit::NewIfStmt(cond, then, nullptr);
		stmts.push_back(ifStmt);
	}
	stmts.push_back(TranslationUnit::NewJumpStmt(_defaultLabel));
	EXIT_SWITCH_BODY()

	stmts.push_back(labelEnd);
	return TranslationUnit::NewCompoundStmt(stmts);
}

CompoundStmt* Parser::ParseCaseStmt(void)
{
	//TODO: constant epxr 整型
	auto expr = ParseExpr();
	Expect(':');
	int val = EvaluateConstantExpr(expr);
	auto labelStmt = TranslationUnit::NewLabelStmt();
	_caseLabels->push_back(std::make_pair(val, labelStmt));
	std::list<Stmt*> stmts;
	stmts.push_back(labelStmt);
	stmts.push_back(ParseStmt());
	return TranslationUnit::NewCompoundStmt(stmts);
}

CompoundStmt* Parser::ParseDefaultStmt(void)
{
	Expect(':');
	auto labelStmt = TranslationUnit::NewLabelStmt();
	_defaultLabel = labelStmt;
	std::list<Stmt*> stmts;
	stmts.push_back(labelStmt);
	stmts.push_back(ParseStmt());
	return TranslationUnit::NewCompoundStmt(stmts);
}

JumpStmt* Parser::ParseContinueStmt(void)
{
	Expect(';');
	if (nullptr == _continueDest)
		Error("'continue' only in loop is allowed");
	return TranslationUnit::NewJumpStmt(_continueDest);
}

JumpStmt* Parser::ParseBreakStmt(void)
{
	Expect(';');
	if (nullptr == _breakDest)
		Error("'break' only in switch/loop is allowed");
	return TranslationUnit::NewJumpStmt(_breakDest);
}

JumpStmt* Parser::ParseGotoStmt(void)
{
	Expect(Token::IDENTIFIER);
	const char* label = Peek()->Val();
	Expect(';');

	auto labelStmt = FindLabel(label);
	if (nullptr != labelStmt)
		return TranslationUnit::NewJumpStmt(labelStmt);
	auto unresolvedJump = TranslationUnit::NewJumpStmt(nullptr);;
	_unresolvedJumps.push_back(std::make_pair(label, unresolvedJump));
	return unresolvedJump;
}

CompoundStmt* Parser::ParseLabelStmt(const char* label)
{
	auto stmt = ParseStmt();
	if (nullptr != FindLabel(label))
		Error("'%s': label redefinition");
	auto labelStmt = TranslationUnit::NewLabelStmt();
	AddLabel(label, labelStmt);
	std::list<Stmt*> stmts;
	stmts.push_back(labelStmt);
	stmts.push_back(stmt);
	return TranslationUnit::NewCompoundStmt(stmts);
}

/*
function-definition:
	declaration-specifiers declarator declaration-list? compound-statement
*/

bool Parser::IsFuncDef(void)
{
	if (Test(Token::STATIC_ASSERT))	//declaration
		return false;

	Mark();
	int storageSpec, funcSpec;
	auto type = ParseDeclSpec(&storageSpec, &funcSpec);
	auto nameType = ParseDeclarator(type);
	Release();
	return !(Test(',') || Test('=') || Test(';'));
}

FuncDef* Parser::ParseFuncDef(void)
{
	int storageSpec, funcSpec;
	auto type = ParseDeclSpec(&storageSpec, &funcSpec);
	auto funcType = ParseDeclaratorAndDo(type, storageSpec, funcSpec)->Ty();
	auto stmt = ParseCompoundStmt();
	return TranslationUnit::NewFuncDef(funcType->ToFuncType(), stmt);
}

bool Parser::EvaluateConstantExpr(int& val, const Expr* expr)
{

}

