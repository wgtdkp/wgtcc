#ifndef _WGTCC_CODE_GEN_H_
#define _WGTCC_CODE_GEN_H_

#include "ast.h"
#include "mem_pool.h"
#include "token.h"
#include "type.h"

#include <cassert>
#include <cstdio>

#include <string>


class Parser;

class Memory;
class Register;
class Immediate;


class Operand
{
public:
    virtual ~Operand(void) {
        _pool->Free(this);
    }

    virtual std::string Repr(void) const = 0;

    virtual Immediate* ToImmediate(void) {
        return nullptr;
    }

    virtual Register* ToRegister(void) {
        return nullptr;
    }

    virtual Memory* ToMemory(void) {
        return nullptr;
    }

    int Width(void) {
        return _width;
    }

    void SetWidth(int width) {
        _width = width;
    }

protected:
    explicit Operand(int width): _width(width) {}

    int _width;

    MemPool* _pool;
};


enum class ParamClass
{
    INTEGER,
    SSE,
    SSEUP,
    X87,
    X87_UP,
    COMPLEX_X87,
    NO_CLASS,
    MEMORY
};


class Register: public Operand
{
public:
    enum {
        RAX, RBX, RCX, RDX,
        RSI, RDI, RBP, RSP,
        R8, R9, R10, R11,
        R12, R13, R14, R15,
        XMM0, XMM1, XMM2, XMM3,
        XMM4, XMM5, XMM6, XMM7,
        N_REG
    };

    ~Register(void) {}

    virtual std::string Repr(void) const;

    bool  Is(int tag) const {
        return _tag == tag;
    }

    static Register* Get(int tag) {
        assert(0 <= tag && tag < N_REG);

        return _regs[tag];
    }

    void Bind(::Expr* expr, int width=-1) {
        _expr = expr;
        if (width == -1)
            _width = expr->Type()->Width();
        else
            _width = width;
    }

    void Unbind(void) {
        _expr = nullptr;
    }

    ::Expr* Expr(void) {
        return _expr;
    }

    bool Using(void) {
        return _expr != nullptr;
    }

private:
    static Register* New(int tag, int width=8);
    
    explicit Register(int tag, int width=8)
            : Operand(width), _tag(tag) {}

    ::Expr* _expr;
    int _tag;

    static Register* _regs[N_REG];
    static const char* _reprs[N_REG][4];
};


class Immediate: public Operand
{
public:
    static Immediate* New(Constant* cons);

    static Immediate* New(int tag, long long val) {
        //TODO(wgtdkp):
        //auto cons = Constant::New(tag, val);
        //return New(cons);
        return nullptr;
    }

    ~Immediate(void) {}

    virtual std::string Repr(void) const;

    virtual Immediate* ToImmediate(void) {
        return this;
    }

    Constant* Cons(void) {
        return _cons;
    }

private:
    explicit Immediate(Constant* cons)
            : Operand(cons->Type()->Width()), _cons(cons) {}

    Constant* _cons;
};


/*
 * Direct   // Often just symbolic name for a location of 
 *          // data section(static object) or code section(function)
 * Indirect // 
 * Base + displacement
 * (index * scale) + displacement
 * Base + index + displacement
 * Base + (index * scale) + displacement
 */
class Memory: public Operand
{
    friend class Generator;

public:
    static Memory* New(int width, Register* base, int disp,
            Register* index=nullptr, int scale=0);

    ~Memory(void) {}

    virtual std::string Repr(void) const;

    virtual Memory* ToMemory(void) {
        return this;
    }

private:
    Memory(int width, Register* base, int disp,
            Register* index=nullptr, int scale=0)
            : Operand(width), _base(base),
              _index(index), _scale(scale), _disp(disp) {}

    //Memory()

    Register* _base;
    Register* _index;
    int _scale;
    int _disp;
    std::string _symb;
};


