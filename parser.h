#ifndef _PARSER_H_
#define _PARSER_H_

#include "ast.h"
#include "encoding.h"
#include "error.h"
#include "mem_pool.h"
#include "scope.h"
#include "token.h"

#include <cassert>
#include <memory>
#include <stack>

class Preprocessor;

typedef std::pair<Token*, Type*> TokenTypePair;

class Parser
{
	typedef std::vector<Constant*> LiteralList;
	typedef std::vector<Object*> StaticObjectList;
	typedef std::vector<std::pair<Constant*, LabelStmt*>> CaseLabelList;
	typedef std::list<std::pair<Token*, JumpStmt*>> LabelJumpList;
	typedef std::map<std::string, LabelStmt*> LabelMap;
	
public:
	explicit Parser(const TokenSequence& ts) 
		: unit_(TranslationUnit::New()),
		  ts_(ts),
		  externalSymbols_(new Scope(nullptr, S_BLOCK)),
		  errTok_(nullptr), curScope_(new Scope(nullptr, S_FILE)),
		  curParamScope_(nullptr), curFunc_(nullptr),
		  breakDest_(nullptr), _continueDest(nullptr),
		  caseLabels_(nullptr), defaultLabel_(nullptr) {
			  ts_.SetParser(this);
		  }

	~Parser(void) {}

	Constant* ParseConstant(const Token* tok);
	Constant* ParseFloat(const Token* tok);
	Constant* ParseInteger(const Token* tok);
	Constant* ParseCharacter(const Token* tok);
	std::string ParseLiteral(Encoding& enc, const Token* tok);
	Constant* ConcatLiterals(const Token* tok);

	Expr* ParseGeneric(void);

	void Parse(void);
	void ParseTranslationUnit(void);
	FuncDef* ParseFuncDef(Identifier* ident);
	/************ Expressions ************/
	
	Expr* ParseExpr(void);

	Expr* ParsePrimaryExpr(void);

	Expr* ParsePostfixExpr(void);
	Expr* ParsePostfixExprTail(Expr* primExpr);
	Expr* ParseSubScripting(Expr* pointer);
	BinaryOp* ParseMemberRef(const Token* tok, int op, Expr* lhs);
	UnaryOp* ParsePostfixIncDec(const Token* tok, Expr* operand);
	FuncCall* ParseFuncCall(Expr* caller);

	Expr* ParseUnaryExpr(void);
	Constant* ParseSizeof(void);
	Constant* ParseAlignof(void);
	UnaryOp* ParsePrefixIncDec(const Token* tok);
	UnaryOp* ParseUnaryOp(const Token* tok, int op);
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

	/************* Declarations **************/
	CompoundStmt* ParseDecl(void);
	
	Type* ParseDeclSpec(int* storage, int* func);
	
	Type* ParseSpecQual(void);
	
	int ParseAlignas(void);
	
	Type* ParseStructUnionSpec(bool isStruct);
	
	Type* ParseEnumSpec(void);
	
	StructType* ParseStructUnionDecl(StructType* type);
	
	bool ParseBitField(StructType* structType,
			Token* tok, Type* type, bool packed);

	Type* ParseEnumerator(ArithmType* type);
	
	//declarator
	int ParseQual(void);
	
	Type* ParsePointer(Type* typePointedTo);
	
	TokenTypePair ParseDeclarator(Type* type);
	
	Type* ParseArrayFuncDeclarator(Token* ident, Type* base);
	
	int ParseArrayLength(void);
	
	bool ParseParamList(FuncType::TypeList& paramTypes);
	
	Type* ParseParamDecl(void);

	//typename
	Type* ParseAbstractDeclarator(Type* type);
	
	Identifier* ParseDirectDeclarator(Type* type,
			int storageSpec, int funcSpec);

	//initializer
	void ParseInitializer(Declaration* decl, Type* type,
			int offset, bool designated, bool forceBrace=false);
	
	void ParseArrayInitializer(Declaration* decl,
			ArrayType* type, int offset, bool designated);
			
