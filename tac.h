#ifndef _WGTCC_TAC_H_
#define _WGTCC_TAC_H_

#include <cinttypes>
#include <string>

namespace tac {
/*
 * We donot explicitly generate AST.
 * Instead, TAC is chosen to replace it.
 * The design of TAC here is based on the 'Dragon Book'.
 */

enum class OperandType {
  SIGNED,
  UNSIGNED,
  FLOAT,
  AGGREGATE,
};


class Operand {
public:
  virtual ~Operand() {}

  // The readable representation of operands
  virtual const std::string Repr() const = 0;

  bool IsInteger() const {
    return ty_ == OperandType::SIGNED ||
           ty_ == OperandType::UNSIGNED;
  }
  bool IsUnsigned()   const { return ty_ == OperandType::UNSIGNED; }
  bool IsSigned()     const { return ty_ == OperandType::SIGNED; }
  bool IsFloat()      const { return ty_ == OperandType::FLOAT; }
  bool IsAggregate()  const { return ty_ == OperandType::AGGREGATE; }

protected:
  Operand(size_t width, OperandType ty): width_(width), ty_(ty) {}

  size_t width_;
  OperandType ty_;
};


class Variable: Operand {
public:
  static Variable* New(size_t width, OperandType ty, const std::string* name);
  virtual ~Variable() {}
  virtual const std::string Repr() const { return *name_; }

private:
  Variable(size_t width, OperandType ty, const std::string* name)
      : Operand(width, ty), name_(name) {}
  Variable(size_t width, OperandType ty, const ssize_t offset)
      : Operand(width, ty), offset_(offset) {}

  // For code gen
  
  const std::string* name_;
  ssize_t offset_;
};


class Constant: Operand {
public:
  static Constant* New(size_t width, OperandType ty, uint64_t val);
  static Constant* Zero();
  static Constant* One();
  virtual ~Constant() {}
  virtual const std::string Repr() const { return std::to_string(val_); }

private:
  Constant(size_t width, OperandType ty, uint64_t val)
      : Operand(width, ty), val_(val) {}

  // For a floating pointer number,
  // the value has been converted
  uint64_t val_;
};


// Mapping to infinite register
class Temporary: Operand {
public:
  static Temporary* New(size_t width, OperandType ty);
  virtual ~Temporary() {}
  virtual const std::string Repr() const { return "t" + std::to_string(id_); }

private:
  Temporary(size_t width, OperandType ty)
      : Operand(width, ty), id_(GenId()) {}
  static size_t GenId() {
    static size_t id = 0;
    return ++id;
  }

  size_t id_;
};


/*
 * Each operator can be mapped into a machine instruction.
 * Or, a couple of machine instructions(e.g. POST_INC/POST_DEC).
 */
enum class Operator {
  // Binary
  ADD,      // '+'
  SUB,      // '-'
  MUL,      // '*'
  DIV,      // '/'
  LESS,     // '<'
  GREATER,  // '>'
  EQ,       // '=='
  NE,       // '!='
  LE,       // '<='
  GE,       // '>='
  L_SHIFT,  // '<<'
  R_SHIFT,  // '>>'
  OR,       // '|'
  AND,      // '&'
  XOR,      // '^'
  
  // Assignment
  ASSIGN,   // '='
  // x[n]: desginate variable n bytes after x
  DES_SS_ASSIGN,  // x[n] = y; Des subscripted assignment
  SRC_SS_ASSIGN,  // x = y[n]; Src subscripted assignment 

  // Unary
  PRE_INC,  // '++' ; t = ++x
  POST_INC, // '++' ; t = x++
  PRE_DEC,  // '--' ; t = --x
  POST_DEC, // '--' ; t = x--
  PLUS,     // '+'  ; t = +x
  MINUS,    // '-'  ; t = -x
  ADDR,     // '&'  ; x = &y
  DEREF,    // '*'  ; x = *y
  COMPT,    // '~'  ; x = ~y
  NOT,      // '!'  ; x = !y
  CAST,

  // Function
  PARAM,    // e.g. param x1
  CALL,     // e.g. call f, n

  // Jump
  JUMP,         // goto des
  IF,           // if (lhs) goto des
  IF_FALSE,     // if (!lhs) goto des
  //IF_LESS,    // if (lhs < rhs)  goto des
  //IF_GREATER, // if (lhs > rhs)  goto des
  //IF_LE,      // if (lhs <= rhs) goto des
  //IF_GE,      // if (lhs >= rhs) goto des
  //IF_EQ,      // if (lhs == rhs) goto des
  //IF_NE,      // if (lhs != rhs) goto des

  //LABEL,  // temporary jump dest
};


class TAC {
public:
  static TAC* NewBinary(Operator op, Operand* des,
                        Operand* lhs, Operand* rhs);
  static TAC* NewUnary(Operator op, Operand* des, Operand* operand);
  static TAC* NewAssign(Operand* des, Operand* src);
  static TAC* NewDesSSAssign(Operand* des, Operand* src, ssize_t offset);
  static TAC* NewSrcSSAssign(Operand* des, Operand* src, ssize_t offset);
  static TAC* NewJump(TAC* des);
  static TAC* NewIf(Operand* cond, TAC* des);
  static TAC* NewIfFalse(Operand* cond, TAC* des);
  //static TAC* NewLabel();
  ~TAC() {}

private:
  TAC(Operator op, Operand* res=nullptr,
      Operand* lhs=nullptr, Operand* rhs=nullptr)
    : op_(op), res_(res), lhs_(lhs), rhs_(rhs) {}
  TAC(Operator op, Operand* res=nullptr,
      Operand* lhs=nullptr, ssize_t n)
    : op_(op), res_(res), lhs_(lhs), n_(n) {}

  Operator op_; 
  Operand* des_; 
  Operand* lhs_;
  union {
    Operand* rhs_;
    ssize_t n_;
  }
};

}

#endif
