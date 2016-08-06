#ifndef _WGTCC_AST_H_
#define _WGTCC_AST_H_

#include "error.h"
#include "string_pair.h"
#include "type.h"

#include <cassert>
#include <list>
#include <memory>


class Operand;
class Generator;

class Scope;
class Parser;
class ASTNode;
class Token;

//Expression
class Expr;
class BinaryOp;
class UnaryOp;
class ConditionalOp;
class FuncCall;
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


/*
 * AST Node
 */

class ASTNode
{
    friend class Parser;

public:
    virtual ~ASTNode(void) {}
    
    virtual Operand* Accept(Generator* g) = 0;

    //virtual Coordinate Coord(void) = 0;
    
protected:
    explicit ASTNode(MemPool* pool): _pool(pool) {}

    mutable std::string _str;

private:
    //ASTNode(void);  //禁止直接创建Node

    MemPool* _pool;
};

typedef ASTNode ExtDecl;


/*********** Statement *************/

class Stmt : public ASTNode
{
public:
    virtual ~Stmt(void){}

protected:
    explicit Stmt(MemPool* pool) : ASTNode(pool) {}
};


class EmptyStmt : public Stmt
{
    friend class Parser;

public:
    virtual ~EmptyStmt(void) {}
    
    virtual Operand* Accept(Generator* g);

protected:
    EmptyStmt(MemPool* pool): Stmt(pool) {}
};


