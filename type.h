#ifndef _WGTCC_TYPE_H_
#define _WGTCC_TYPE_H_

#include "mem_pool.h"
#include "scope.h"

#include <cassert>

#include <algorithm>
#include <list>


/********* Type System ***********/
class Scope;
class Token;
class Expr;

class Type;
class VoidType;
class Identifier;
class Object;
class Constant;

class ArithmType;
class DerivedType;
class ArrayType;
class FuncType;
class PointerType;
class StructType;
class EnumType;


enum {
  /*****storage-class-specifiers*****/
  S_TYPEDEF = 0x01,
  S_EXTERN = 0x02,
  S_STATIC = 0x04,
  S_THREAD = 0x08,
  S_AUTO = 0x10,
  S_REGISTER = 0x20,

  /*****type-specifier*****/
  T_SIGNED = 0x40,
  T_UNSIGNED = 0x80,
  T_CHAR = 0x100,
  T_SHORT = 0x200,
  T_INT = 0x400,
  T_LONG = 0x800,
  T_VOID = 0x1000,
  T_FLOAT = 0x2000,
  T_DOUBLE = 0x4000,
  T_BOOL = 0x8000,
  T_COMPLEX = 0x10000,
  //T_ATOMIC = 0x20000,
  T_STRUCT_UNION = 0x40000,
  T_ENUM = 0x80000,
  T_TYPEDEF_NAME = 0x100000,

  /*****type-qualifier*****/
  Q_CONST = 0x200000,
  Q_RESTRICT = 0x400000,
  Q_VOLATILE = 0x800000,
  Q_ATOMIC = 0x1000000,

  T_LLONG = 0x2000000,

  /*****function specifier*****/
  F_INLINE = 0x4000000,
  F_NORETURN = 0x8000000,
};


class QualType {
public:
  //QualType(Type* ptr): val_(ptr) {}
  //QualType(Type* ptr, int quals): 

  void AddConst(void) {
  
  }

private:
  struct {
    uint64_t qual_: 3;
    uint64_t ptr_: sizeof(uint64_t) * 8 - 3;
  } val_;
};


class Type
{
public:
  static const int intWidth_ = 4;
  static const int machineWidth_ = 8;

  bool operator!=(const Type& other) const = delete;
  bool operator==(const Type& other) const = delete;

  virtual bool Compatible(const Type& other) const {
    return complete_ == other.complete_;
  }

  virtual ~Type() {}

  // For Debugging
  virtual std::string Str() const = 0; 
  virtual int Width() const = 0;
  virtual int Align() const { return Width(); }
  static int MakeAlign(int offset, int align) {
    if ((offset % align) == 0)
      return offset;
    if (offset >= 0)
      return offset + align - (offset % align);
    else
      return offset - align - (offset % align);
  } 

  static Type* MayCast(Type* type);
  int Qual() const { return qual_; }
  void SetQual(int qual) { qual_ = qual; }
  bool Complete() const { return complete_; }
  void SetComplete(bool complete) { complete_ = complete; }

  bool IsReal() const { return IsInteger() || IsFloat(); };  
  virtual bool IsScalar() const { return false; }
  virtual bool IsFloat() const { return false; }
  virtual bool IsInteger() const { return false; }
  virtual bool IsBool() const { return false; }
  virtual bool IsVoidPointer() const { return false; }
  virtual bool IsUnsigned() const { return false; }
   
  virtual VoidType* ToVoid() { return nullptr; }
  virtual const VoidType* ToVoid() const { return nullptr; }
  virtual ArithmType* ToArithm() { return nullptr; }  
  virtual const ArithmType* ToArithm() const { return nullptr; }  
  virtual ArrayType* ToArray() { return nullptr; }
  virtual const ArrayType* ToArray() const { return nullptr; }
  virtual FuncType* ToFunc() { return nullptr; }
  virtual const FuncType* ToFunc() const { return nullptr; }
  virtual PointerType* ToPointer() { return nullptr; }
  virtual const PointerType* ToPointer() const { return nullptr; }
  virtual DerivedType* ToDerived() { return nullptr; }
  virtual const DerivedType* ToDerived() const { return nullptr; }
  virtual StructType* ToStruct() { return nullptr; }
  virtual const StructType* ToStruct() const { return nullptr; }
  virtual EnumType* ToEnum() { return nullptr; }
  virtual const EnumType* ToEnum() const { return nullptr; }

protected:
  Type(MemPool* pool, bool complete)
      : qual_(0), complete_(complete), pool_(pool) {}