class Generator
{
public:
    Generator(Parser* parser, FILE* outFile)
            : _parser(parser), _outFile(outFile),
              _desReg(Register::Get(Register::RAX)),
              _argRegUsed(0), _argVecRegUsed(0),
              _argStackOffset(-8), _stackOffset(0) {}

    //Expression
    virtual Operand* GenBinaryOp(BinaryOp* binaryOp);
    virtual Operand* GenUnaryOp(UnaryOp* unaryOp);
    virtual Operand* GenConditionalOp(ConditionalOp* condOp);
    virtual Register* GenFuncCall(FuncCall* funcCall);
    virtual Memory* GenObject(Object* obj);
    virtual Immediate* GenConstant(Constant* cons);
    virtual Register* GenTempVar(TempVar* tempVar);

    Operand* GenMemberRefOp(BinaryOp* binaryOp);
    Operand* GenSubScriptingOp(BinaryOp* binaryOp);
    Operand* GenAndOp(BinaryOp* binaryOp);
    Operand* GenOrOp(BinaryOp* binaryOp);
    Register* GenAddOp(BinaryOp* binaryOp);
    Register* GenSubOp(BinaryOp* binaryOp);
    Operand* GenAssignOp(BinaryOp* binaryOp);

    //statement
    virtual void GenStmt(Stmt* stmt);
    virtual void GenIfStmt(IfStmt* ifStmt);
    virtual void GenJumpStmt(JumpStmt* jumpStmt);
    virtual void GenReturnStmt(ReturnStmt* returnStmt);
    virtual void GenLabelStmt(LabelStmt* labelStmt);
    virtual void GenEmptyStmt(EmptyStmt* emptyStmt);
    virtual void GenCompoundStmt(CompoundStmt* compoundStmt);

    //Function Definition
    virtual Register* GenFuncDef(FuncDef* funcDef);

    //Translation Unit
    virtual void GenTranslationUnit(TranslationUnit* unit);
    void Gen(void);

    void PushFuncArg(Expr* arg, ParamClass cls);
    void PushReturnAddr(int addr, Register* reg);

    Register* AllocArgReg(void) {
        if (_argRegUsed >= N_ARG_REG)
            return nullptr;

        auto ret = _argRegs[_argRegUsed++];
        //assert(!ret->Using());
        return ret;
    }

    Register* AllocArgVecReg(void) {
        if (_argVecRegUsed >= N_ARG_VEC_REG)
            return nullptr;
        
        auto ret = _argVecRegs[_argVecRegUsed++];
        //assert(!ret->Using());
        return ret;
    }

    int AllocArgStack(void) {
        _argStackOffset -= 8;
        return _argStackOffset;
    }

    void SetDesReg(Register* desReg) {
        //assert(!desReg->Using());
        _desReg = desReg;
    }

    Register* AllocReg(Expr* expr, bool addr=false);
    void FreeReg(Register* reg);

    void Emit(const char* format, ...);
    void Emit(const char* inst, Operand* lhs, Operand* rhs);
    void EmitMOV(Operand* src, Operand* des);
    //void EmitMOV(Register* src, Operand* des) {}
    void EmitLEA(Memory* mem, Register* reg);
    void EmitJE(LabelStmt* label);
    void EmitJNE(LabelStmt* label);
    void EmitJMP(LabelStmt* label);
    void EmitCMP(Immediate* lhs, Operand* rhs);
    void EmitPUSH(Operand* operand);
    void EmitPOP(Operand* operand);
    void EmitLabel(LabelStmt* label);

private:
    Parser* _parser;
    FILE* _outFile;

    // The destination register for current expression
    // Setted after translation of the expression
    Register* _desReg;

    // The number of argument passing register used
    int _argRegUsed;

    // The number of argument passing vector register used
    int _argVecRegUsed;

    // The stack pointer after pushed arguments on the stack 
    int _argStackOffset;

    int _stackOffset;

    static const int N_ARG_REG = 6;
    static Register* _argRegs[N_ARG_REG];

    static const int N_ARG_VEC_REG = 8;
    static Register* _argVecRegs[N_ARG_VEC_REG];
};

#endif
