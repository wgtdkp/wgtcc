#ifndef _WGTCC_AST_H_
#define _WGTCC_AST_H_

#include "mem_pool.h"
#include "token.h"
#include "type.h"

#include <list>
#include <memory>


class ASTNode;
class Visitor;

//Expression
class Expr;
class BinaryOp;
class UnaryOp;
class ConditionalOp;
class FuncCall;
class Variable;
class Constant;
class TempVar;

//statement
class Stmt;
class IfStmt;
class JumpStmt;
class LabelStmt;
class EmptyStmt;
class CompoundStmt;

class FuncDef;

class TranslationUnit;

/************ AST Node *************/

class ASTNode
{
    friend class TranslationUnit;

public:
    virtual ~ASTNode(void) {}
    
    virtual void Accept(Visitor* v) = 0;

protected:
    explicit ASTNode(MemPool* pool): _pool(pool) {}

private:
    //ASTNode(void);  //禁止直接创建Node

    //Coordinate _coord;
    MemPool* _pool;
};

typedef ASTNode ExtDecl;


/*********** Statement *************/

class Stmt : public ASTNode
{
public:
    virtual ~Stmt(void){}

protected:
    explicit Stmt(MemPool* pool): ASTNode(pool) {}
};


class EmptyStmt : public Stmt
{
    friend class TranslationUnit;

public:
    virtual ~EmptyStmt(void) {}
    
    virtual void Accept(Visitor* v);

protected:
    explicit EmptyStmt(MemPool* pool): Stmt(pool) {}

private:
};


// 构建此类的目的在于，在目标代码生成的时候，能够生成相应的label
class LabelStmt : public Stmt
{
    friend class TranslationUnit;

public:
    ~LabelStmt(void) {}
    
    virtual void Accept(Visitor* v);
    
    int Tag(void) const { return Tag(); }

protected:
    explicit LabelStmt(MemPool* pool)
            : Stmt(pool), _tag(GenTag()) {}

private:
    static int GenTag(void) {
        static int tag = 0;
        return ++tag;
    }
    
    int _tag; // 使用整型的tag值，而不直接用字符串
};


class IfStmt : public Stmt
{
    friend class TranslationUnit;

public:
    virtual ~IfStmt(void) {}
    virtual void Accept(Visitor* v);

protected:
    IfStmt(MemPool* pool, Expr* cond, Stmt* then, Stmt* els = nullptr)
        : Stmt(pool), _cond(cond), _then(then), _else(els) {}

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
    
    virtual void Accept(Visitor* v);
    
    void SetLabel(LabelStmt* label) { _label = label; }

protected:
    JumpStmt(MemPool* pool, LabelStmt* label)
            : Stmt(pool), _label(label) {}

private:
    LabelStmt* _label;
};

class ReturnStmt: public Stmt
{
    friend class TranslationUnit;

public:
    virtual ~ReturnStmt(void) {}
    
    virtual void Accept(Visitor* v);
    
protected:
    ReturnStmt(MemPool* pool, Expr* expr)
            : Stmt(pool), _expr(expr) {}

private:
    Expr* _expr;
};

class CompoundStmt : public Stmt
{
    friend class TranslationUnit;

public:
    virtual ~CompoundStmt(void) {}
    
    virtual void Accept(Visitor* v);

protected:
    CompoundStmt(MemPool* pool, const std::list<Stmt*>& stmts)
        : Stmt(pool), _stmts(stmts) {}

private:
    std::list<Stmt*> _stmts;
};


/********** Expr ************/
/*
 * Expr
 *      BinaryOp
 *      UnaryOp
 *      ConditionalOp
 *      FuncCall
 *      Variable
 *          Constant
 *      TempVar
 */

class Expr : public Stmt
{
    friend class TranslationUnit;

public:
    virtual ~Expr(void) {}
    Type* Ty(void) { return _ty; }
    const Type* Ty(void) const {
        assert(nullptr != _ty);
        return _ty;
    }

    virtual bool IsLVal(void) const = 0;

    //virtual bool EvaluateConstant(Constant* cons) = 0;

    virtual Constant* ToConstant(void) {
        return nullptr;
    }

protected:
    /*
     * You can construct a expression without specifying a type,
     * then the type should be evaluated in TypeChecking()
     */
    Expr(MemPool* pool, Type* type, bool isConstant = false)
        : Stmt(pool), _ty(type), _isConsant(isConstant) {}
    
    /*
     * Do type checking and evaluating the expression type;
     * called after construction
     */
    virtual Expr* TypeChecking(void) = 0;

    Type* _ty;
    bool _isConsant;
};


/***********************************************************
'+', '-', '*', '/', '%', '<', '>', '<<', '>>', '|', '&', '^'
'=',(复合赋值运算符被拆分为两个运算)
'==', '!=', '<=', '>=',
'&&', '||'
'['(下标运算符), '.'(成员运算符), '->'(成员运算符)
','(逗号运算符),
*************************************************************/
class BinaryOp : public Expr
{
    friend class TranslationUnit;

public:
    virtual ~BinaryOp(void) {}
    
