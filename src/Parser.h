#ifndef _PARSER_H_
#define _PARSER_H_

#include <cassert>
#include <stack>
#include <memory>
#include "ast.h"
#include "lexer.h"
#include "env.h"
#include "error.h"

class Env;

typedef std::pair<const char*, Type*> NameTypePair;

class Parser
{
public:
    explicit Parser(Lexer* lexer) 
		: _lexer(lexer), _topEnv(nullptr),
		  _breakDest(nullptr), _continueDest(nullptr),
		  _caseLabels(nullptr), _defaultLabel(nullptr) {}

    ~Parser(void) { 
		delete _lexer; 
	}

	Expr* ParseConstant(const Token* tok);
	Expr* ParseString(const Token* tok);
	Expr* ParseGeneric(void);

	TranslationUnit* ParseTranslationUnit(void);

    /************ Expressions ************/
	
	Expr* ParseExpr(void);

	Expr* ParsePrimaryExpr(void);

	Expr* ParsePostfixExpr(void);
	Expr* ParsePostfixExprTail(Expr* primExpr);
	Expr* ParseSubScripting(Expr* pointer);
	Expr* ParseMemberRef(int tag, Expr* lhs);
	UnaryOp* ParsePostfixIncDec(int tag, Expr* expr);
	FuncCall* ParseFuncCall(Expr* caller);

	Expr* ParseUnaryExpr(void);
	Constant* ParseSizeof(void);
	Constant* ParseAlignof(void);
	UnaryOp* ParsePrefixIncDec(int tag);
	UnaryOp* ParseUnaryOp(int op);
	//UnaryOp* ParseDerefOperand(void);

	Type* ParseTypeName(void);
	Expr* ParseCastExpr(void);
	Expr* ParseMultiplicativeExpr(void);
	Expr* ParseAdditiveExpr(void);
	Expr* ParseShiftExpr(void);
	Expr* ParseRelationalExpr(void);
	Expr* ParseEqualityExpr(void);
	Expr* ParseBitiwiseAndExpr(void);
	Expr* ParseBitwiseXorExpr(void);
	Expr* ParseBitwiseOrExpr(void);
	Expr* ParseLogicalAndExpr(void);
	Expr* ParseLogicalOrExpr(void);
	Expr* ParseConditionalExpr(void);

	Expr* ParseCommaExpr(void);

	Expr* ParseAssignExpr(void);

	Constant* ParseConstantExpr(void);


	/************* Declarations **************/
	CompoundStmt* Parser::ParseDecl(void);
	Type* ParseDeclSpec(int* storage, int* func);
	Type* ParseSpecQual(void);
	int ParseAlignas(void);
	Type* ParseStructUnionSpec(bool isStruct);
	Type* ParseEnumSpec(void);
	StructUnionType* ParseStructDecl(StructUnionType* type);
	Type* ParseEnumerator(ArithmType* type);
	//declarator
	int ParseQual(void);
	Type* ParsePointer(Type* typePointedTo);
	Variable* ParseDeclaratorAndDo(Type* base, int storageSpec, int funcSpec);
	NameTypePair ParseDeclarator(Type* type);
	Type* ParseArrayFuncDeclarator(Type* base);
	int ParseArrayLength(void);
	bool ParseParamList(std::list<Type*>& params);
	Type* ParseParamDecl(void);

	//typename
	Type* ParseAbstractDeclarator(Type* type);

	//initializer
	Expr* ParseInitDeclarator(Type* type, int storageSpec, int funcSpec);
	//Expr* ParseInit

	/************* Statements ***************/
	Stmt* ParseStmt(void);
	CompoundStmt* ParseCompoundStmt(void);
	IfStmt* ParseIfStmt(void);
	CompoundStmt* ParseSwitchStmt(void);
	CompoundStmt* ParseWhileStmt(void);
	CompoundStmt* ParseDoStmt(void);
	CompoundStmt* ParseForStmt(void);
	JumpStmt* ParseGotoStmt(void);
	JumpStmt* ParseContinueStmt(void);
	JumpStmt* ParseBreakStmt(void);
	JumpStmt* ParseReturnStmt(void);
	CompoundStmt* ParseLabelStmt(const char* label);
	CompoundStmt* ParseCaseStmt(void);
	CompoundStmt* ParseDefaultStmt(void);