	StructType::Iterator ParseStructDesignator(StructType* type,
			const std::string& name);

	void ParseStructInitializer(Declaration* decl,
			StructType* type, int offset, bool designated);

	bool ParseLiteralInitializer(Declaration* init,
			ArrayType* type, int offset);

	Declaration* ParseInitDeclarator(Identifier* ident);

	/************* Statements ***************/
	Stmt* ParseStmt(void);
	
	CompoundStmt* ParseCompoundStmt(FuncType* funcType=nullptr);
	
	IfStmt* ParseIfStmt(void);
	
	CompoundStmt* ParseSwitchStmt(void);
	
	CompoundStmt* ParseWhileStmt(void);
	
	CompoundStmt* ParseDoStmt(void);
	
	CompoundStmt* ParseForStmt(void);
	
	JumpStmt* ParseGotoStmt(void);
	
	JumpStmt* ParseContinueStmt(void);
	
	JumpStmt* ParseBreakStmt(void);
	
	ReturnStmt* ParseReturnStmt(void);
	
	CompoundStmt* ParseLabelStmt(const Token* label);
	
	CompoundStmt* ParseCaseStmt(void);
	
	CompoundStmt* ParseDefaultStmt(void);

	Identifier* ProcessDeclarator(Token* tok, Type* type,
			int storageSpec, int funcSpec);

	bool IsTypeName(Token* tok) const{
		if (tok->IsTypeSpecQual())
			return true;

		if (tok->IsIdentifier()) {
			auto ident = curScope_->Find(tok);
			if (ident && ident->ToTypeName())
				return true;
		}
		return false;
	}

	bool IsType(Token* tok) const{
		if (tok->IsDecl())
			return true;

		if (tok->IsIdentifier()) {
			auto ident = curScope_->Find(tok);
			return (ident && ident->ToTypeName());
		}

		return false;
	}

	void EnsureInteger(Expr* expr) {
		if (!expr->Type()->IsInteger()) {
			Error(expr, "expect integer expression");
		}
	}

	void EnterBlock(FuncType* funcType=nullptr);
	
	Scope* ExitBlock(void) {
		auto scope = curScope_;
		curScope_ = curScope_->Parent();
		return scope;
	}

	void EnterProto(void) {
		curScope_ = new Scope(curScope_, S_PROTO);
		if (curParamScope_ == nullptr)
			curParamScope_ = curScope_;
	}

	void ExitProto(void) {
		curScope_ = curScope_->Parent();
	}

	FuncDef* EnterFunc(Identifier* ident);

	void ExitFunc(void);

	LabelStmt* FindLabel(const std::string& label) {
		auto ret = curLabels_.find(label);
		if (curLabels_.end() == ret)
			return nullptr;
		return ret->second;
	}

	void AddLabel(const std::string& label, LabelStmt* labelStmt) {
		assert(nullptr == FindLabel(label));
		curLabels_[label] = labelStmt;
	}

	TranslationUnit* Unit(void) {
		return unit_;
	}

	//StaticObjectList& StaticObjects(void) {
	//    return _staticObjects;
	//}
	
	FuncDef* CurFunc(void) {
		return curFunc_;
	}

private:
	
	// The root of the AST
	TranslationUnit* unit_;
	
	TokenSequence ts_;

	// It is not the real scope,
	// It contains all external symbols(resolved and not resolved)
	Scope* externalSymbols_;
	
	//LiteralList _literals;
	//StaticObjectList _staticObjects;


	Token* errTok_;
	//std::stack<Token*> _buf;

	Scope* curScope_;
	Scope* curParamScope_;
	//Identifier* curFunc_;
	FuncDef* curFunc_;
	LabelMap curLabels_;
	LabelJumpList unresolvedJumps_;
	
	
	LabelStmt* breakDest_;
	LabelStmt* _continueDest;
	CaseLabelList* caseLabels_;
	LabelStmt* defaultLabel_;
};

#endif