    virtual void Accept(Visitor* v);
    
    //like member ref operator is a lvalue
    virtual bool IsLVal(void) const { return false; }

    virtual bool EvaluateConstant(Constant* cons);

protected:
    BinaryOp(MemPool* pool, int op, Expr* lhs, Expr* rhs)
        :Expr(pool, nullptr), _op(op), _lhs(lhs), _rhs(rhs) {}

    // TODO: 
    //  1.type checking;
    //  2. evalute the type;
    virtual BinaryOp* TypeChecking(void);

    BinaryOp* SubScriptingOpTypeChecking(void);
    
    BinaryOp* MemberRefOpTypeChecking(const char* rhsName);
    
    BinaryOp* MultiOpTypeChecking(void);
    
    BinaryOp* AdditiveOpTypeChecking(void);
    
    BinaryOp* ShiftOpTypeChecking(void);
    
    BinaryOp* RelationalOpTypeChecking(void);
    
    BinaryOp* EqualityOpTypeChecking(void);
    
    BinaryOp* BitwiseOpTypeChecking(void);
    
    BinaryOp* LogicalOpTypeChecking(void);
    
    BinaryOp* AssignOpTypeChecking(void);

    int _op;
    Expr* _lhs;
    Expr* _rhs;
};


/************* Unary Operator ****************/
class UnaryOp : public Expr
{
    friend class TranslationUnit;

public:
    virtual ~UnaryOp(void) {}
    virtual void Accept(Visitor* v);
    //TODO: like '*p' is lvalue, but '~i' is not lvalue
    virtual bool IsLVal(void) const {
        /*only deref('*') op is lvalue;
        so it's only deref with override this func*/
        return (Token::DEREF == _op);
    }

protected:
    UnaryOp(MemPool* pool, int op, Expr* operand, Type* type = nullptr)
        : Expr(pool, type), _op(op), _operand(operand) {}

    virtual UnaryOp* TypeChecking(void);
    
    UnaryOp* IncDecOpTypeChecking(void);
    
    UnaryOp* AddrOpTypeChecking(void);
    
    UnaryOp* DerefOpTypeChecking(void);
    
    UnaryOp* UnaryArithmOpTypeChecking(void);
    
    UnaryOp* CastOpTypeChecking(void);

    int _op;
    Expr* _operand;
};


// cond ？ true ： false
class ConditionalOp : public Expr
{
    friend class TranslationUnit;

public:
    virtual ~ConditionalOp(void) {}
    
    virtual void Accept(Visitor* v);
    
    virtual bool IsLVal(void) const { return false; }

protected:
    ConditionalOp(MemPool* pool, Expr* cond, Expr* exprTrue, Expr* exprFalse)
            : Expr(pool, nullptr), _cond(cond), 
              _exprTrue(exprTrue), _exprFalse(exprFalse) {}
    
    virtual ConditionalOp* TypeChecking(void);

private:
    Expr* _cond;
    Expr* _exprTrue;
    Expr* _exprFalse;
};


/************** Function Call ****************/
class FuncCall : public Expr
{
    friend class TranslationUnit;

public:
    ~FuncCall(void) {}
    virtual void Accept(Visitor* v);
    //a function call is ofcourse not lvalue
    virtual bool IsLVal(void) const { return false; }

protected:
    FuncCall(MemPool* pool, Expr* designator, std::list<Expr*> args)
        : Expr(pool, nullptr), _designator(designator), _args(args) {}

    virtual FuncCall* TypeChecking(void);

    Expr* _designator;
    std::list<Expr*> _args;
};


/********* Identifier *************/
class Variable : public Expr
{
    friend class TranslationUnit;

public:
    static const int TYPE = -1;
    static const int VAR = 0;
    
    ~Variable(void) {}
    
    virtual void Accept(Visitor* v);
    
    bool IsVar(void) const {
        return _offset >= 0;
    }

    int Offset(void) const { return _offset; }
        
    void SetOffset(int offset) { _offset = offset; }
    
    int Storage(void) const { return _storage; }

    void SetStorage(int storage) { _storage = storage; }

    //of course a variable is a lvalue expression
    virtual bool IsLVal(void) const { return true; }

    bool operator==(const Variable& other) const {
        return _offset == other._offset
            && *_ty == *other._ty;
    }

    bool operator!=(const Variable& other) const {
        return !(*this == other);
    }

    virtual Constant* ToConstant(void) { return nullptr; }
    
    virtual const Constant* ToConstant(void) const { return nullptr; }

    Variable* GetStructMember(const char* name);

    Variable* GetArrayElement(TranslationUnit* unit, size_t idx);

protected:
    Variable(MemPool* pool, Type* type, int offset=VAR, bool isConstant=false)
        : Expr(pool, type, isConstant), _storage(0), _offset(offset) {}
    //do nothing
    virtual Variable* TypeChecking(void) { return this; }

protected:
    int _storage;

private:
    //the relative address
    int _offset;
};


