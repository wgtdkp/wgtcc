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
    friend class Generator;

public:
    virtual ~ASTNode(void) {}
    
    virtual Operand* Accept(Generator* g) = 0;

    //virtual Coordinate Coord(void) = 0;

protected:
    ASTNode(void) {}

    //mutable std::string _str;
    MemPool* _pool;
};

typedef ASTNode ExtDecl;


/*********** Statement *************/

class Stmt : public ASTNode
{
public:
    virtual ~Stmt(void) {}

    virtual void TypeChecking( const Token* errTok) {}

protected:
     Stmt(void) {}
};


class EmptyStmt : public Stmt
{
public:
    static EmptyStmt* New(void);

    virtual ~EmptyStmt(void) {}
    
    virtual Operand* Accept(Generator* g);

protected:
    EmptyStmt(void) {}
};


// 构建此类的目的在于，在目标代码生成的时候，能够生成相应的label
class LabelStmt : public Stmt
{
public:
    static LabelStmt* New(void);

    ~LabelStmt(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    int Tag(void) const {
        return Tag();
    }

protected:
    LabelStmt(void): _tag(GenTag()) {}

private:
    static int GenTag(void) {
        static int tag = 0;
        return ++tag;
    }
    
    int _tag; // 使用整型的tag值，而不直接用字符串
};


class IfStmt : public Stmt
{
public:
    static IfStmt* New(Expr* cond, Stmt* then, Stmt* els=nullptr);

    virtual ~IfStmt(void) {}
    
    virtual Operand* Accept(Generator* g);

protected:
    IfStmt(Expr* cond, Stmt* then, Stmt* els = nullptr)
            : _cond(cond), _then(then), _else(els) {}

private:
    Expr* _cond;
    Stmt* _then;
    Stmt* _else;
};


class JumpStmt : public Stmt
{
public:
    static JumpStmt* New(LabelStmt* label);

    virtual ~JumpStmt(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    void SetLabel(LabelStmt* label) { _label = label; }

protected:
    JumpStmt(LabelStmt* label): _label(label) {}

private:
    LabelStmt* _label;
};

class ReturnStmt: public Stmt
{
public:
    static ReturnStmt* New(Expr* expr);

    virtual ~ReturnStmt(void) {}
    
    virtual Operand* Accept(Generator* g);
    
protected:
    ReturnStmt(Expr* expr): _expr(expr) {}

private:
    Expr* _expr;
};

class CompoundStmt : public Stmt
{
public:
    static CompoundStmt* New(std::list<Stmt*>& stmts, ::Scope* scope=nullptr);

    virtual ~CompoundStmt(void) {}
    
    virtual Operand* Accept(Generator* g);

    std::list<Stmt*>& Stmts(void) {
        return _stmts;
    }

    ::Scope* Scope(void) {
        return _scope;
    }

protected:
    CompoundStmt(const std::list<Stmt*>& stmts, ::Scope* scope=nullptr)
            : _stmts(stmts), _scope(scope) {}

private:
    std::list<Stmt*> _stmts;
    ::Scope* _scope;
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
    friend class Generator;

public:
    virtual ~Expr(void) {}
    
    ::Type* Type(void) {
        return _type;
    }

    virtual bool IsLVal(void) const = 0;

    virtual long long EvalInteger(const Token* errTok) = 0;

    virtual void TypeChecking(const Token* errTok) = 0;

    virtual std::string Name(void) {
        return "";
    }

protected:
    /*
     * You can construct a expression without specifying a type,
     * then the type should be evaluated in TypeChecking()
     */
    Expr(::Type* type): _type(type) {}

    ::Type* _type;
};


/***********************************************************
'+', '-', '*', '/', '%', '<', '>', '<<', '>>', '|', '&', '^'
'=',(复合赋值运算符被拆分为两个运算)
'==', '!=', '<=', '>=',
'&&', '||'
'['(下标运算符), '.'(成员运算符)
','(逗号运算符),
*************************************************************/
class BinaryOp : public Expr
{
    friend class Parser;
    friend class Generator;

public:
    static BinaryOp* New(const Token* tok, Expr* lhs, Expr* rhs);

    static BinaryOp* New(const Token* tok, int op, Expr* lhs, Expr* rhs);

    virtual ~BinaryOp(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    //like member ref operator is a lvalue
    virtual bool IsLVal(void) const {
        return false;
    }

    virtual long long EvalInteger(const Token* errTok);

    ArithmType* Promote(const Token* errTok);

    virtual void TypeChecking(const Token* errTok);
    void SubScriptingOpTypeChecking(const Token* errTok);
    void MemberRefOpTypeChecking(const Token* errTok);
    void MultiOpTypeChecking(const Token* errTok);
    void AdditiveOpTypeChecking(const Token* errTok);
    void ShiftOpTypeChecking(const Token* errTok);
    void RelationalOpTypeChecking(const Token* errTok);
    void EqualityOpTypeChecking(const Token* errTok);
    void BitwiseOpTypeChecking(const Token* errTok);
    void LogicalOpTypeChecking(const Token* errTok);
    void AssignOpTypeChecking(const Token* errTok);

protected:
    BinaryOp(int op, Expr* lhs, Expr* rhs)
            : Expr(nullptr), _op(op), _lhs(lhs), _rhs(rhs) {}

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
    friend class Generator;

public:
    static UnaryOp* New(const Token* tok,
            int op, Expr* operand, ::Type* type=nullptr);

    virtual ~UnaryOp(void) {}
    
    virtual Operand* Accept(Generator* g);

    //TODO: like '*p' is lvalue, but '~i' is not lvalue
    virtual bool IsLVal(void) const;

    virtual long long EvalInteger(const Token* errTok);

    ArithmType* Promote(Parser* parser, const Token* errTok);

    void TypeChecking(const Token* errTok);
    void IncDecOpTypeChecking(const Token* errTok);
    void AddrOpTypeChecking(const Token* errTok);
    void DerefOpTypeChecking(const Token* errTok);
    void UnaryArithmOpTypeChecking(const Token* errTok);
    void CastOpTypeChecking(const Token* errTok);

protected:
    UnaryOp(int op, Expr* operand, ::Type* type = nullptr)
        : Expr(type), _op(op), _operand(operand) {}

    int _op;
    Expr* _operand;
};


// cond ? true ： false
class ConditionalOp : public Expr
{
    friend class Parser;
    friend class Generator;

public:
    static ConditionalOp* New(const Token* tok,
            Expr* cond, Expr* exprTrue, Expr* exprFalse);
    
