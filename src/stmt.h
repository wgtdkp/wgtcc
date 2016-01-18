#ifndef _STMT_H_
#define _STMT_H_

#include "ast.h"
#include <list>

class Stmt : public ASTNode
{

};

class EmptyStmt : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~EmptyStmt(void) {}
protected:
	EmptyStmt(void) {}
private:
};

//构建此类的目的在于，在目标代码生成的时候，能够生成相应的label
class LabelStmt : public Stmt
{
	friend class TranslationUnit;
public:
	~LabelStmt(void) {}
	int Tag(void) const { return Tag(); }
protected:
	LabelStmt(void): _tag(GenTag()) {}
private:
	static int GenTag(void) {
		static int tag = 0;
		return ++tag;
	}
	int _tag; //使用整型的tag值，而不直接用字符串
};

class IfStmt : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~IfStmt(void) {}
protected:
	IfStmt(Expr* cond, Stmt* then, Stmt* els=nullptr)
		: _cond(cond), _then(then), _else(els) {}
private:
	Expr* _cond;
	Stmt* _then;
	Stmt* _else;
};

class JumpStmt : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~JumpStmt(void) {}
	void SetLabel(LabelStmt* label) { _label = label; }
protected:
	JumpStmt(LabelStmt* label) : _label(label) {}
private:
	LabelStmt* _label;
};

class CompoundStmt : public Stmt
{
	friend class TranslationUnit;
public:
	virtual ~CompoundStmt(void) {}

protected:
	CompoundStmt(const std::list<Stmt*>& stmts)
		: _stmts(stmts) {}

private:
	std::list<Stmt*> _stmts;
};


#endif