// 构建此类的目的在于，在目标代码生成的时候，能够生成相应的label
class LabelStmt : public Stmt
{
    friend class Parser;

public:
    ~LabelStmt(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    int Tag(void) const {
        return Tag();
    }

protected:
    LabelStmt(MemPool* pool)
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
    friend class Parser;

public:
    virtual ~IfStmt(void) {}
    
    virtual Operand* Accept(Generator* g);

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
    friend class Parser;

public:
    virtual ~JumpStmt(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    void SetLabel(LabelStmt* label) { _label = label; }

protected:
    JumpStmt(MemPool* pool, LabelStmt* label)
            : Stmt(pool), _label(label) {}

private:
    LabelStmt* _label;
};

class ReturnStmt: public Stmt
{
    friend class Parser;

public:
    virtual ~ReturnStmt(void) {}
    
    virtual Operand* Accept(Generator* g);
    
protected:
    ReturnStmt(MemPool* pool, Expr* expr)
            : Stmt(pool), _expr(expr) {}

private:
    Expr* _expr;
};

class CompoundStmt : public Stmt
{
    friend class Parser;

public:
    virtual ~CompoundStmt(void) {}
    
    virtual Operand* Accept(Generator* g);

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
 *      Constant
 *      Identifier
 *          Object
 *      TempVar
 */

class Expr : public Stmt
{
    friend class Parser;

public:
    virtual ~Expr(void) {}
    
    ::Type* Type(void) {
        return _type;
    }

    virtual bool IsLVal(void) const = 0;

    virtual long long EvalInteger(const Token* errTok) = 0;

protected:
    /*
     * You can construct a expression without specifying a type,
     * then the type should be evaluated in TypeChecking()
     */
    Expr(MemPool* pool, ::Type* type): Stmt(pool), _type(type) {}

    ::Type* _type;
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
    friend class Parser;

public:
    virtual ~BinaryOp(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    //like member ref operator is a lvalue
    virtual bool IsLVal(void) const {
        return false;
    }

    virtual long long EvalInteger(const Token* errTok);

    ArithmType* Promote(Parser* parser, const Token* errTok);

protected:
    BinaryOp(MemPool* pool, int op, Expr* lhs, Expr* rhs)
            : Expr(pool, nullptr), _op(op), _lhs(lhs), _rhs(rhs) {}

    int _op;
    Expr* _lhs;
    Expr* _rhs;
};


/*
 * Unary Operator:
 * '++' (prefix/postfix)
 * '--' (prefix/postfix)
 * '&'  (ADDR)
 * '*'  (DEREF)
 * '+'  (PLUS)
 * '-'  (MINUS)
 * '~'
 * '!'
 * CAST // like (int)3
 */
class UnaryOp : public Expr
{
    friend class Parser;

public:
    virtual ~UnaryOp(void) {}
    
    virtual Operand* Accept(Generator* g);

    //TODO: like '*p' is lvalue, but '~i' is not lvalue
    virtual bool IsLVal(void) const;

    virtual long long EvalInteger(const Token* errTok);

    ArithmType* Promote(Parser* parser, const Token* errTok);

protected:
    UnaryOp(MemPool* pool, int op, Expr* operand, ::Type* type = nullptr)
        : Expr(pool, type), _op(op), _operand(operand) {}

    int _op;
    Expr* _operand;
};


// cond ? true ： false
class ConditionalOp : public Expr
{
    friend class Parser;

public:
    virtual ~ConditionalOp(void) {}
    
    virtual Operand* Accept(Generator* g);

    virtual bool IsLVal(void) const {
        return false;
    }

    virtual long long EvalInteger(const Token* errTok);

    ArithmType* Promote(Parser* parser, const Token* errTok);

protected:
    ConditionalOp(MemPool* pool, Expr* cond, Expr* exprTrue, Expr* exprFalse)
            : Expr(pool, nullptr), _cond(cond), 
              _exprTrue(exprTrue), _exprFalse(exprFalse) {}

private:
    Expr* _cond;
    Expr* _exprTrue;
    Expr* _exprFalse;
};


/************** Function Call ****************/
class FuncCall : public Expr
{
    friend class Parser;
    typedef std::list<Expr*> ArgList;
public:
    ~FuncCall(void) {}
    
    virtual Operand* Accept(Generator* g);

    //a function call is ofcourse not lvalue
    virtual bool IsLVal(void) const {
        return false;
    }

    virtual long long EvalInteger(const Token* errTok);

    ArgList* Args(void) {
        return &_args;
    }

    Expr* Designator(void) {
        return _designator;
    }


protected:
    FuncCall(MemPool* pool, Expr* designator, std::list<Expr*> args)
        : Expr(pool, nullptr), _designator(designator), _args(args) {}

    Expr* _designator;
    ArgList _args;
};


/********* Identifier *************/


//integer, character, string literal, floating
class Constant : public Expr
{
    friend class Parser;

public:
    ~Constant(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    virtual bool IsLVal(void) const {
        return false;
    }

    virtual long long EvalInteger(const Token* errTok);
    
    long long IVal(void) const {
        return _ival;
    }
    
    double FVal(void) const {
        return _fval;
    }

protected:
    Constant(MemPool* pool, ArithmType* type, long long val)
            : Expr(pool, type), _ival(val) {
        assert(type->IsInteger());
    }

    Constant(MemPool* pool, ArithmType* type, double val)
            : Expr(pool, type), _fval(val) {
        assert(type->IsFloat());
    }
    /*
    Constant(MemPool* pool, const char* val)
            : Expr(pool, , VAR, true), _sval(val) {

    }
    */

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
    friend class Parser;

public:
    virtual ~TempVar(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    virtual bool IsLVal(void) const {
        return true;
    }

    virtual long long EvalInteger(const Token* errTok);

protected:
    TempVar(MemPool* pool, ::Type* type)
            : Expr(pool, type), _tag(GenTag()) {}
    
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
    friend class Parser;

public:
    virtual ~FuncDef(void) {}
    
    virtual FuncType* Type(void) {
        return _type;
    }
    
    virtual Operand* Accept(Generator* g);

protected:
    FuncDef(MemPool* pool, FuncType* type, CompoundStmt* stmt)
            : ExtDecl(pool), _type(type), _stmt(stmt) {}

private:
    FuncType* _type;
    CompoundStmt* _stmt;
};


class TranslationUnit : public ASTNode
{
public:
    virtual ~TranslationUnit(void) {
        auto iter = _extDecls.begin();
        for (; iter != _extDecls.end(); iter++)
            delete *iter;
    }

    virtual Operand* Accept(Generator* g);
    
    void Add(ExtDecl* extDecl) {
        _extDecls.push_back(extDecl);
    }

    static TranslationUnit* NewTranslationUnit(void) {
        return new TranslationUnit();
    }

private:
    TranslationUnit(void): ASTNode(nullptr) {}

    std::list<ExtDecl*> _extDecls;
};

/*
class ASTVisitor
{
public:
    virtual void Visit(ASTNode* node) = 0;
};
*/


enum Linkage {
    L_NONE,
    L_EXTERNAL,
    L_INTERNAL,
};


class Identifier: public Expr
{
    friend class Parser;
    
public:
    virtual ~Identifier(void) {}

    virtual Operand* Accept(Generator* g) {
        return nullptr;
    }

    virtual bool IsLVal(void) const {
        return false;
    }

    virtual Object* ToObject(void) {
        return nullptr;
    }

    virtual long long EvalInteger(const Token* errTok);

    /*
     * An identifer can be:
     *     object, sturct/union/enum tag, typedef name, function, label.
     */
    virtual ::Type* ToType(void) {
        if (ToObject())
            return nullptr;
        return this->Type();
    }

    ::Scope* Scope(void) {
        return _scope;
    }

    enum Linkage Linkage(void) const {
        return _linkage;
    }

    void SetLinkage(enum Linkage linkage) {
        _linkage = linkage;
    }

    const Token* Tok(void) {
        return _tok;
    }

    const std::string Name(void);

    virtual bool operator==(const Identifier& other) const {
        return *_type == *other._type && _scope == other._scope;
    }

protected:
    Identifier(MemPool* pool, const Token* tok, 
            ::Type* type, ::Scope* scope, enum Linkage linkage)
            : Expr(pool, type), _tok(tok), _scope(scope), _linkage(linkage) {}
    
    const Token* _tok;

    // An identifier has property scope
    ::Scope* _scope;
    // An identifier has property linkage
    enum Linkage _linkage;
};


class Object : public Identifier
{
    friend class Parser;

public:
    ~Object(void) {}

    virtual Operand* Accept(Generator* g);
    
    virtual bool IsLVal(void) const {
        // TODO(wgtdkp): not all object is lval?
        return true;
    }

    virtual Object* ToObject(void) {
        return this;
    }

    int Storage(void) const {
        return _storage;
    }

    void SetStorage(int storage) {
        _storage = storage;
    }

    int Offset(void) const {
        return _offset;
    }

    void SetOffset(int offset) {
        _offset = offset;
    }

    /*
    // of course a variable is a lvalue expression
    virtual bool IsLVal(void) const {
        return true;
    }
    */

    bool operator==(const Object& other) const {
        // TODO(wgtdkp): Not implemented
        assert(0);
    }

    bool operator!=(const Object& other) const {
        return !(*this == other);
    }

protected:
    Object(MemPool* pool, const Token* tok, ::Type* type, ::Scope* scope,
            int storage=0, enum Linkage linkage=L_NONE)
            : Identifier(pool, tok, type, scope, linkage),
              _storage(0), _offset(0) {}

private:
    int _storage;
    
    // For code gen
    int _offset;
};

#endif