  // C11 6.7.3 [4]: The properties associated with qualified types
  // are meaningful only for expressions that are lvalues.
  // It is convenient to carry qualification within type
  // But it's not used to decide the compatibility of two types.
  mutable int qual_;
  bool complete_;
  MemPool* pool_;
};


class VoidType : public Type
{
public:
  static VoidType* New();

  virtual ~VoidType() {}

  virtual VoidType* ToVoid() { return this; }
  virtual const VoidType* ToVoid() const { return this; }
  virtual bool Compatible(const Type& other) const {
    return other.ToVoid();
  }

  virtual int Width() const {
    // Non-standard GNU extension
    return 1;
  }

  virtual std::string Str() const { return "void:1"; }

protected:
  explicit VoidType(MemPool* pool): Type(pool, false) {}
};


class ArithmType : public Type
{
public:
  static ArithmType* New(int typeSpec);

  virtual ~ArithmType() {}
  virtual ArithmType* ToArithm() { return this; }
  virtual const ArithmType* ToArithm() const { return this; }
  virtual bool Compatible(const Type& other) const {
    // C11 6.2.7 [1]: Two types have compatible type if their types are the same
    // But i would to loose this constraints: integer and pointer are compatible
    if (IsInteger() && other.ToPointer())
      return other.Compatible(*this);
    return this == &other;
  }

  virtual int Width() const;
  virtual std::string Str() const;
  virtual bool IsScalar() const { return true; }
  virtual bool IsInteger() const { return !IsFloat() && !IsComplex(); }
  virtual bool IsUnsigned() const { return tag_ & T_UNSIGNED; }
  virtual bool IsFloat() const {
    return (tag_ & T_FLOAT) || (tag_ & T_DOUBLE);
  }
  virtual bool IsBool() const { return tag_ & T_BOOL; }
  bool IsComplex() const { return tag_ & T_COMPLEX; }
  int Tag() const { return tag_; }
  int Rank() const;
  static ArithmType* IntegerPromote(ArithmType* type) {
    assert(type->IsInteger());
    if (type->Rank() < ArithmType::New(T_INT)->Rank())
      return ArithmType::New(T_INT);
    return type;
  }
  static ArithmType* MaxType(ArithmType* lhsType, ArithmType* rhsType);

protected:
  explicit ArithmType(MemPool* pool, int spec)
    : Type(pool, true), tag_(Spec2Tag(spec)) {}

private:
  static int Spec2Tag(int spec);

  int tag_;
};


class DerivedType : public Type
{
public:
  Type* Derived() {
    return derived_;
  }
  
  void SetDerived(Type* derived) {
    derived_ = derived;
  }
  
  virtual DerivedType* ToDerived() {
    return this;
  }
  
  virtual const DerivedType* ToDerived() const {
    return this;
  }

protected:
  DerivedType(MemPool* pool, Type* derived)
      : Type(pool, true), derived_(derived) {}

  Type* derived_;
};


class PointerType : public DerivedType
{
public:
  static PointerType* New(Type* derived);

  ~PointerType() {}
  virtual PointerType* ToPointer() { return this; }
  virtual const PointerType* ToPointer() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int Width() const { return 8; }
  virtual bool IsScalar() const { return true; }
  virtual bool IsVoidPointer() const { return derived_->ToVoid(); }
  virtual std::string Str() const {
    return derived_->Str() + "*:" + std::to_string(Width());
  }

protected:
  PointerType(MemPool* pool, Type* derived): DerivedType(pool, derived) {}
};


class ArrayType : public DerivedType
{
public:
  static ArrayType* New(int len, Type* eleType);
  static ArrayType* New(Expr* expr, Type* eleType);
  virtual ~ArrayType() { /*delete derived_;*/ }

  virtual ArrayType* ToArray() { return this; }
  virtual const ArrayType* ToArray() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int Width() const {
    return Complete() ? (derived_->Width() * len_): 0;
  }
  virtual int Align() const { return derived_->Align(); }
  virtual std::string Str() const {
    return derived_->Str() + "[]:" + std::to_string(Width());
  }