	/*********** Function Definition *************/
	bool IsFuncDef(void);
	FuncDef* ParseFuncDef(void);

private:
	//如果当前token符合参数，返回true,并consume一个token
	//如果与tokTag不符，则返回false，并且不consume token
	bool Try(int tokTag) {
		auto tok = Next();
		if (tok->Tag() == tokTag)
			return true;
		PutBack();
		return false;
	}

	//返回当前token，并前移
	Token* Next(void) { return _lexer->Get(); }
	void PutBack(void) { _lexer->Unget(); }

	//返回当前token，但是不前移
	Token* Peek(void) { return _lexer->Peek(); }
	const Token* Peek(void) const{ return _lexer->Peek(); }
	bool Test(int tag) const { return Peek()->Tag() == tag; }

	//记录当前token位置
	void Mark(void) { _buf.push(Peek()); }

	//回到最近一次Mark的地方
	Token* Release(void) {
		assert(!_buf.empty());
		while (Peek() != _buf.top()) {
			PutBack();
		}
		_buf.pop();
		return Peek();
	}

	bool IsTypeName(const Token* tok) const{
		if (tok->IsTypeSpecQual())
			return true;
		return (tok->IsIdentifier() 
			&& nullptr != _topEnv->FindType(tok->Val()));
	}

	bool IsType(const Token* tok) const{
		if (tok->IsDecl())
			return true;
		return (tok->IsIdentifier()
			&& nullptr != _topEnv->FindType(tok->Val()));
	}

	void Expect(int expect, int follow1 = ',', int follow2 = ';');

	void Panic(int follow1, int follow2) {
		for (const Token* tok = Next(); !tok->IsEOF(); tok = Next()) {
			if (tok->Tag() == follow1 || tok->Tag() == follow2)
				return PutBack();
		}
	}

	void EnsureModifiableExpr(Expr* expr) const {
		if (!expr->IsLVal()) {
			//TODO: error
			Error("lvalue expression expected");
		} else if (expr->Ty()->IsConst()) {
			Error("can't modifiy 'const' qualified expression");
		}
	}

	void EnsureLValExpr(Expr* expr) const {
		if (!expr->IsLVal()) {
			Error("lvalue expected");
		}
	}

	void EnsureScalarExpr(Expr* expr) const {
		if (!expr->Ty()->IsScalar())
			Error("scalar type expression expected");
	}

	void EnsureIntegerExpr(Expr* expr) const {
		if (!expr->Ty()->IsInteger())
			Error("integer expression expected");
	}

	void EnterBlock(void) {
		_topEnv = new Env(_topEnv);
	}
	void ExitBlock(void) {
		_topEnv = _topEnv->Parent();
	}

	void EnterFunc(const char* funcName);

	void ExitFunc(void);

	LabelStmt* FindLabel(const char* label) {
		auto ret = _topLabels.find(label);
		if (_topLabels.end() == ret)
			return nullptr;
		return ret->second;
	}

	void AddLabel(const char* label, LabelStmt* labelStmt) {
		assert(nullptr == FindLabel(label));
		_topLabels[label] = labelStmt;
	}

	//constant evaluator
	bool EvaluateConstantExpr(int& val, const Expr* expr);

	typedef std::vector<std::pair<int, LabelStmt*>> CaseLabelList;
	typedef std::list<std::pair<const char*, JumpStmt*>> LabelJumpList;
	typedef std::map<const char*, LabelStmt*, StrCmp> LabelMap;
private:
	
    Lexer* _lexer;
	Env* _topEnv;
	LabelMap _topLabels;
	LabelJumpList _unresolvedJumps;
	std::stack<Token*> _buf;

	LabelStmt* _breakDest;
	LabelStmt* _continueDest;
	CaseLabelList* _caseLabels;
	LabelStmt* _defaultLabel;
};

#endif