    virtual ~ConditionalOp(void) {}
    
    virtual Operand* Accept(Generator* g);

    virtual bool IsLVal(void) const {
        return false;
    }

    virtual long long EvalInteger(const Token* errTok);

    ArithmType* Promote(const Token* errTok);
    
    virtual void TypeChecking(const Token* errTok);

protected:
    ConditionalOp(Expr* cond, Expr* exprTrue, Expr* exprFalse)
            : Expr(nullptr), _cond(cond),
              _exprTrue(exprTrue), _exprFalse(exprFalse) {}

private:
    Expr* _cond;
    Expr* _exprTrue;
    Expr* _exprFalse;
};


/************** Function Call ****************/
class FuncCall : public Expr
{
    typedef std::list<Expr*> ArgList;

public:
    static FuncCall* New(const Token* tok,
            Expr* designator, const std::list<Expr*>& args);

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

    virtual void TypeChecking(const Token* errTok);

protected:
    FuncCall(Expr* designator, std::list<Expr*> args)
        : Expr(nullptr), _designator(designator), _args(args) {}

    Expr* _designator;
    ArgList _args;
};


/********* Identifier *************/


//integer, character, string literal, floating
class Constant : public Expr
{
public:
    static Constant* New(int tag, long long val);
    static Constant* New(ArithmType* type, double val);

    ~Constant(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    virtual bool IsLVal(void) const {
        return false;
    }

    virtual long long EvalInteger(const Token* errTok);
    
    virtual void TypeChecking(const Token* errTok) {}

    long long IVal(void) const {
        return _ival;
    }
    
    double FVal(void) const {
        return _fval;
    }


protected:
    Constant(ArithmType* type, long long val): Expr(type), _ival(val) {
        assert(type->IsInteger());
    }

    Constant(ArithmType* type, double val): Expr(type), _fval(val) {
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
public:
    static TempVar* New(::Type* type);

    virtual ~TempVar(void) {}
    
    virtual Operand* Accept(Generator* g);
    
    virtual bool IsLVal(void) const {
        return true;
    }

    virtual long long EvalInteger(const Token* errTok);
    
    virtual void TypeChecking(const Token* errTok) {}

protected:
    TempVar(::Type* type): Expr(type), _tag(GenTag()) {}
    
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
public:
    static FuncDef* New(FuncType* type,
            const std::list<Object*>& params, CompoundStmt* stmt);

    virtual ~FuncDef(void) {}
    
    virtual FuncType* Type(void) {
        return _type;
    }

    std::list<Object*>& Params(void) {
        return _params;
    }

    CompoundStmt* Body(void) {
        return _body;
    }
    
    virtual Operand* Accept(Generator* g);

protected:
    FuncDef(FuncType* type, const std::list<Object*>& params,
            CompoundStmt* stmt): _type(type), _params(params), _body(stmt) {}

private:
    FuncType* _type;
    std::list<Object*> _params;
    CompoundStmt* _body;
};


class TranslationUnit : public ASTNode
{
public:
    static TranslationUnit* New(void) {
        return new TranslationUnit();
    }

    virtual ~TranslationUnit(void) {}

    virtual Operand* Accept(Generator* g);
    
    void Add(ExtDecl* extDecl) {
        _extDecls.push_back(extDecl);
    }

    std::list<ExtDecl*>& ExtDecls(void) {
        return _extDecls;
    }

private:
    TranslationUnit(void) {}

    std::list<ExtDecl*> _extDecls;
};


enum Linkage {
    L_NONE,
    L_EXTERNAL,
    L_INTERNAL,
};


class Identifier: public Expr
{
public:
    static Identifier* New(const Token* tok,
            ::Type* type, Scope* scope, enum Linkage linkage);

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

    virtual std::string Name(void);

    virtual bool operator==(const Identifier& other) const {
        return *_type == *other._type && _scope == other._scope;
    }

    virtual void TypeChecking(const Token* errTok) {}

protected:
    Identifier(const Token* tok, ::Type* type,
            ::Scope* scope, enum Linkage linkage)
            : Expr(type), _tok(tok), _scope(scope), _linkage(linkage) {}
    
    const Token* _tok;

    // An identifier has property scope
    ::Scope* _scope;
    // An identifier has property linkage
    enum Linkage _linkage;
};


class Object : public Identifier
{
public:
    static Object* New(const Token* tok, ::Type* type, ::Scope* scope,
            int storage=0, enum Linkage linkage=L_NONE);

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
    Object(const Token* tok, ::Type* type, ::Scope* scope,
            int storage=0, enum Linkage linkage=L_NONE)
            : Identifier(tok, type, scope, linkage),
              _storage(0), _offset(0) {}

private:
    int _storage;
    
    // For code gen
    int _offset;
};

#endif
