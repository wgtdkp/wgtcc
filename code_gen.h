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

    virtual int Width(void) = 0;

protected:
    Operand(void) {}

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
        RIP, N_REG
    };

    virtual ~Register(void) {}

    virtual Register* ToRegister(void) {
        return this;
    }

    virtual std::string Repr(void) const;

    bool  Is(int tag) const {
        return _tag == tag;
    }

    static Register* Get(int tag) {
        assert(0 <= tag && tag < N_REG);

        return _regs[tag];
    }

    bool Allocated(void) {
        return _allocated;
    }

    void SetAllocated(bool allocated) {
        _allocated = allocated;
    }

    bool Spilled(void) {
        return _spills.size();
    }

    void AddSpill(Memory* mem) {
        _spills.push_back(mem);
    }

    Memory* RemoveSpill(void) {
        auto ret = _spills.back();
        _spills.pop_back();
        return ret;
    }

    virtual int Width(void) {
        return _width;
    }

    void SetWidth(int width) {
        _width = width;
    }

private:
    static Register* New(int tag, int width=8);
    
    explicit Register(int tag, int width=8)
            : _allocated(false), _width(width), _tag(tag) {}
    
    bool _allocated;
    int _width;
    int _tag;
    std::vector<Memory*> _spills;

    static Register* _regs[N_REG];
    static const char* _reprs[N_REG][4];
};


class Immediate: public Operand
{
public:
    static Immediate* New(long val, int width=4);

    virtual ~Immediate(void) {}

    virtual Immediate* ToImmediate(void) {
        return this;
    }

    virtual std::string Repr(void) const;

    virtual int Width(void) {
        return _width;
    }

    long Val(void) const {
        return _val;
    }

private:
    explicit Immediate(long val, int width)
            : _val(val), _width(width) {}

    long _val;
    int _width;
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
    static Memory* New(Type* type, Register* base, int disp,
            Register* index=nullptr, int scale=0);

    virtual ~Memory(void) {}

    virtual Memory* ToMemory(void) {
        return this;
    }
    
    virtual int Width(void) {
        return _type->Width();
    }

    virtual std::string Repr(void) const;

private:
    Memory(Type* type, Register* base, int disp,
            Register* index=nullptr, int scale=0)
            : _type(type), _base(base), _index(index),
              _scale(scale), _disp(disp) {}

    //Memory()
    Type* _type;
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
              _argStackOffset(-8) {}

    //Expression
    virtual Operand* GenBinaryOp(BinaryOp* binaryOp);
    virtual Operand* GenUnaryOp(UnaryOp* unaryOp);
    virtual Operand* GenConditionalOp(ConditionalOp* condOp);
    virtual Register* GenFuncCall(FuncCall* funcCall);
    virtual Memory* GenObject(Object* obj);
    virtual Immediate* GenConstant(Constant* cons);
    virtual Register* GenTempVar(TempVar* tempVar);

    Memory* GenMemberRefOp(BinaryOp* binaryOp);
    Memory* GenSubScriptingOp(BinaryOp* binaryOp);
    Operand* GenAndOp(BinaryOp* binaryOp);
    Operand* GenOrOp(BinaryOp* binaryOp);
    Register* GenAddOp(BinaryOp* binaryOp);
    Register* GenSubOp(BinaryOp* binaryOp);
    Memory* GenAssignOp(BinaryOp* binaryOp);
    Register* GenCastOp(UnaryOp* cast);
    Memory* GenDerefOp(UnaryOp* deref);

    //statement
    virtual Operand* GenIfStmt(IfStmt* ifStmt);
    virtual Operand* GenJumpStmt(JumpStmt* jumpStmt);
    virtual Operand* GenReturnStmt(ReturnStmt* returnStmt);
    virtual Operand* GenLabelStmt(LabelStmt* labelStmt);
    virtual Operand* GenCompoundStmt(CompoundStmt* compoundStmt);

    //Function Definition
    virtual Register* GenFuncDef(FuncDef* funcDef);

    //Translation Unit
    virtual void GenTranslationUnit(TranslationUnit* unit);
    void Gen(void);

    void PushFuncArg(Expr* arg, ParamClass cls);
    void PushReturnAddr(int addr, Register* reg);

    Register* AllocArgReg(int width);

    //std::vector<Register*> GetArgReg(std::vec)


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

    // May fail, return nullptr;
    Register* TryAllocReg(Register* reg, int width);

    Register* AllocReg(int width, bool flt, Operand* except=nullptr);
    void Free(Operand* operand);

    void Spill(Register* reg);

    void Reload(Register* reg);

    void Push(int width, int align) {
        auto offset = Top() - width;
        offset = Type::MakeAlign(offset, align);
        _offsets.push_back(offset);
    }

    void Pop(void) {
        assert(_offsets.size() > 1);
        _offsets.pop_back();
    }

    int Top(void) {
        assert(_offsets.size());
        return _offsets.back();
    }

    Register* Load(Operand* operand);

    void Emit(const char* format, ...);

    // lhsWidth enabled only when lhs == rhs
    void Emit(const std::string& inst, Register* des, Operand* src);
    void EmitCAST(const std::string& cast,
        Register* des, Register* src, int desWidth=0);
    void EmitLoad(const std::string& load, Register* des, Memory* src);
    void EmitLoad(const std::string& load, Register* des, int src);
    void EmitStore(Memory* des, Register* src);
    
    void EmitLEA(Register* des, Memory* src);

    void EmitJE(LabelStmt* label);
    void EmitJNE(LabelStmt* label);
    void EmitJMP(LabelStmt* label);
    void EmitLabel(LabelStmt* label);
    
    //void EmitCMP(Immediate* lhs, Operand* rhs);
    void EmitCMP(int imm, Register* reg);
    void EmitPUSH(Operand* operand);
    void EmitPOP(Operand* operand);

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

    std::vector<int> _offsets {0};

    static const int N_ARG_REG = 6;
    static Register* _argRegs[N_ARG_REG];

    static const int N_ARG_VEC_REG = 8;
    static Register* _argVecRegs[N_ARG_VEC_REG];
};

#endif