  int GetElementOffset(int idx) const { return derived_->Width() * idx; }
  int Len() const { return len_; }
  void SetLen(int len) { len_ = len; }
  bool Variadic() const { return lenExpr_ != nullptr; }

protected:
  ArrayType(MemPool* pool, Expr* lenExpr, Type* derived)
      : DerivedType(pool, derived),
        lenExpr_(lenExpr), len_(0) {
    SetComplete(false);
    SetQual(Q_CONST);
  }
  
  ArrayType(MemPool* pool, int len, Type* derived)
      : DerivedType(pool, derived),
        lenExpr_(nullptr), len_(len) {
    SetComplete(len_ >= 0);
    SetQual(Q_CONST);
  }
  const Expr* lenExpr_;
  int len_;
};


class FuncType : public DerivedType
{
public:
  typedef std::vector<Object*> ParamList;

public:
  static FuncType* New(Type* derived, int funcSpec,
      bool variadic, const ParamList& params);

  ~FuncType() {}
  
  virtual FuncType* ToFunc() { return this; }
  virtual const FuncType* ToFunc() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int Width() const { return 1; }
  virtual std::string Str() const;
  //bool IsInline() const { inlineNoReturn_ & F_INLINE; }
  //bool IsNoReturn() const { return inlineNoReturn_ & F_NORETURN; }
  const ParamList& Params() const { return params_; }
  void SetParams(const ParamList& params) { params_ = params; }
  bool Variadic() const { return variadic_; }

protected:
  FuncType(MemPool* pool, Type* derived, int inlineReturn,
           bool variadic, const ParamList& params)
      : DerivedType(pool, derived), inlineNoReturn_(inlineReturn),
        variadic_(variadic), params_(params) {
    SetComplete(false);
  }

private:
  int inlineNoReturn_;
  bool variadic_;
  ParamList params_;
};


class StructType : public Type
{
public:
  typedef std::list<Object*> MemberList;
  typedef std::list<Object*>::iterator Iterator;
  
public:
  static StructType* New(bool isStruct, bool hasTag, Scope* parent);
  
  ~StructType() {/*TODO: delete _env ?*/ }
  virtual StructType* ToStruct() { return this; }
  virtual const StructType* ToStruct() const { return this; }
  virtual bool Compatible(const Type& other) const;
  virtual int Width() const { return width_; }
  virtual int Align() const { return align_; }
  virtual std::string Str() const;

  // struct/union
  void AddMember(Object* member);
  void AddBitField(Object* member, int offset);
  bool IsStruct() const { return isStruct_; }
  Object* GetMember(const std::string& member);
  Scope* MemberMap() { return memberMap_; }
  MemberList& Members() { return members_; }
  int Offset() const { return offset_; }
  bool HasTag() { return hasTag_; }
  void MergeAnony(Object* anony);
  void Finalize();
  
protected:
  // default is incomplete
  StructType(MemPool* pool, bool isStruct, bool hasTag, Scope* parent);
  
  StructType(const StructType& other);

private:
  void CalcWidth();

  //static std::string AnonymousBitField() {
  //    static int id = 0;
  //    return std::to_string(id++) + "<anonymous>";
  //}

  bool isStruct_;
  bool hasTag_;
  Scope* memberMap_;

  MemberList members_;
  int offset_;
  int width_;
  int align_;
  int bitFieldAlign_;
};

// Not used yet
class EnumType: public Type {
public:
  static EnumType* New(bool complete=false);

  virtual ~EnumType() {}
  virtual bool IsInteger() const { return true; }
  virtual ArithmType* ToArithm() {
    assert(false);
    return ArithmType::New(T_INT);
  };
  virtual const ArithmType* ToArithm() const {
    assert(false);
    return ArithmType::New(T_INT);
  }

  virtual EnumType* ToEnum() { return this; }
  virtual const EnumType* ToEnum() const { return this; }
  virtual bool Compatible(const Type& other) const {
    // As enum is always casted to INT, there is no chance for this call    
    assert(false);
    return (other.ToEnum() || other.IsInteger()) && Type::Compatible(other);    
  }

  virtual int Width() const {
    return ArithmType::New(T_INT)->Width();
  }

  virtual std::string Str() const { return "enum:4"; }

protected:
  explicit EnumType(MemPool* pool, bool complete): Type(pool, complete) {}  
};

#endif
