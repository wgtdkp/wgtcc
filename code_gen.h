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
    friend class Generator;

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
        return this == Get(tag);
    }

    static Register* Get(int tag) {
        assert(0 <= tag && tag < N_REG);

        return &_regs[tag];
    }

private:
    explicit Register(void) {}

    static Register _regs[N_REG];
};


class Immediate: public Operand
{
    friend class Generator;

public:
    ~Immediate(void) {}

    virtual std::string Repr(void) const;

    virtual Immediate* ToImmediate(void) {
        return this;
    }

private:
    explicit Immediate(Constant* cons): _cons(cons) {}

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
    ~Memory(void) {}

    virtual std::string Repr(void) const;

    virtual Memory* ToMemory(void) {
        return this;
    }

private:
    Memory(Register* base, int disp, Register* index=nullptr, int scale=0)
            : _base(base), _index(index), _scale(scale), _disp(disp) {}

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
              _argStackOffset(-8),
              _immFalse(NewImmediate(T_INT, 0)),
              _immTrue(NewImmediate(T_INT, 1)) {}
    
    Immediate* NewImmediate(Constant* cons);
    Immediate* NewImmediate(int tag, long long val);

    
    Memory* NewMemory(Register* base, int disp,
            Register* index=nullptr, int scale=0);

    //Expression
    virtual Operand* GenBinaryOp(BinaryOp* binaryOp);
    virtual Operand* GenUnaryOp(UnaryOp* unaryOp);
    virtual Operand* GenConditionalOp(ConditionalOp* condOp);
    virtual Register* GenFuncCall(FuncCall* funcCall);
    virtual Memory* GenObject(Object* obj);
    virtual Immediate* GenConstant(Constant* cons);
    virtual Register* GenTempVar(TempVar* tempVar);

    Operand* GenMemberRefOp(Operand* lhs, Memory* rhs);
    Operand* GenSubScriptingOp(Operand* lhs, Operand* rhs, int scale);
    Operand* GenAndOp(Expr* lhsExpr, Expr* rhsExpr);
    Operand* GenOrOp(Expr* lhsExpr, Expr* rhsExpr);
    Register* GenAddOp(Expr* lhsExpr, Expr* rhsExpr);
    Register* GenSubOp(Expr* lhsExpr, Expr* rhsExpr);

    //statement
    virtual void GenStmt(Stmt* stmt);
    virtual void GenIfStmt(IfStmt* ifStmt);
    virtual void GenJumpStmt(JumpStmt* jumpStmt);
    virtual void GenReturnStmt(ReturnStmt* returnStmt);
    virtual void GenLabelStmt(LabelStmt* labelStmt);
    virtual void GenEmptyStmt(EmptyStmt* emptyStmt);
    virtual void GenCompoundStmt(CompoundStmt* compoundStmt);

    //Function Definition
    virtual void GenFuncDef(FuncDef* funcDef);

    //Translation Unit
    virtual void GenTranslationUnit(TranslationUnit* transUnit);

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

    // TODO(wgtdkp):
    Register* AllocReg(void) {
        return nullptr;
    }

    void Emit(const char* format, ...);
    void EmitMOV(Operand* operand, Register* reg) {}
    void EmitLEA(Memory* mem, Register* reg) {}
    void EmitJE(LabelStmt* label) {}
    void EmitJNE(LabelStmt* label) {}
    void EmitJMP(LabelStmt* label) {}
    void EmitCMP(Immediate* lhs, Operand* rhs) {}
    void EmitLabel(LabelStmt* label) {}

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

    Immediate* const _immFalse;
    Immediate* const _immTrue;

    static const int N_ARG_REG = 6;
    static Register* _argRegs[N_ARG_REG];

    static const int N_ARG_VEC_REG = 8;
    static Register* _argVecRegs[N_ARG_VEC_REG];

    MemPoolImp<Immediate>   _immediatePool;
    MemPoolImp<Memory>      _memoryPool;
};

#endif
