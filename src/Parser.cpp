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
	return nullptr;
}


/**************** Declarations ********************/

/* if there is an initializer, then return the initializer expression,
   else, return null.*/
void Parser::ParseDecl(std::list<Expr*>& initializers)
{
	if (Try(Token::STATIC_ASSERT)) {

	} else {
		int storageSpec, funcSpec;
		auto type = ParseDeclSpec(&storageSpec, &funcSpec);
		//init-declarator 的 FIRST 集合：'*' identifier '('
		if (Test('*') || Test(Token::IDENTIFIER) || Test('(')) {
			do {
				auto initExpr = ParseInitDeclarator(type, storageSpec, funcSpec);
				if (nullptr != initExpr) initializers.push_back(initExpr);
			} while (Try(','));
		}
	}
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
		Expect(')');
		align = EvalIntegerExpr(constantExpr);
	}
	return align;
}

static inline string MakeStructUnionName(const char* name)
{
	static string ret = "struct/union@";
	return ret + name;
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
					return ParseStructDecl(curScopeType);
				}
				else Error("'%s': struct type redefinition", tok->Val()); //在当前作用域找到了完整的定义，并且现在正在定义同名的类型，所以报错；
			} 
			else //我们不用关心上层scope是否定义了此tag，如果定义了，那么就直接覆盖定义
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
			auto type = _topEnv->FindStructUnionType(structUnionTag);
			//如果tag已经定义或声明，那么直接返回此定义或者声明
			if (nullptr != type) return type;
			//如果tag尚没有定义或者声明，那么创建此tag的声明(因为没有见到‘{’，所以不会是定义)
			type = Type::NewStructUnionType(isStruct); //创建不完整的类型
			//因为有tag，所以不是匿名的struct/union， 向当前的scope插入此tag
			_topEnv->InsertStructUnionType(structUnionTag, type);
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
		_topEnv->InsertType(structUnionTag, type);
	return ParseStructDecl(type);
}

StructUnionType* Parser::ParseStructDecl(StructUnionType* type)
{
	//既然是定义，那输入肯定是不完整类型，不然就是重定义了
	assert(!type->IsComplete());
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
		auto len = Evaluate(expr);
		Expect(']');
		return len;
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
		auto rhs = ParseInitializer();
		return TranslationUnit::NewBinaryOp('=', var, rhs);
	}
	return nullptr;
}


/************** Statements ****************/

