#ifndef _WGTCC_TAC_H_
#define _WGTCC_TAC_H_

#include <cinttypes>
#include <string>

class Type;
class Object;
class ASTConstant;

/*
 * We donot explicitly generate AST.
 * Instead, TAC is chosen to replace it.
 * The design of TAC here is based on the 'Dragon Book'.
 */

class Operand;
class Variable;
class Constant;
class Temporary;
class Translator;
class LValTranslator;
enum class OperandType;

OperandType ToTACOperandType(const Type* type);

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
    return type_ == OperandType::SIGNED ||
           type_ == OperandType::UNSIGNED;
  }
  bool IsUnsigned()   const { return type_ == OperandType::UNSIGNED; }
  bool IsSigned()     const { return type_ == OperandType::SIGNED; }
  bool IsFloat()      const { return type_ == OperandType::FLOAT; }
  bool IsAggregate()  const { return type_ == OperandType::AGGREGATE; }
  size_t width()      const { return width_; }
  OperandType type()  const { return type_; }

protected:
  Operand(size_t width, OperandType type): width_(width), type_(type) {}
  Operand(const Operand& other) {
    width_ = other.width_;
    type_ = other.type_;
  }

  size_t width_;
  OperandType type_;
};


class Variable: public Operand {
public:
  static Variable* New(const Object* obj);
  static Variable* New(const Type* type, Operand* base);
  // Copy constructor
  static Variable* New(const Variable& other);

  virtual ~Variable() {}
  virtual const std::string Repr() const { return name_; }
  ssize_t offset() const { return offset_; }
  void set_offset(ssize_t offset) { offset_ = offset; }
  const std::string& name() const { return name_; }
  void set_name(const std::string& name) {name_ = name; }
  Operand* base() { return base_; }
  const Operand* base() const { return base_; }

private:
  // Creating a global or static variable
  Variable(size_t width, OperandType type, const std::string& name)
      : Operand(width, type), name_(name), offset_(0), base_(nullptr) {}

  // Creating a stack variable
  Variable(size_t width, OperandType type, ssize_t offset)
      : Operand(width, type), name_(""), offset_(offset), base_(nullptr) {}
  
  // Creating a derefed variable *p
  Variable(size_t width, OperandType type, Operand* base)
      : Operand(width, type), name_(""), offset_(0), base_(base) {}

  Variable(const Variable& other)
      : Operand(other.width(), other.type()) {
    name_   = other.name_;
    offset_ = other.offset_;
    base_   = other.base_;
  }

  // A global or static variable is represented by its name
  std::string name_;

  // The offset to the stack frame register %rbp or the 
  // variable specified by member name_
  ssize_t offset_;

  // It represents the deref form *p,
  // base_ could only be variable or temporary
  Operand* base_;  
};


class Constant: public Operand {
public:
  static Constant* New(const ASTConstant* c);
  static Constant* Zero();
  static Constant* One();
  virtual ~Constant() {}
  virtual const std::string Repr() const { return std::to_string(val_); }
  uint64_t val() const { return val_; }

private:
  Constant(size_t width, OperandType type, uint64_t val)
      : Operand(width, type), val_(val) {}

  // For a floating pointer number,
  // the value has been converted
  uint64_t val_;
};


// Mapping to infinite register
class Temporary: public Operand {
public:
  static Temporary* New(const Type* type);
  virtual ~Temporary() {}
  virtual const std::string Repr() const {
    return "t" + std::to_string(id_);
  }

private:
  Temporary(size_t width, OperandType type)
      : Operand(width, type), id_(GenId()) {}
  static size_t GenId() {
    static size_t id = 0;
    return ++id;
  }

  size_t id_;
};


class TAC {
public:
  // Each operator can be mapped into a machine instruction.
  // Or, a couple of machine instructions(e.g. POST_INC/POST_DEC).
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
    DES_SS_ASSIGN,  // '[]=' ; x[n] = y; Des subscripted assignment
    SRC_SS_ASSIGN,  // '=[]' ; x = y[n]; Src subscripted assignment 
    DEREF_ASSIGN,   // '*='  ; *x = y;

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

    LABEL,  // jump dest
  };

public:
  static TAC* NewBinary(Operator op, Operand* des,
                        Operand* lhs, Operand* rhs);
  static TAC* NewUnary(Operator op, Operand* des, Operand* operand);
  static TAC* NewAssign(Operand* des, Operand* src);
  static TAC* NewJump(TAC* des);
  static TAC* NewIf(Operand* cond, TAC* des);
  static TAC* NewIfFalse(Operand* cond, TAC* des);
  static TAC* NewLabel() {
    return NewBinary(Operator::LABEL, nullptr, nullptr, nullptr);
  }
  ~TAC() {}

private:
  TAC(Operator op, Operand* des=nullptr,
      Operand* lhs=nullptr, Operand* rhs=nullptr)
    : op_(op), des_(des), lhs_(lhs), rhs_(rhs) {}
  TAC(Operator op, Operand* lhs=nullptr, TAC* jump_des=nullptr)
    : op_(op), des_(nullptr), lhs_(lhs), jump_des_(jump_des) {}

  Operator op_; 
  Operand* des_; 
  Operand* lhs_;
  union {
    Operand* rhs_;
    TAC* jump_des_;
  };
};

#endif
