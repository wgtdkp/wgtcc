#ifndef _PARSER_H_
#define _PARSER_H_

#include <cassert>
#include <stack>
#include <memory>
#include "ast.h"
#include "lexer.h"
#include "symbol.h"
#include "error.h"

class Env;

class Parser
{
public:
    explicit Parser(Lexer* lexer): _lexer(lexer) {}

    ~Parser(void) { 
        delete _lexer;
    }

	Expr* ParseConstant(const Token* tok);
	Expr* ParseString(const Token* tok);
	Expr* ParseGeneric(void);

	TranslationUnit* ParseTranslationUnit(void);

    /****** Declaration ******/

	Decl* ParseFuncDef(void);

	Expr* ParseDecl(void);

    /****** Statement ******/
	/*
    Statement* ParseStatment(void);

    LabeledStatement* ParseLabeledStatement(void);

    CompoundStatement* ParseCompoundStatement(void);

    ExpressionStatement* ParseExpressionStatement(void);

    SelectionStatement* ParseSelectionStatement(void);

    IterationStatement* ParseIterationStatement(void);

    JumpStatement* ParseJumpStatement(void);
	*/
    /****** Expression ******/
	
	Expr* ParseExpr(void);

	Expr* ParsePrimaryExpr(void);

	Expr* ParsePostfixExpr(void);
	Expr* ParsePostfixExprTail(Expr* primExpr);
	SubScriptingOp* ParseSubScripting(Expr* pointer);
	MemberRefOp* ParseMemberRef(int tag, Expr* lhs);
	UnaryOp* ParsePostfixIncDec(int tag, Expr* expr);
	FuncCall* ParseFuncCall(Expr* caller);

	Expr* ParseUnaryExpr(void);
	Constant* ParseSizeofOperand(void);
	Constant* ParseAlignofOperand(void);
	UnaryOp* ParsePrefixIncDec(int tag);
	UnaryOp* ParseAddrOperand(void);

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

    /*
    AssignmentExpression* ParseAssignmentExpression(void);

    ConditionalExpression* ParseConditionalExpression(void);

    LogicalOrExpression* ParseLogicalOrExpression(void);

    LogicalAndExpression* ParseLogicalAndExpression(void);

    InclusiveOrExpression* ParseInclusiveOrExpression(void);

    ExclusiveOrExpression* ParseExclusiveOrExpression(void);

    AndExpression* ParseAndExpression(void);

    EqualityExpression* ParseEqualityExpression(void);

    RelationalExpression* ParseRelationalExpression(void);

    ShiftPexression* ParseShiftExpression(void);

    AdditiveExpression* ParseAdditiveExpression(void);

    MultiplicativeExpression* ParseMultiplicativeExpression(void);

    CastExpression* ParseCastExpression(void);

    UnaryExpression* ParseUnaryExpression(void);

    PostfixExpression* ParsePostfixExpression(void);

    PrimaryExpression* ParsePrimaryExpression(void);
	*/

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
	Token* Next(void) {
		return _lexer->Get();
	}

	void PutBack(void) {
		_lexer->Unget();
	}

	//返回当前token，但是不前移
	Token* Peek(void) {
		auto tok = _lexer->Peek();
		return tok;
	}

	//记录当前token位置
	void Mark(void) {
		_buf.push(Peek());
	}

	//回到最近一次Mark的地方
	Token* Release(void) {
		assert(!_buf.empty());
		while (Peek() != _buf.top()) {
			PutBack();
		}
		_buf.pop();
		return Peek();
	}

	bool IsType(const Token* tok) {
		if (tok->IsTypeQual() || tok->IsTypeSpec())
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

	void EnsureModifiable(Expr* expr) const {
		if (!expr->IsLVal()) {
			//TODO: error
			Error("lvalue expression expected");
		} else if (expr->Ty()->IsConst()) {
			Error("can't modifiy 'const' qualified expression");
		}
	}

	void EnsureLVal(Expr* expr) const {
		if (!expr->IsLVal()) {
			Error("lvalue expected");
		}
	}

	void EnterBlock(void);
	void ExitBlock(void);

private:
    Lexer* _lexer;
	Env* _topEnv;
	std::stack<Token*> _buf;
};

#endif