//integer, character, string literal, floating
class Constant : public Variable
{
    friend class TranslationUnit;

public:
    ~Constant(void) {}
    
    virtual void Accept(Visitor* v);
    
    virtual bool IsLVal(void) const { return false; }
    
    long long IVal(void) const { return _ival; }
    
    double FVal(void) const { return _fval; }

    virtual Constant* ToConstant(void) {
        return this;
    }

protected:
    Constant(MemPool* pool, ArithmType* type, long long val)
            : Variable(pool, type, VAR, true), _ival(val) {
        assert(type->IsInteger());
    }

    Constant(MemPool* pool, ArithmType* type, double val)
            : Variable(pool, type, VAR, true), _fval(val) {
        assert(type->IsFloat());
    }
/*
    Constant(MemPool* pool, const char* val)
            : Variable(pool, , VAR, true), _sval(val) {

    }
*/
    virtual Constant* TypeChecking(void) {
        return this;
    }

private:
    union {
        long long _ival;
        double _fval;
        std::string _sval;
    };
};


//临时变量
class TempVar : public Expr
{
    friend class TranslationUnit;

public:
    virtual ~TempVar(void) {}
    virtual void Accept(Visitor* v);
    virtual bool IsLVal(void) const { return true; }

protected:
    TempVar(MemPool* pool, Type* type)
            : Expr(pool, type), _tag(GenTag()) {}
    virtual TempVar* TypeChecking(void) { return this; }

private:
    static int GenTag(void) {
        static int tag = 0;
        return ++tag;
    }

    int _tag;
};


/*************** Declaration ******************/

class FuncDef : public ExtDecl
{
    friend class TranslationUnit;

public:
    virtual ~FuncDef(void) {}
    virtual void Accept(Visitor* v);

protected:
    FuncDef(MemPool* pool, FuncType* type, CompoundStmt* stmt)
            : ExtDecl(pool), _type(type), _stmt(stmt) {}

private:
    FuncType* _type;
    CompoundStmt* _stmt;
};

/*
class Decl : public ExtDecl
{

};
*/


class TranslationUnit : public ASTNode
{
public:
    virtual ~TranslationUnit(void) {
        auto iter = _extDecls.begin();
        for (; iter != _extDecls.end(); iter++)
            delete *iter;
    }

    virtual void Accept(Visitor* v);
    
    void Add(ExtDecl* extDecl) {
        _extDecls.push_back(extDecl);
    }

    static TranslationUnit* NewTranslationUnit(void) {
        return new TranslationUnit();
    }

    /*
     * Binary Operator
     */
    BinaryOp* NewBinaryOp(int op, Expr* lhs, Expr* rhs);
    BinaryOp* NewMemberRefOp(int op, Expr* lhs, const char* rhsName);
    ConditionalOp* NewConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse);
    FuncCall* NewFuncCall(Expr* designator, const std::list<Expr*>& args);
    Variable* NewVariable(Type* type, int offset=0);
    Constant* NewConstantInteger(ArithmType* type, long long val);
    Constant* NewConstantFloat(ArithmType* type, double val);
    TempVar* NewTempVar(Type* type);
    UnaryOp* NewUnaryOp(int op, Expr* operand, Type* type=nullptr);

    /*
     * Statement
     */
    EmptyStmt* NewEmptyStmt(void);
    IfStmt* NewIfStmt(Expr* cond, Stmt* then, Stmt* els=nullptr);
    JumpStmt* NewJumpStmt(LabelStmt* label);
    ReturnStmt* NewReturnStmt(Expr* expr);
    LabelStmt* NewLabelStmt(void);
    CompoundStmt* NewCompoundStmt(std::list<Stmt*>& stmts);

    /*
     * Function Definition
     */
    FuncDef* NewFuncDef(FuncType* type, CompoundStmt* stmt);

    void Delete(ASTNode* node);

private:
    TranslationUnit(void): ASTNode(nullptr) {}

    std::list<ExtDecl*> _extDecls;

    MemPoolImp<BinaryOp>        _binaryOpPool;
    MemPoolImp<ConditionalOp>   _conditionalOpPool;
    MemPoolImp<FuncCall>        _funcCallPool;
    MemPoolImp<Variable>        _variablePool;
    MemPoolImp<Constant>        _constantPool;
    MemPoolImp<TempVar>         _tempVarPool;
    MemPoolImp<UnaryOp>         _unaryOpPool;
    MemPoolImp<EmptyStmt>       _emptyStmtPool;
    MemPoolImp<IfStmt>          _ifStmtPool;
    MemPoolImp<JumpStmt>        _jumpStmtPool;
    MemPoolImp<ReturnStmt>      _returnStmtPool;
    MemPoolImp<LabelStmt>       _labelStmtPool;
    MemPoolImp<CompoundStmt>    _compoundStmtPool;
    MemPoolImp<FuncDef>         _funcDefPool;
};

/*
class ASTVisitor
{
public:
    virtual void Visit(ASTNode* node) = 0;
};
*/

#endif
